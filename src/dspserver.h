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
	 * @brief The server's state is in one of these states
	 */
	typedef enum e_states
	{
		ST_STOPPED,         //!< The processing thread is completely killed.
		//ST_PAUSED,          //!< The processing pipe (thread) is waiting.
		ST_RUNNING,         //!< Processing is in progress.
	} TState;

	/**
	 * default constructor and destructor
	 */
	DspServer();
	virtual ~DspServer();

	/**
	 * Starts the server and the audio thread
	 * @return Returns an an error code defined in enum e_errors
	 */
	TAlchemyError start(void);

	/**
	 * Stops the audio process and the processing thread exits
	 */
	void stop(void);

	void getDeviceList(AudioInf& devicelist);

	TAlchemyError setInputStream(TDeviceId device, TStreamId id);

	TAlchemyError setOutputStream(TDeviceId device, TStreamId id);

	llaInputStream& getInputStream(void) { return proc_graph_.getInput(); }

	llaOutputStream& getOutputStream(void) {  return proc_graph_.getOutput(); }

	void setBufferSize(TSize buffer_size);

	void setSampleRate(TSampleRate sample_rate);


	void addEffect(SoundEffect* effect);

	/**
	 * Starts the command line listener.
	 * @return Returns an an error code defined in enum e_errors
	 */
	TAlchemyError listenOn(ClientConnector& client);

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


	class DspProcess: public Runnable, private llaAudioBuffer{
	public:
		DspProcess(DspServer& server);
		~DspProcess() {
			delete proc_thread_;
		}

		TState getState(void) { return (TState) state_.val; }
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
		DspServer& dspserver_;

		class State: public ConditionVariable {
		public:
			TState val;
			TState state_requested_;
			bool isTrue(void) { return val == ST_STOPPED; }
		} state_;

		unsigned int callback_counter_;

		//Mutex *proc_mutex_;

		Thread *proc_thread_;

	} dsp_process_;

	class Input: public MixerEffect {
	public:
		Input():MixerEffect(1,1), llainput(lla_devman_.getDefaultDevice().getInputStream()) { }

		llaInputStream& llainput;
	};

	class Output: public MixerEffect {
	public:
		Output(): MixerEffect(1,2), llaoutput(lla_devman_.getDefaultDevice().getOutputStream()) { }

		llaOutputStream& llaoutput;
	};

	class ProcessingGraph {
		typedef std::vector<SoundEffect*> TEffectStack;
		typedef TEffectStack::iterator TEffectStackIt;

		TEffectStack effectlist_;
		Mutex *mutex_;
		Input input_;
		Output output_;
	public:

		ProcessingGraph():mutex_(Thread::getMutex()){}
		~ProcessingGraph() { delete mutex_; }

		void setInput(llaInputStream& input) { input_.llainput = input; }
		void setOutput(llaOutputStream& output) { output_.llaoutput = output; }

		llaInputStream& getInput(void) { return input_.llainput; }
		llaOutputStream& getOutput(void) { return output_.llaoutput; }

		void traverse(unsigned int sample_count) {
			mutex_->lock();
			input_.process(sample_count);
			for(TEffectStackIt it = effectlist_.begin();
					it != effectlist_.end(); it++ ) {
				(*it)->process(sample_count);
			}
			output_.process(sample_count);
			mutex_->unlock();
		}

		void addEffect(SoundEffect* effect);
	} proc_graph_;


	/**
	 * @class MessageQueue
	 * @brief A queue class which can be used by multiple threads.
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
	};

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
