/*
 * Copyright (c) 2013 Mészáros Tamás.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v2.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
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
#include "effectdatabase.h"

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
	llaInputStream& getInputStream(void) { return effect_chain_.getInput(); }

	/**
	 *
	 * @return
	 */
	llaOutputStream& getOutputStream(void) {  return effect_chain_.getOutput(); }

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
	void addEffect(std::string effect_name);

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
	 * This class represents a separate audio processing thread. It inherits the
	 * llaAudioPipe interface as a private base to be able to overload specific
	 * call-back functions as the onSamplesReady() method in which the
	 * processing is taking place.
	 *
	 * The Runnable base class ensures that this class can be passed to a Thread
	 * object's run method.
	 */
	class DspProcess: public Runnable, private llaAudioPipe {
	public:

		/**
		 * Generic Interface for a Graph of audio filters. The DspProcess will
		 * process this graph when the startProcessing method is called.
		 * The implementation of the graph is not concerned.
		 */
		class ProcessingGraph {
		public:

			/**
			 * Setting the input buffer to the Graph.
			 * @param buffer A sample matrix with channels*samples dimensions.
			 * @param channels The count of audio channels.
			 */
			virtual void setInputBuffer(SoundEffect::TSample **buffer,
					unsigned int channels) = 0;

			/**
			 * Setting the output buffers where the final audio data will be
			 * copied.
			 * @param buffer A sample matrix with channels*samples dimensions.
			 * @param channels The count of audio channels.
			 */
			virtual void setOutputBuffer(SoundEffect::TSample **buffer,
					unsigned int channels) = 0;

			/**
			 * Retrieves the input llaudio stream.
			 * @return Returns a reference to an llaInputStream object.
			 */
			virtual llaInputStream& getInput(void) = 0;

			/**
			 * Retrieves the output llaudio stream.
			 * @return Returns a reference to an llaOutputStream object.
			 */
			virtual llaOutputStream& getOutput(void) = 0;

			/**
			 * Several real time audio plug-ins have to be activated before
			 * they are used for processing. This method has to activate all
			 * the plug-ins in the graph and called right before the processing
			 * is started.
			 */
			virtual void activate(void) = 0;

			/**
			 * The opposite of activate(). The method has to be called right
			 * after the processing is stopped.
			 */
			virtual void deactivate(void) = 0;

			/**
			 * This method will traverse the graph and call the process (or
			 * or similar) method of the filter.
			 * @param sample_count The count of samples which will be processed.
			 */
			virtual void traverse(unsigned int sample_count) = 0;

			virtual ~ProcessingGraph() {}
		};

		DspProcess(ProcessingGraph& graph);
		~DspProcess() {
			delete proc_thread_;
		}

		/**
		 * Retrieves the state of the processing. This method has to be
		 * surrounded with a lock() and an unlock() function call to avoid
		 * the threads to slap on each others foot.
		 * @return Returns the state of processing.
		 */
		TProcessingState getState(void) { return (TProcessingState) state_.val; }

		/**
		 * The overloaded method of the Runnable base class.
		 */
		void* run(void);

		/**
		 * This method will start the processing in a separate thred of execution.
		 * @return Returns an error code from TAlchemyError or E_OK if the
		 * processing started without problem. The function will block for a
		 * short period of time to wait for a response from the processing thread.
		 */
		TAlchemyError startProcessing(void);

		void setBufferSize(unsigned int frames) {
			llaAudioPipe::setBufferLength(frames);
		}

		/**
		 * Stops the processing.
		 */
		void stopProcessing( void );


		void lock() { state_.lock(); }
		void unlock() { state_.unlock(); }


	private:

		/*
		 * Overloaded from llaAudioPipe. This method is where the processing is
		 * done.
		 */
		void onSamplesReady(void);

		// Overloaded. Returns true if the processing has to stop. Triggered by
		// stopProcessing() method.
		bool stop(void);

		// waits for the proc. thread for a response.
		void waitFor() { proc_thread_->waitOn(state_); };

		// the processing graph
		ProcessingGraph& graph_;

		// the thread object which hosts the processing.
		Thread *proc_thread_;

		// Synchronization facility for the state of the processing
		class State: public ConditionVariable {
			Thread *thread_;
		public:
			State(Thread* th): thread_(th) {}
			TProcessingState val;
			TProcessingState state_requested_;
			bool isTrue(void) { return thread_->isRunning(); }
		} state_;

		// counter for the onSamplesReady method. If called
		// several times successfully than the processing probably runs fine.
		// and ready to send a response to the caller of startProcessing.
		unsigned int callback_counter_;

	} dsp_process_;

	/**
	 * @brief A class implementing a simple processing graph.
	 *
	 * This is a very simple implementation of an effect chain. The input is
	 * strictly mono and the output is a stereo signal. Effects are stacked in
	 * a list of SoundEffect objects and their audio ports are connected on
	 * addition by the addEffect method.
	 */
	class EffectChain : public DspProcess::ProcessingGraph {

		// typedef for the list data structure which is an stl vector for a
		// constant complexity of reaching the effects.
		typedef std::vector<SoundEffect*> TEffectStack;

		// Iterator for the list.
		typedef TEffectStack::iterator TEffectStackIt;

		// This type is used as an index of an effect in the effect list.
		typedef SoundEffect::TEffectID TEffectID;

		// The input is a mixer effect with an llaudio input stream
		// representing the output interface.
		class Input: public MixerEffect {
		public:
			Input();

			llaInputStream* llainput;
		} input_;

		// The output is a MixerEffect with an llaudio output stream.
		class Output: public MixerEffect {
		public:
			Output();

			llaOutputStream* llaoutput;
		} output_;

		// the actual effect list
		TEffectStack effectstack_;

		// A mutex for synchronizing parameter changes an effect addition if
		// the processing is in progress.
		Mutex *mutex_;

		// The sample rate used in the processing.
		TSampleRate sample_rate_;

		// A switch for bypass.
		bool bypassed_;

		// Returns a SoundEffect object by the ID. ID 0 is the input effect
		SoundEffect* getEffectById(TEffectID id);

		// Set the value of the effect parameter by the parameter's ID or name
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

		// Same for getting the parameter's value.
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

		EffectChain();
		~EffectChain() { delete mutex_; }

		/// See the ProcessingGraph class for more description.
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
		TAlchemyError addEffect(SoundEffect* effect, int position = -1 );

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

	} effect_chain_;

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

	// The effect database
	static EffectDatabase* database_;

	// the unique name of the sound device used for processing
	const char* device_name_;
	bool exit_;

};

} /* namespace soundalchemy */
#endif /* DSPSERVER_H_ */
