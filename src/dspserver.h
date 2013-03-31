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
	 * Clients can be connected to the process and the types of clients
	 * supported are listed here
	 */
	typedef enum e_clients
	{
		CLIENT_COMMAND_PROMPT,//!< a command line client connector
		CLIENT_MIDI,          //!< a MIDI controller handler
		CLIENT_OTHER          //!< CLIENT_OTHER
	} TClients;

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


	void addEffect(SoundEffect* effect);

	/**
	 * Starts the command line listener.
	 * @return Returns an an error code defined in enum e_errors
	 */
	TAlchemyError listenOnCliPrompt(void);

	/**
	 * Starts the MIDI listener. MIDI controller support is not implemented yet.
	 * @param interface
	 * @return Returns an an error code defined in enum e_errors
	 */
	TAlchemyError listenOnMidi(const char* interface);
	//enum e_errors listenOnSocket();

	/**
	 * Enables the android GUI support trough the stdin and stdout stream as
	 * the communication channel. IPC is problematic with the dalvik VM with
	 * classic POSIX facilities.
	 * @return Returns an an error code defined in enum e_errors
	 */
	TAlchemyError listenOnAndroidGUI(void);

	/**
	 * Starts the handling of user inputs from the various client connectors.
	 * Call this method after setting up client connectors and all resources are
	 * allocated. Normally at the end of main.
	 */
	void startListening(void);

	/**
	 * This function can be called by a client connector to stop the whole
	 * service.
	 */
	void stopListening(void) { exit_ = true; }

	/**
	 * This method is for client connectors for sending messages (instructions)
	 * to the server.
	 * @param msg Pointer to the message to be sent for processing.
	 */
	void processMessage(InboundMessage& msg);


private:

//	static Message* ACK_START(TChannelID chid, TAlchemyString error = "" );
//	static Message* ACK_STOP(TChannelID chid, TAlchemyString error = "");
//	static Message* ACK_GET_DEVICE_LIST( TChannelID chid, llaDeviceIterator& devices);
//	static Message* ACK_SET_INPUT_STREAM( TChannelID chid, TAlchemyString error);
//	static Message* ACK_SET_OUTPUT_STREAM( TChannelID chid, TAlchemyString error);

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
			sig_atomic_t val;
			bool isTrue(void) { return val == ST_STOPPED; }
		} state_;

		volatile sig_atomic_t state_requested_;
		unsigned int callback_counter_;

		//Mutex *proc_mutex_;

		Thread *proc_thread_;

	} dsp_process_;

	class EffectNode {
	public:
		SoundEffect* effect;
		EffectNode * next;
		EffectNode(): effect(NULL), next(NULL){}
	};

	class Input: public EffectNode {
	public:
		Input(): llainput(LLA_NULL_STREAM) { effect = new MixerEffect(1,1); }

		llaInputStream& llainput;
	}audio_input_;

	class Output: public EffectNode {
	public:
		Output(): llaoutput(LLA_NULL_STREAM) { effect = new MixerEffect(1,2); }

		llaOutputStream& llaoutput;
	}audio_output_;


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
	};

	Thread *this_thread_;

	// Objects for connecting different front-ends
	ClientConnector* clients_[CLIENTS_MAX];

	// A facility for communication between the UI and the server
	MessageQueue messagequeue_;

	// The audio interface manager
	llaDeviceManager& lla_devman_;

	// the unique name of the sound device used for processing
	const char* device_name_;
	bool remote_;
	bool exit_;

};




} /* namespace soundalchemy */
#endif /* DSPSERVER_H_ */
