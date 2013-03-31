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
#include "androidconnector.h"


using namespace std;

namespace soundalchemy {

class LLAErrorHandler: public llaErrorHandler {
public:
	virtual void rawlog(TErrorLevel lvl, const char* srcfile, int line,
			TErrors err, const char* details){
		e_errorlevels saerr;
		switch(lvl) {
		case ERROR:
		case CRITICAL_ERROR: saerr = LEVEL_ERROR; break;
		case DEBUGMSG: saerr = LEVEL_DEBUG; break;
		case INFO: saerr = LEVEL_INFO; break;
		case WARNING: saerr = LEVEL_WARNING; break;
		}
		soundalchemy::log(saerr, getFormatedError(lvl, err, details));
	}
} llaEH;

DspServer::DspServer():
		dsp_process_(*this),
		lla_devman_(llaDeviceManager::getInstance()),
		remote_(false) {

	for (int i = 0; i < CLIENTS_MAX; i++)
		clients_[i] = NULL;

	this_thread_ = Thread::getCurrent();

	llaEH.enableLogging(true, true);
	llaEH.enableDebugMessages(true);
	llaEH.useExceptions(true);
	try {
		lla_devman_.setErrorHandler(llaEH);
	} catch (llaErrorHandler::Exception& ex) {
		log(LEVEL_ERROR, ex.what());
	}
}

DspServer::~DspServer() {

	for (int i = 0; i < CLIENTS_MAX; i++)
		delete clients_[i];

	stop();

	delete this_thread_;

	// TODO implement the signal chain
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
	state_requested_ = ST_STOPPED;
}


void DspServer::processMessage(InboundMessage& msg) {
	messagequeue_.pushBack(&msg);
	delete &msg;
}

TAlchemyError DspServer::listenOnCliPrompt(void) {
	clients_[CLIENT_COMMAND_PROMPT] = new AndroidConnector(*this);
	TAlchemyError ret = clients_[CLIENT_COMMAND_PROMPT]->watch();

	if (ret != E_OK)
		log(LEVEL_ERROR,
				"%s. Command prompt controller cannot be established.",
				STR_ERRORS[E_START_LISTENER]);

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
	remote_ = true;

	while(!exit_) {

		// this call blocks if the queue is empty
		// allowing a passive waiting for a message and process it. Meanwhile,
		// other messages are buffered in the queue.
		instruction = messagequeue_.popFront();
		reply = instruction->instruct(*this);
		broadcastMessage(*reply);
		delete instruction;
	}

}

DspServer::DspProcess::DspProcess(DspServer& server):
		dspserver_(server),
		state_requested_(ST_STOPPED) {
	callback_counter_ = 0;
	state_.val = (TState) ST_STOPPED;
	proc_thread_ = Thread::getNewThread();
	if( proc_thread_->setRealtime())
		log(LEVEL_WARNING, "Cannot set real time policy");

}

void * DspServer::DspProcess::run() {

	state_requested_ = ST_RUNNING;

	TErrors e = llaudio::E_OK;
	TState st;

	// This thread will consume most of its life in the connectStream function
	// in which the processing is done. It calls the onSamplesReady method
	// whenever samples are ready to be processed
	e = connectStreams(dspserver_.audio_input_.llainput,
				dspserver_.audio_output_.llaoutput);


	st = (TState) state_requested_;

	state_.val = (TState) ST_STOPPED;
	if( e != llaudio::E_OK || st != ST_STOPPED ) {
		proc_thread_->wakeUpAll();
		log(LEVEL_ERROR, "Alchemy server stopped unexpectedly");
	}

	return NULL;
}

void DspServer::DspProcess::onSamplesReady(void) {

	if(state_requested_ == ST_RUNNING) {
		state_.val = ST_RUNNING;

		// Signal the control process waiting for the result of startProcessing
		// call that the processing goes well and it can continue broadcasting
		// this fact to clients
		if(callback_counter_ == 2) proc_thread_->wakeUpAll();

	}
	else return;

	EffectNode* enode = &dspserver_.audio_input_;
	while ( enode != NULL ) {
		// don't let the signal chain parameters to change while the samples
		// are processed
		// It's important to know the expensiveness of this operation.
		// To lock a mutex, which is not already locked takes very little
		// effort on linux. ( says Stack-owerflow )
		lock();

		// process the samples
		enode->effect->process(this->lastwrite_);

		// give room for the main thread to change some parameters
		unlock();

		enode = enode->next;
	}

	callback_counter_++;
}

bool DspServer::DspProcess::stop(void) {
	if( state_requested_ == ST_STOPPED ) {
		// this will break the connection of audio streams as the "bool stop()"
		// function will return true. DspProcess::run will exit after that.
		callback_counter_ = 0;
		return true;
	}

	return false;
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
}

DspServer::MessageQueue::~MessageQueue() {
	delete writemutex_;
	delete monitor_;
}

InboundMessage* DspServer::MessageQueue::popFront(void) {
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
	unsigned int size;

	cond_var_.lock();
	writemutex_->lock();

	size = cond_var_.queue_base_.size();

	writemutex_->unlock();
	cond_var_.unlock();

	return size;
}

bool DspServer::MessageQueue::isEmpty(void) {
	bool empty;

	cond_var_.lock();
	writemutex_->lock();

	empty = cond_var_.queue_base_.empty();

	writemutex_->unlock();
	cond_var_.unlock();

	return empty;

}

//Message* ACK_START(TAlchemyString error, TChannelID chid);
//Message* ACK_STOP(TAlchemyString error, TChannelID chid);
//Message* ACK_GET_DEVICE_LIST(llaDeviceIterator& devices, TChannelID chid);
//Message* ACK_SET_INPUT_STREAM(TAlchemyString error, TChannelID chid);
//Message* ACK_SET_OUTPUT_STREAM(TAlchemyString error, TChannelID chid);

//Message* DspServer::ACK_START( TChannelID chid, TAlchemyString error) {
//	Message * ack = new Message(MSG_ACK);
//	ack->setChannelId(chid);
//	ack->addElement("request", Message::DATA(MSG_START));
//	if(!error.empty()) {
//		ack->addElement("error", Message::DATA(error));
//	}
//	return ack;
//}
//
//Message* DspServer::ACK_STOP(TChannelID chid, TAlchemyString error) {
//	Message * ack = new Message(MSG_ACK);
//	ack->setChannelId(chid);
//	ack->addElement("request", Message::DATA(MSG_STOP));
//	if(!error.empty()) {
//		ack->addElement("error", Message::DATA(error));
//	}
//	return ack;
//}
//
//Message* DspServer::ACK_GET_DEVICE_LIST(TChannelID chid, llaDeviceIterator& devices) {
//	Message * ack = new Message(MSG_ACK);
//	ack->setChannelId(chid);
//	ack->addElement("request", Message::DATA(MSG_GET_DEVICE_LIST));
//	for( ; !devices.end(); devices++) {
//		ack->beginObject(devices->getName());
//		ack->addElement("full_name", Message::DATA(devices->getName(true)));
//		ack->beginArray("input_streams");
//		for(llaDevice::IStreamIterator i = devices->getInputStreamIterator(); !i.end(); i++) {
//			ack->beginObject("");
//			ack->addElement("id", Message::DATA(i->getId()));
//			ack->addElement("name", Message::DATA(i->getName()));
//			ack->endObject();
//		}
//		ack->endArray();
//		ack->beginArray("output_streams");
//		for(llaDevice::OStreamIterator i = devices->getOutputStreamIterator(); !i.end(); i++) {
//			ack->beginObject("");
//			ack->addElement("id", Message::DATA(i->getId()));
//			ack->addElement("name", Message::DATA(i->getName()));
//			ack->endObject();
//		}
//		ack->endArray();
//		ack->endObject();
//	}
//
//	return ack;
//}

//Message* ACK_SET_INPUT_STREAM(TAlchemyString error) {
//	Message * ack = new Message(MSG_START);
//	ack->addElement("error", error);
//	return ack;
//}
//
//Message* ACK_SET_OUTPUT_STREAM(TAlchemyString error) {
//
//}

}
 /* namespace soundalchemy */


//	class Buffer:public llaudio::llaAudioBuffer {
//	public:
//		Buffer(TSize frames, DspProcess& parent ):llaAudioBuffer(frames), parent_(parent) {
//			stop_ = false;
//			ack_sent_ = false;
//		}
//		virtual ~Buffer() {}
//
//
//		virtual void onSamplesReady( void ) {
//			DspProcess* self = &parent_;
//			state_.lock();
//			switch (state_.state) { // evaluate state transition
//			case ST_PAUSED:
//				// will wait until a signal is emitted to continue
//				messagequeue_.pushBack(new Message(MSG_ACK));
//				//pthread_cond_wait(&self->wcond_, &self->wmutex_);
//				self->this_thread_->waitOn(self->state_);
//				break;
//			case ST_STOPPED:
//				// stop the processing thread
//				stop_= true;
//				ack_sent_ = false;
//				//pthread_mutex_unlock(&self->wmutex_);
//				self->state_.unlock();
//				break;
//			case ST_MODIFY_PROCESSING:
//				// do some changes in parameters then continue
//				// use lastmessage_ to obtain data
//				self->state_.safeSet( ST_RUNNING );
//				break;
//			case ST_RUNNING:
//				//do nothing
//
//				break;
//			}
//
//			if(!ack_sent_)
//				self->messagequeue_.pushBack(new Message(MSG_ACK));
//
//			ack_sent_ = true;
//			self->state_.unlock();
//
//			usleep(4000);
//		}
//
//		virtual bool stop(void) {
//
//			return stop_;
//		}
//
//	private:
//		DspProcess& parent_;
//		bool stop_;
//	public:
//
//		bool ack_sent_;
//	} buffer(512, *this);

//DspServer& self = parent_;
//	state_.lock();
//
//	llaDevice& dev = self.lla_devman_.getDevice("RP250");
//	if(dev.isNull()) {
//		log(LEVEL_ERROR, STR_ERRORS[E_AUDIO]);
//		self.messagequeue_.pushBack(new Message(MSG_ACK));
//		state_.state = ST_STOPPED;
//		state_.unlock();
//	}
//	else {
//		llaInputStream& input = dev.getInputStream();
//		input.setChannelCount(CH_STEREO);
//
//		llaOutputStream& output = dev.getOutputStream();
//		output.setChannelCount(CH_STEREO);
//
//		state_.unlock();
//		buffer.connectStreams(input, output);
//		if(!buffer.ack_sent_) {
//
//			state_.lock();
//			state_.state = ST_STOPPED;
//			log(LEVEL_ERROR, STR_ERRORS[E_AUDIO]);
//			self.messagequeue_.pushBack(new Message(MSG_ACK));
//
//			state_.unlock();
//		}
//	}

