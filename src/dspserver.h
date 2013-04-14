/*
 * dspserver.h
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
#ifndef DSPSERVER_H_
#define DSPSERVER_H_

#include "logs.h"
#include "message.h"
#include "clientconnector.h"
#include "thread.h"
#include "soundeffect.h"

#include <queue>
#include <signal.h>
#include "llaudio/llaudio.h"


using namespace llaudio;

namespace soundalchemy {

class ClientConnector;

/**
 * @class DspServer
 * @brief The class representing the DSP server itself
 * @details This class encapsulates the whole signal chain and a controlling
 * interface for it. The signal processing pipe is running in a separate thread
 * and the controlling interface runs in the instantiating thread parallel to it.
 */
class DspServer {
public:

	/**
	 * default constructor and destructor
	 */
	DspServer();
	virtual ~DspServer();

	/**
	 * Starts the audio processing
	 * @return Returns an an error code defined in enum e_errors
	 */
	TAlchemyError start(void);

	/**
	 * Stops the audio processing. The thread for the processing exits.
	 */
	void stop(void);

	/**
	 * Collects information about the known audio devices and streams.
	 * @param devicelist Target (empty) AudioInf type object.
	 * @param redetect Triggers a complete re-detection of audio devices if true.
	 */
	void getDeviceList(AudioInf& devicelist, bool redetect);

	/**
	 * Sets the input audio source of the processing graph.
	 * @param device The String ID of the audio device
	 * @param id The integer ID of the actual stream on the device
	 * @return Returns E_OK or an error constant from TAlchemyError on failure.
	 */
	TAlchemyError setInputStream(TDeviceId device, TStreamId id);

	/**
	 *
	 * @param device
	 * @param id
	 * @return
	 */
	TAlchemyError setOutputStream(TDeviceId device, TStreamId id);

	/**
	 *
	 * @return
	 */
	llaInputStream& getInputStream(void) { return proc_graph_.getInput(); }

	/**
	 *
	 * @return
	 */
	llaOutputStream& getOutputStream(void) {  return proc_graph_.getOutput(); }

	/**
	 *
	 * @param buffer_size
	 */
	void setBufferSize(TSize buffer_size);

	/**
	 *
	 * @param sample_rate
	 */
	void setSampleRate(TSampleRate sample_rate);

	/**
	 *
	 * @param effect
	 */
	void addEffect(SoundEffect* effect);

	/**
	 *
	 * @return
	 */
	TProcessingState getState(void);

	/**
	 * Starts the command line listener.
	 * @return Returns an an error code defined in enum e_errors
	 */
	TAlchemyError listenOn(ClientConnector& client);

	/**
	 *
	 * @param client
	 */
	void clientOut(Message::TChannelID client);

	/**
	 * This function can be called by a client connector to stop the whole
	 * service.
	 */
	void stopListening(void);

	/**
	 * Starts the handling of user inputs from the various client connectors.
	 * Call this method after setting up client connectors and all resources are
	 * allocated. Normally at the end of main.
	 */
	void startListening(void);

	/**
	 * This method is for client connectors for sending messages (instructions)
	 * to the server.
	 * @param msg Pointer to the message to be sent for processing.
	 */
	void processMessage(InboundMessage& msg);


private:

	/**
	 * Sending a message or reply to all the client connectors
	 * @param message
	 */
	void broadcastMessage(OutboundMessage& message);

	/**
	 * @brief Class encapsulating the processing graph.
	 *
	 * An object of this class is a map of audio effects connected together in
	 * a specific scheme. This is a very simple implementation of an effect
	 * chain. The input is strictly mono and the output is a stereo signal.
	 * Effects are stacked in a list of SoundEffect objects and their audio ports
	 * are connected on addition by the addEffect method. If there is a
	 * mono/stereo incompatibility then a MixerEffect is inserted which resolves
	 * it.
	 */
	class ProcessingGraph {
		typedef std::vector<SoundEffect*> TEffectStack;
		typedef TEffectStack::iterator TEffectStackIt;
		typedef SoundEffect::TEffectID TEffectID;

		class Input: public MixerEffect {
		public:
			Input();

			llaInputStream* llainput;
		} input_;

		class Output: public MixerEffect {
		public:
			Output();

			llaOutputStream* llaoutput;
		} output_;


		TEffectStack effectstack_;
		Mutex *mutex_;

		TSampleRate sample_rate_;
		bool bypassed_;

		SoundEffect* getEffectById(TEffectID id);

		template<class T>
		void setEffectParam(TEffectID id, T param,
				SoundEffect::TParamValue value) {

			SoundEffect *effect = getEffectById(id);
			if( effect == NULL ) return;

			SoundEffect::Param *p = effect->getParam(param);
			if (p) {
				effect->getMutex()->lock();
				p->setValue(value);
				effect->getMutex()->unlock();
			}
		}

		template<class T>
		SoundEffect::TParamValue getEffectParam(SoundEffect::TEffectID id,
						T param) {

			SoundEffect::TParamValue val = 0.0;
			SoundEffect *effect = getEffectById(id);
			if( effect == NULL ) return val;

			SoundEffect::Param *p = effect->getParam(param);
			if(p) {
				effect->getMutex()->lock();
				val = p->getValue();
				effect->getMutex()->unlock();
			}

			return val;
		}

	public:

		class Preset {

		};

		ProcessingGraph();
		~ProcessingGraph() { delete mutex_; }

		void setInput(llaInputStream& input) { input_.llainput = &input; }
		void setOutput(llaOutputStream& output) { output_.llaoutput = &output; }

		void setInputBuffer(SoundEffect::TSample **buffer, unsigned int channels);
		void setOutputBuffer(SoundEffect::TSample **buffer, unsigned int channels);

		void setSampleRate();
		TSampleRate getSampleRate() { return sample_rate_; }

		unsigned int getInputChannelsCount() { return input_.getInputsCount(); }
		unsigned int getOutputChannelsCount() { return output_.getOutputsCount(); }

		llaInputStream& getInput(void) { return *(input_.llainput); }
		llaOutputStream& getOutput(void) { return *(output_.llaoutput); }

		void traverse(unsigned int sample_count) ;

		// position 0 means insert at the beginning
		// position -1 is the end of the effect list
		TAlchemyError addEffect(SoundEffect* effect, int position );

		void removeEffect(TEffectID id);

		void setEffectParam(TEffectID id, std::string param,
				SoundEffect::TParamValue value);

		void setEffectParam(TEffectID id, SoundEffect::TParamID param_name,
						SoundEffect::TParamValue value);

		SoundEffect::TParamValue getEffectParam(TEffectID id,
				SoundEffect::TParamID param);

		SoundEffect::TParamValue getEffectParam(TEffectID id,
						std::string param_name);

		void bypass(void);

		void activate(void);
		void deactivate(void);

		void save(Preset& preset);
		void load(Preset& preset);

	} proc_graph_;

	/**
	 * This class represents the processing thread. It inherits the
	 * llaAudioBuffer interface as a private base be able to overload specific
	 * call-back functions as the onSamplesReady() method in which the
	 * processing is taking place.
	 */
	class DspProcess: public Runnable, private llaAudioPipe {
	public:
		DspProcess(ProcessingGraph& graph_);
		~DspProcess() {
			delete proc_thread_;
		}

		TProcessingState getState(void) { return (TProcessingState) state_.val; }
		void* run(void);

		TAlchemyError startProcessing(void);
		void stopProcessing( void);

		// overloaded
		void onSamplesReady(void);
		bool stop(void);

		void lock() { state_.lock(); }
		void unlock() { state_.unlock(); }


	private:
		void waitFor() { proc_thread_->waitOn(state_); };
		ProcessingGraph& graph_;

		Thread *proc_thread_;
		class State: public ConditionVariable {
			Thread *thread_;
		public:
			State(Thread* th): thread_(th) {}
			TProcessingState val;
			TProcessingState state_requested_;
			bool isTrue(void) { return thread_->isRunning(); }
		} state_;

		unsigned int callback_counter_;


	} dsp_process_;



	/**
	 * @class MessageQueue
	 * @brief A queue class which can be used by multiple threads. This type
	 * of queue is known as a blocking queue which blocks if the popFront()
	 * method is called for an empty queue until another process pushes an
	 * element in the queue.
	 */
	class MessageQueue {

	public:
		MessageQueue();
		~MessageQueue();

		/**
		 * @brief Pops out the first message and removes it from the queue
		 * @details If the queue is empty this operation will block the calling
		 * thread until another thread pushes a message at the back of the
		 * queue. The message obtained is a pointer to a Message class and after
		 * interpreting its content it has to be deleted properly with delete.
		 * @return Returns a pointer to soundalchemy::Message object.
		 */
		InboundMessage* popFront(void);

		/**
		 * @brief Pushes a message at the end of the queue.
		 * @param message Pointer to a soundalchemy::Message object.
		 */
		void pushBack(InboundMessage* message);

		/**
		 * @brief The number of messages in the queue.
		 * @return An unsigned integer representing the number of messages.
		 */
		unsigned int getSize(void);

		/**
		 *
		 * @return Returns true if the queue is false otherwise.
		 */
		bool isEmpty(void);


	private:

		Thread *monitor_;
		Mutex *writemutex_;

		unsigned int waiters_;
		typedef std::queue<InboundMessage*> TMessageQueue;

		class QCondition: public ConditionVariable {
		public:
			virtual bool isTrue(void) { return queue_base_.empty(); }

			TMessageQueue queue_base_;
		}cond_var_;

		bool enabled_;
	}; // end of MessageQueue

	Thread *this_thread_;

	// Objects for connecting different front-ends
	ClientConnector* clients_[CLIENTS_MAX];
	unsigned int clients_count_;

	// A facility for communication between the UI and the server
	MessageQueue *messagequeue_;

	// The audio interface manager
	static llaDeviceManager& lla_devman_;

	// the unique name of the sound device used for processing
	const char* device_name_;
	bool exit_;

};

} /* namespace soundalchemy */
#endif /* DSPSERVER_H_ */
