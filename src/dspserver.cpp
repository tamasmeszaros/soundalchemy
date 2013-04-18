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
#include "ladspaeffect.h"



using namespace std;

namespace soundalchemy {


// a class for the llaDeviceManager which implements an llaErrorHandler interface
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

// Instantiation of the device manager
llaDeviceManager& DspServer::lla_devman_ = llaDeviceManager::getInstance(llaEH);
EffectDatabase* DspServer::database_ = EffectDatabase::buildDatabase();


// DspServer Constructor
DspServer::DspServer():
		dsp_process_(effect_chain_),
		effect_chain_(),
		clients_count_(0),
		messagequeue_(NULL)
		 {

	// initialize all client connectors to NULL
	for (int i = 0; i < CLIENTS_MAX; i++)
		clients_[i] = NULL;

	// allocate a thread object for this thread
	this_thread_ = Thread::getCurrent();
}



// DspServer Destructor
DspServer::~DspServer() {

	// stop the processing
	stop();

	delete this_thread_;
	delete messagequeue_;

	// destroy the llaudio device manager
	try {
		lla_devman_.destroy();
	} catch (llaErrorHandler::Exception& ex) {
		log(LEVEL_ERROR, ex.what());
	}

	delete database_;
}

void DspServer::stop() {
	return dsp_process_.stopProcessing();
}

TAlchemyError DspServer::start() {
	return dsp_process_.startProcessing();
}

// The processMessage method queues up the message for further processing
void DspServer::processMessage(InboundMessage& msg) {
	if(messagequeue_ != NULL) messagequeue_->pushBack(&msg);
}

// Connects a ClientConnector type object with the server
TAlchemyError DspServer::listenOn(ClientConnector& client) {

	// the connection on the server side
	clients_[clients_count_] = &client;

	// make the connection on the client side
	client.connect(*this);

	// start the client connector to read its input
	TAlchemyError ret = clients_[clients_count_]->watch();


	if (ret != E_OK) {
		log(LEVEL_ERROR,
				"%s. Command prompt controller cannot be established.",
				STR_ERRORS[E_START_LISTENER]);
	}
	else {
		clients_count_++;

		// prepare a message with the new address to the client
		OutboundMessage* msgclientid =
				OutboundMessage::MsgSendClientID(clients_count_);

		// set up the address of the client in the clientconnector
		client.setChannelId(clients_count_);

		// send the new address to the client and delete the message
		client.send( *msgclientid );
		delete msgclientid;
	}

	return ret;
}

// collect the information about the audio devices
void DspServer::getDeviceList(AudioInf& devicelist, bool redetect) {
	if(redetect) lla_devman_.refresh();
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

// this method starts the processing of the messages from various clients
void DspServer::startListening(void) {

	InboundMessage *instruction;
	OutboundMessage *reply;
	exit_ = false;

	// allocating the message queue
	if(messagequeue_ == NULL) messagequeue_ = new MessageQueue();

	while(!exit_) {

		// this call blocks if the queue is empty
		// allowing a passive waiting for a message and process it while
		// other messages are buffered in the queue.
		instruction = messagequeue_->popFront();
		if(instruction != NULL) {
			reply = instruction->instruct(*this);
			broadcastMessage(*reply);
			delete instruction;
		}
	}
}

// Shut down a client connector. This means that messages from this client will
// be no longer processed. This method is called when a Message is obtained from
// a particular client which signals the end of its connection.
void DspServer::clientOut(Message::TChannelID client) {
	if( client > clients_count_ ) {
		log(LEVEL_ERROR, STR_ERRORS[soundalchemy::E_INDEX]);
		return;
	}
	else {
		unsigned int index = client - 1;
		clients_[index]->stopWatching();
		//delete clients_[client];
		clients_[index] = NULL;
		clients_count_--;
		if(clients_count_ == 0) stopListening();
	}
}

// Stops the processing of messages. This method is triggered by an Exit message
// from a client.
void DspServer::stopListening(void) {
	stop();
	exit_ = true;
}

TAlchemyError DspServer::setInputStream(TDeviceId device, TStreamId id) {
	llaDevice& dev = lla_devman_.getDevice(device );
	if(dev.isNull()) return E_SET_STREAM;

	llaInputStream& is = dev.getInputStream(id);
	if(is.isNull()) return E_SET_STREAM;

	effect_chain_.setInput(is);

	return E_OK;
}

TProcessingState DspServer::getState(void) {
	TProcessingState tmp;
	dsp_process_.lock();
	tmp = dsp_process_.getState();
	dsp_process_.unlock();
	return tmp;
}

TAlchemyError DspServer::setOutputStream(TDeviceId device, TStreamId id) {
	llaDevice& dev = lla_devman_.getDevice(device );
	if(dev.isNull()) return E_SET_STREAM;

	llaOutputStream& os = dev.getOutputStream(id);
	if(os.isNull()) return E_SET_STREAM;

	effect_chain_.setOutput(os);

	return E_OK;
}


void DspServer::setBufferSize(TSize buffer_size) {
}

void DspServer::setSampleRate(TSampleRate sample_rate) {
}

void DspServer::addEffect(SoundEffect* effect) {
}

void DspServer::broadcastMessage(OutboundMessage& message) {
	for (int i = 0; i < CLIENTS_MAX; i++) {
		if (clients_[i] != NULL)
			clients_[i]->send(message);
	}
}

// DspProcess //////////////////////////////////////////////////////////////////
//

// Constructor of the DspProcess class
DspServer::DspProcess::DspProcess(ProcessingGraph& graph) :
		graph_(graph), proc_thread_(Thread::getNewThread()),
		state_(proc_thread_) {
	callback_counter_ = 0;
	state_.val = ST_STOPPED;
	state_.state_requested_ = ST_STOPPED;

	// try to set real time priority to the processing thread
	proc_thread_->setRealtime();

	getInputBuffer().channelsRequested = CH_MONO;
	getOutputBuffer().channelsRequested = CH_STEREO;
}

// The code which is executed on the start of the processing thread
void* DspServer::DspProcess::run() {

	lock();
	state_.state_requested_ = ST_RUNNING;
	unlock();
	TErrors e = llaudio::E_OK;
	TProcessingState st;

	graph_.activate();

	// This thread will consume most of its life in the connectStream function
	// in which the processing is done. It calls the onSamplesReady method
	// whenever samples are ready to be processed
	e = connectStreams(graph_.getInput(), graph_.getOutput());

	graph_.deactivate();

	// while the processing has ran the requested state could be changed
	// but if all went right this has to be ST_STOPPED
	lock();
	st = state_.state_requested_;


	// set the actual state to ST_STOPPED
	state_.val = (TProcessingState)((ST_STOPPED));

	// If the  requested state is not equal to ST_STOPPED than the processing
	// stopped due to an error.
	if (e != llaudio::E_OK || st != ST_STOPPED) {
		unlock();
		proc_thread_->wakeUp();
		log(LEVEL_ERROR, "Alchemy server stopped unexpectedly");
	}
	else unlock();

	return NULL;
}

// this method starts the processing of the audio signal
TAlchemyError DspServer::DspProcess::startProcessing(void) {
	TAlchemyError retval = E_OK;
	lock();
	TAlchemyError ret;
	switch (state_.val) {
	case ST_STOPPED:
		ret = proc_thread_->run(*this);
		if (ret == E_THREAD) {
			retval = E_START;
			log(LEVEL_ERROR, STR_ERRORS[retval]);
			unlock();
			return retval;
		}

		waitFor();

		if (state_.val != ST_RUNNING) {
			log(LEVEL_ERROR, STR_ERRORS[E_START]);
			retval = E_START;
		}
		break;
	default:
		break;
	}

	unlock();

	return retval;
}

// stop the processing
void DspServer::DspProcess::stopProcessing(void) {
	lock();
	state_.state_requested_ = ST_STOPPED;
	unlock();
	if(proc_thread_->isRunning()) proc_thread_->join();
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

void DspServer::DspProcess::onSamplesReady(void) {
	lock();
	if (state_.state_requested_ == ST_RUNNING) {
		state_.val = ST_RUNNING;
		unlock();
		// Signal the control process waiting for the result of startProcessing
		// call that the processing goes well and it can continue broadcasting
		// this fact to clients
		if (callback_counter_ == 2)
			proc_thread_->wakeUp();
	} else {
		unlock();
		return;
	}

#ifdef DEBUG
	if(getInputBuffer().getChannels() == CH_NONE ||
			getOutputBuffer().getChannels() == CH_NONE ) {
		fail_state_ = true;
		return;
	}
#endif


//	float** o_samples = getOutputBuffer().getSamples();
//	float** i_samples = getInputBuffer().getSamples();
//
//	graph_.setInputBuffer(i_samples, getInputBuffer().getChannels());
//	graph_.setOutputBuffer(o_samples, getOutputBuffer().getChannels());
//
//	graph_.traverse(this->lastwrite_);
//
//	//usleep(8000);
//	getOutputBuffer().writeSamples();

	callback_counter_++;
}

// Processing Graph ////////////////////////////////////////////////////////////
//
DspServer::EffectChain::Input::Input() : MixerEffect("input"),
		llainput(&(lla_devman_.getDevice("RP250").getInputStream())) {

	// the channel count of the audio interface can be changed when it's
	// opened so this is just a guess of the actual channel count. The final
	// value is determined in the ProcessingGraph::setInputBuffer() method
	setInputsCount(llainput->getChannelCount());

	// the input is a mono signal
	addPort(new MixerPort(SoundEffect::OUTPUT_PORT, "output"));
}


DspServer::EffectChain::Output::Output() : MixerEffect("output"),
		llaoutput(&(lla_devman_.getDevice("RP250").getOutputStream())) {

	// the channel count of the audio interface can be changed when it is
	// opened so this is just a guess of the actual channel count. The final
	// value is determined in the ProcessingGraph::setOutputBuffer() method
	setOutputsCount(llaoutput->getChannelCount());

	// create the 2 output ports as the output will be stereo and
	// the final output to the audio interface will be mixed to the appropriate
	// channel count that is supported by the interface
	MixerPort* pl = new MixerPort(SoundEffect::INPUT_PORT, "left");
	MixerPort* pr = new MixerPort(SoundEffect::INPUT_PORT, "right");
	addPort(pl);
	addPort(pr);
}

DspServer::EffectChain::EffectChain() :
		input_(), output_(), mutex_(Thread::getMutex()),
		sample_rate_(SR_CD_QUALITY_44100), bypassed_(true)
		 {


	output_.setInputsCount(input_.getOutputsCount());

//	SoundEffect *e[] = {
//
//			database_->getEffect("mono_phaser", sample_rate_),
//			database_->getEffect("caps_amp", sample_rate_),
//			database_->getEffect("caps_cabinet", sample_rate_),
//			database_->getEffect("plate_reverb", sample_rate_),
//
//	};
//
//	e[1]->getParam("tonestack")->setValue(3.0f);
//	e[1]->getParam("gain")->setValue(0.80f);
//	e[1]->getParam("treble")->setValue(1.0f);
//	e[1]->getParam("mid")->setValue(0.5f);
//	e[2]->getParam("model")->setValue(1.0f);
//////	e[0]->getParam("mode")->setValue(3);
//////	e[0]->getParam("gain (dB)")->setValue(-24.0f);
//////	e[0]->getParam("bias")->setValue(0.0f);
////////	addEffect(e[1],-1);
//	addEffect(e[0],-1);
//	addEffect(e[1],-1);
//	addEffect(e[2],-1);
//	addEffect(e[3],-1);
}

void DspServer::EffectChain::setSampleRate() {
	// TODO set the sample rate off all effects in a for loop
}

TAlchemyError
DspServer::EffectChain::addEffect(SoundEffect* effect, int position ) {
	bool add_after = position < 0;

	unsigned int index;
	if(effectstack_.empty()) index = 0;
	else index = add_after ? effectstack_.size() + position : position;

	SoundEffect *effect_before = index == 0 ? &input_ : effectstack_[index];
	SoundEffect *effect_after = NULL;

	if(effectstack_.empty()) {
		effect_after = &output_;
	}
	else if (add_after) effect_after = index == effectstack_.size()-1 ? &output_ : effectstack_[index+1];
	else effect_after = effectstack_[index];


	if( effect->getInputsCount() != effect_before->getOutputsCount() ) {
		return soundalchemy::E_PORTS_INCOMPATIBLE;
	}

	if( effect_after == &output_ ) {
		if ( effect->getOutputsCount() <= 2)
			output_.setInputsCount(effect->getOutputsCount());
		else return soundalchemy::E_PORTS_INCOMPATIBLE;
	} else if( effect->getOutputsCount() != effect_after->getInputsCount() ) {
		return soundalchemy::E_PORTS_INCOMPATIBLE;
	}

	mutex_->lock();

	// inserting the effect to the effect stack. This is a painful operation
	// but effect addition is assumed to be less frequent
	//bool found = false;
	TEffectStackIt pos = effectstack_.begin();
	for(; pos != effectstack_.end() && *pos != effect_after; pos++) ;

	effectstack_.insert(pos, effect);

	mutex_->unlock();
	return E_OK;
}

void DspServer::EffectChain::removeEffect(SoundEffect::TEffectID id) {

}

void DspServer::EffectChain::bypass(void) {
	mutex_->lock();
	bypassed_ = !bypassed_;
	mutex_->unlock();
}

void DspServer::EffectChain::traverse(unsigned int sample_count) {
	mutex_->lock();

	SoundEffect *previous = &input_;
	if(!bypassed_) {

		// The host has to ensure that the output_ effect has min 2 allocated
		// output ports
		input_.getOutputPort(0)->connect(*(output_.getOutputPort(0)));
		input_.getMutex()->lock();
		input_.process(sample_count);
		input_.getMutex()->unlock();

		for (TEffectStackIt it = effectstack_.begin(); it != effectstack_.end();
				it++) {
			for(unsigned int p = 0; p < (*it)->getInputsCount(); p++) {
				(*it)->getInputPort(p)->connect(*(previous->getOutputPort(p)));
			}

			(*it)->getOutputPort(0)->connect(*(previous->getInputPort(0)));
			if((*it)->getOutputsCount() == 2) {
				if(previous->getInputsCount() == 1 ) {
					// previous effect had 1 input. A buffer from the output
					// port of the output effect will be used
					(*it)->getOutputPort(1)->connect(*(output_.getOutputPort(1)));
				}
				else {
					(*it)->getOutputPort(1)->connect(*(previous->getInputPort(1)));
				}
			}

			(*it)->getMutex()->lock();
			(*it)->process(sample_count);
			(*it)->getMutex()->unlock();
			previous = (*it);
		}

		for(unsigned int p = 0; p < output_.getInputsCount(); p++) {
			output_.getInputPort(p)->connect(*(previous->getOutputPort(p)));
		}

	} else {
		// don't use the input mixer effect. Connect all input ports of the input
		// effect to the inputs of the output mixer and go
		output_.setInputsCount(input_.getInputsCount());
		for(unsigned int p = 0; p < output_.getInputsCount(); p++) {
			output_.getInputPort(p)->connect(*(input_.getInputPort(p)));
		}

	}

	output_.getMutex()->lock();
	output_.process(sample_count);
	output_.getMutex()->unlock();
	mutex_->unlock();
}

void DspServer::EffectChain::setInputBuffer(SoundEffect::TSample** buffer,
		unsigned int channels) {

	input_.setInputsCount(channels);

	for(unsigned int c = 0; c < channels; c++) {
		MixerEffect::MixerPort p(SoundEffect::INPUT_PORT, "", buffer[c]);
		input_.getInputPort(c)->connect(p);
	}

}

void DspServer::EffectChain::setOutputBuffer(SoundEffect::TSample **buffer,
		unsigned int channels) {

	output_.setOutputsCount(channels);

	for(unsigned int c = 0; c < channels; c++) {
		MixerEffect::MixerPort p(SoundEffect::OUTPUT_PORT, "", buffer[c]);
		output_.getOutputPort(c)->connect(p);
	}

}

SoundEffect* DspServer::EffectChain::getEffectById(SoundEffect::TEffectID id) {
	SoundEffect *effect = NULL;

	if(id > effectstack_.size()) {
		log(LEVEL_WARNING, "%: %", STR_ERRORS[soundalchemy::E_INDEX],
				"No effect with the given index");
	}
	else if ( id == 0 ) {
		effect = &input_;
	} else if( id == effectstack_.size()) {
		effect = &output_;
	}
	else effect = effectstack_[id-1];

	return effect;
}

void DspServer::EffectChain::setEffectParam(SoundEffect::TEffectID id,
		std::string param, SoundEffect::TParamValue value) {
	setEffectParam<std::string>(id, param, value);
}

void DspServer::EffectChain::setEffectParam(SoundEffect::TEffectID id,
		SoundEffect::TParamID param, SoundEffect::TParamValue value) {
	setEffectParam<SoundEffect::TParamID>(id, param, value);
}

SoundEffect::TParamValue
DspServer::EffectChain::getEffectParam(SoundEffect::TEffectID id,
		SoundEffect::TParamID param) {
	return getEffectParam<SoundEffect::TParamID>(id, param);
}

SoundEffect::TParamValue
DspServer::EffectChain::getEffectParam(SoundEffect::TEffectID id,
				std::string param_name) {
	return getEffectParam<std::string>(id, param_name);
}

void DspServer::EffectChain::activate(void) {
	for(TEffectStackIt it = effectstack_.begin(); it != effectstack_.end(); it++) {
		(*it)->activate();
	}
}

void DspServer::EffectChain::deactivate(void) {
	for(TEffectStackIt it = effectstack_.begin(); it != effectstack_.end(); it++) {
		(*it)->deactivate();
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
	while (!cond_var_.queue_base_.empty()) {
		temp = cond_var_.queue_base_.front();
		delete temp;
		cond_var_.queue_base_.pop();
	}
	delete writemutex_;
	delete monitor_;
}

InboundMessage* DspServer::MessageQueue::popFront(void) {
	if (!enabled_)
		return NULL;

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
	if (!enabled_)
		return;

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
	if (!enabled_)
		return 0;

	unsigned int size;
	cond_var_.lock();
	writemutex_->lock();
	size = cond_var_.queue_base_.size();
	writemutex_->unlock();
	cond_var_.unlock();
	return size;
}
bool DspServer::MessageQueue::isEmpty(void) {
	if (!enabled_)
		return true;

	bool empty;
	cond_var_.lock();
	writemutex_->lock();
	empty = cond_var_.queue_base_.empty();
	writemutex_->unlock();
	cond_var_.unlock();
	return empty;
}


}


