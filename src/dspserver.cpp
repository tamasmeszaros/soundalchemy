/*
 * dspserver.cpp
 * 
 * Copyright (c) 2013 Mészáros Tamás.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 * 
 * Contributors:
 *     Mészáros Tamás - initial API and implementation
 */


#include <cstdarg>
#include <unistd.h>
#include <string>
#include "dspserver.h"


using namespace std;

namespace soundalchemy {



class LLAErrorHandler: public llaErrorHandler {
public:
	virtual void rawlog(TErrorLevel lvl, const char* srcfile, int line,
			TErrors err, const char* details){
		e_errorlevels saerr = LEVEL_INFO;
		switch(lvl) {
		case ERROR:
		case CRITICAL_ERROR: saerr = LEVEL_ERROR; break;
		case DEBUGMSG: saerr = LEVEL_DEBUG; break;
		case INFO: saerr = LEVEL_INFO; break;
		case WARNING: saerr = LEVEL_WARNING; break;
		}
		soundalchemy::log(saerr, getFormatedError(lvl, err, details));
	}

	LLAErrorHandler() {
		enableLogging(true, true);
		enableDebugMessages(true);
		//useExceptions(true);
	}
} llaEH;

llaDeviceManager& DspServer::lla_devman_ = llaDeviceManager::getInstance(llaEH);

DspServer::DspServer():
		dsp_process_(*this),
		clients_count_(0),
		messagequeue_(NULL)
		 {

	for (int i = 0; i < CLIENTS_MAX; i++)
		clients_[i] = NULL;

	this_thread_ = Thread::getCurrent();

//	llaEH.enableLogging(true, true);
//	llaEH.enableDebugMessages(true);
//	llaEH.useExceptions(true);
//	try {
//		lla_devman_.setErrorHandler(llaEH);
//	} catch (llaErrorHandler::Exception& ex) {
//		log(LEVEL_ERROR, ex.what());
//	}

	//lla_devman_.refresh();
}

DspServer::~DspServer() {

	stop();

	delete this_thread_;
	delete messagequeue_;

	try {
		lla_devman_.destroy();
	} catch (llaErrorHandler::Exception& ex) {
		log(LEVEL_ERROR, ex.what());
	}
}

void DspServer::stop() {
	return dsp_process_.stopProcessing();
}

TAlchemyError DspServer::start() {
	return dsp_process_.startProcessing();
}

TAlchemyError DspServer::DspProcess::startProcessing(void) {

	TAlchemyError retval = E_OK;
	lock();
	int ret;
	switch (state_.val) {
	case ST_STOPPED:
		ret = proc_thread_->run(*this);
		if (ret) {
			retval = E_START;
			log(LEVEL_ERROR, STR_ERRORS[retval]);
		}
		waitFor();
		break;
	default:
		break;
	}
	unlock();

	if(state_.val != ST_RUNNING) {
		log(LEVEL_ERROR, STR_ERRORS[E_START]);
		retval = E_START;
	}

	return retval;
}

void DspServer::DspProcess::stopProcessing(void) {
	lock(); state_.state_requested_ = ST_STOPPED; unlock();
	Thread* current = Thread::getCurrent();
	proc_thread_->join(*current);
	delete current;
}


void DspServer::processMessage(InboundMessage& msg) {
	if(messagequeue_ != NULL) messagequeue_->pushBack(&msg);
}

TAlchemyError DspServer::listenOn(ClientConnector& client) {
	clients_[clients_count_] = &client;
	client.connect(*this);


	TAlchemyError ret = clients_[clients_count_]->watch();

	if (ret != E_OK) {
		log(LEVEL_ERROR,
				"%s. Command prompt controller cannot be established.",
				STR_ERRORS[E_START_LISTENER]);
	}
	else {
		clients_count_++;
		OutboundMessage* msgclientid =
				OutboundMessage::MsgSendClientID(clients_count_);
		client.setChannelId(clients_count_);
		client.send( *msgclientid );
		delete msgclientid;
	}

	return ret;
}

void DspServer::getDeviceList(AudioInf& devicelist) {

	for( llaDeviceIterator devices = lla_devman_.getDeviceIterator(); !devices.end(); devices++) {
		devicelist.beginDevice(devices->getName(), devices->getName(true));
		for(llaDevice::IStreamIterator i = devices->getInputStreamIterator(); !i.end(); i++) {
			devicelist.stream(i->getId(), i->getName(), true);

		}
		devicelist.endDevice();
		for(llaDevice::OStreamIterator i = devices->getOutputStreamIterator(); !i.end(); i++) {
			devicelist.stream(i->getId(), i->getName(), false);
		}
		devicelist.endDevice();
	}
}

void DspServer::startListening(void) {

	InboundMessage *instruction;
	OutboundMessage *reply;
	exit_ = false;

	// allocating the message queue facility
	if(messagequeue_ == NULL) messagequeue_ = new MessageQueue();

	while(!exit_) {

		// this call blocks if the queue is empty
		// allowing a passive waiting for a message and process it. Meanwhile,
		// other messages are buffered in the queue.
		instruction = messagequeue_->popFront();
		if(instruction != NULL) {
			reply = instruction->instruct(*this);
			broadcastMessage(*reply);
			delete instruction;
		}
	}
}

void DspServer::stopListening(void) {
	stop();
	exit_ = true;
}

DspServer::DspProcess::DspProcess(DspServer& server):
		dspserver_(server) {
	callback_counter_ = 0;
	state_.val = ST_STOPPED;
	state_.state_requested_ = ST_STOPPED;
	proc_thread_ = Thread::getNewThread();
	if( proc_thread_->setRealtime())
		log(LEVEL_WARNING, "Cannot set real time policy");

}

void * DspServer::DspProcess::run() {

	lock(); state_.state_requested_ = ST_RUNNING; unlock();

	TErrors e = llaudio::E_OK;
	TState st;

	// This thread will consume most of its life in the connectStream function
	// in which the processing is done. It calls the onSamplesReady method
	// whenever samples are ready to be processed
	e = connectStreams(dspserver_.proc_graph_.getInput(),
			dspserver_.proc_graph_.getOutput());


	lock(); st =  state_.state_requested_; unlock();

	state_.val = (TState)(ST_STOPPED);
	if (e != llaudio::E_OK || st != ST_STOPPED) {
		proc_thread_->wakeUpAll();
		log(LEVEL_ERROR, "Alchemy server stopped unexpectedly");
	}
	return NULL;
}

void DspServer::DspProcess::onSamplesReady(void) {
	lock();
	if (state_.state_requested_ == ST_RUNNING) {
		state_.val = ST_RUNNING;
		unlock();
		// Signal the control process waiting for the result of startProcessing
		// call that the processing goes well and it can continue broadcasting
		// this fact to clients
		if (callback_counter_ == 2)
			proc_thread_->wakeUpAll();
	} else {
		unlock();
		return;
	}
	dspserver_.proc_graph_.traverse(this->lastwrite_);
//	while ( enode != NULL ) {
//		// don't let the signal chain parameters to change while the samples
//		// are processed
//		// It's important to know the expensiveness of this operation.
//		// To lock a mutex, which is not already locked takes very little
//		// effort on linux. ( says Stack-owerflow )
//		lock();
//
//		// process the samples
//		enode->effect->process(this->lastwrite_);
//
//		// give room for the main thread to change some parameters
//		unlock();
//
//		enode = enode->next;
//	}
	callback_counter_++;
}

bool DspServer::DspProcess::stop(void) {
	bool ret = false;
	lock();
	if (state_.state_requested_ == ST_STOPPED) {
		// this will break the connection of audio streams as the "bool stop()"
		// function will return true. DspProcess::run will exit after that.
		callback_counter_ = 0;
		ret = true;
	}
	unlock();
	return ret;
}

TAlchemyError DspServer::setInputStream(TDeviceId device, TStreamId id) {
	llaDevice& dev = lla_devman_.getDevice(device );
	if(dev.isNull()) return E_SET_STREAM;

	llaInputStream& is = dev.getInputStream(id);
	if(is.isNull()) return E_SET_STREAM;

	proc_graph_.setInput(is);

	return E_OK;
}

TAlchemyError DspServer::setOutputStream(TDeviceId device, TStreamId id) {
	llaDevice& dev = lla_devman_.getDevice(device );
	if(dev.isNull()) return E_SET_STREAM;

	llaOutputStream& os = dev.getOutputStream(id);
	if(os.isNull()) return E_SET_STREAM;

	proc_graph_.setOutput(os);

	return E_OK;
}


void DspServer::broadcastMessage(OutboundMessage& message) {
	for (int i = 0; i < CLIENTS_MAX; i++) {
			if(clients_[i] != NULL) clients_[i]->send(message);
	}
}

/* ************************************************************************** */
DspServer::MessageQueue::MessageQueue() {
	monitor_ = Thread::getNewThread();
	writemutex_ = Thread::getMutex();
	waiters_ = 0;
	enabled_ = true;
}

DspServer::MessageQueue::~MessageQueue() {
	writemutex_->lock();
	cond_var_.lock();
	enabled_ = false;
	cond_var_.unlock();
	writemutex_->unlock();

	// delete remaining messages
	InboundMessage* temp;
	while(!cond_var_.queue_base_.empty()) {
		temp = cond_var_.queue_base_.front();
		delete temp;
		cond_var_.queue_base_.pop();
	}

	delete writemutex_;
	delete monitor_;
}

InboundMessage* DspServer::MessageQueue::popFront(void) {
	if(!enabled_) return NULL;
	cond_var_.lock();
	writemutex_->lock();
	if (cond_var_.queue_base_.empty()) {
		writemutex_->unlock();
		waiters_++;
		monitor_->waitOn(cond_var_);
	} else
		writemutex_->unlock();

	cond_var_.unlock();
	InboundMessage* ret = cond_var_.queue_base_.front();
	cond_var_.queue_base_.pop();
	return ret;

}

void DspServer::MessageQueue::pushBack(InboundMessage* message) {
	if(!enabled_) return ;
	unsigned int waiters;

	cond_var_.lock();
	writemutex_->lock();
		waiters = waiters_;
		cond_var_.queue_base_.push(message);
	writemutex_->unlock();
	cond_var_.unlock();

	if (waiters > 0) {
		if (monitor_->wakeUp()) {
			log(LEVEL_ERROR,
			    "%s. No thread is waiting on the queue when attempt to wake up",
			    STR_ERRORS[E_QUEUE]);
		} else
			waiters_--;
	}

}

unsigned int DspServer::MessageQueue::getSize(void) {
	if(!enabled_) return 0;
	unsigned int size;

	cond_var_.lock();
	writemutex_->lock();

	size = cond_var_.queue_base_.size();

	writemutex_->unlock();
	cond_var_.unlock();

	return size;
}

bool DspServer::MessageQueue::isEmpty(void) {
	if(!enabled_) return true;
	bool empty;

	cond_var_.lock();
	writemutex_->lock();

	empty = cond_var_.queue_base_.empty();

	writemutex_->unlock();
	cond_var_.unlock();

	return empty;

}

}

