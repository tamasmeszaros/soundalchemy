/*
 * soundeffect.h
 *
 *  Created on: Mar 26, 2013
 *      Author: Mészáros Tamás
 */

#ifndef SOUNDEFFECT_H_
#define SOUNDEFFECT_H_
#include <vector>
#include <string>
#include "llaudio/llaudio.h"
#include "thread.h"

namespace soundalchemy {

/**
 * Base class for an audio effect.
 */
class SoundEffect {
public:

	/// Identifier type for an effect parameter
	typedef unsigned long TParamID;

	/// fundamental type for an audio sample
	typedef float TSample;

	/// Representation of a parameter value
	typedef double TParamValue;

	/// Identifier of an audio port
	typedef unsigned int TPortID;

	/**
	 * Enum type for a port direction
	 */
	typedef enum {
		INPUT_PORT,//!< Input port
		OUTPUT_PORT//!< Output port
	} TPortDirection;

	/// Unique identifier of an effect instance
	typedef unsigned long TEffectID;

	/**
	 * abstract base class for an Effect parameter
	 */
	class Param {

		// an optional name for the parameter
		std::string name_;

		// An id for the parameter. For now it represents the index of this
		// parameter in the params_ array
		TParamID id_;

		// The ID is set by the SoundEffect object when the parameter is added
		// with the addParam() method
		friend class SoundEffect;

	public:
		/// constructor
		Param(std::string name = ""): name_(name) {}

		/// virtual destructor
		virtual ~Param() {}

		/// get the name
		std::string getName() { return name_; }

		/// get the id (index)
		TParamID getId() { return id_; }

		/**
		 *
		 * @return
		 */
		virtual TParamValue getValue() = 0;

		/**
		 *
		 * @param value
		 */
		virtual void setValue(TParamValue value) = 0;

	};

	class Port {

		TPortDirection dir_;
		std::string name_;
		TPortID id_;

		// The ID is set by the SoundEffect object when the port is added
		// with the addPort() method
		friend class SoundEffect;
	public:
		Port(TPortDirection dir, std::string name = ""): dir_(dir),
				name_(name) {}

		virtual ~Port() {}

		virtual void connect(Port& port) = 0;
		virtual TSample* getBuffer() = 0;

		std::string getName() { return name_; }
		TPortDirection getDirection() { return dir_; }
	};

	SoundEffect(llaudio::TSampleRate sample_rate, std::string name = "");

	virtual ~SoundEffect();

	Param* getParam(std::string name);
	Param* getParam(TParamID id);

	unsigned int getInputsCount(void);
	unsigned int getOutputsCount(void);
	unsigned int getParamsCount(void);

	Port* getInputPort(TPortID index);
	Port* getInputPort(std::string name);
	Port* getOutputPort(TPortID index);
	Port* getOutputPort(std::string name);

	//TEffectID getId() { return id_; }
	std::string getName(void) { return name_; }

	virtual void process(unsigned int sample_count) = 0;

	virtual void activate(void) = 0;
	virtual void deactivate(void) = 0;

	virtual void setSampleRate(llaudio::TSampleRate srate) {
		sample_rate_ = srate;
	}

	virtual llaudio::TSampleRate getSampleRate() { return sample_rate_; }

	//void setId(TEffectID index) { id_ = index; }

	Mutex* getMutex() { return mutex_; }

protected:

	void addParam(Param *param);
	void addPort(Port *port);

	//TEffectID id_;
	std::string name_;

	typedef std::vector<Port*> TPortVector;
	typedef std::vector<Param*> TParamVector;

	TPortVector inputs_;
	TPortVector outputs_;

	TParamVector params_;
	llaudio::TSampleRate sample_rate_;
	Mutex *mutex_;

};

class MixerEffect: public SoundEffect {
	class MixerParam: public Param {
		TParamValue value_;
	public:
		MixerParam( std::string name ): Param( name ) {}
		double getValue() { return value_; }
		void setValue(TParamValue value) { value_ = value; }
	};

public:

	class MixerPort: public Port {
		TSample *buffer_;
	public:
		MixerPort(TPortDirection dir, std::string name, TSample *buffer = NULL ):
			Port(dir, name), buffer_(buffer) {}
		virtual void connect(Port& port) { buffer_ = port.getBuffer(); }

		virtual TSample* getBuffer() { return buffer_; }
	};

	MixerEffect(std::string name = "");

	void process(unsigned int sample_count);

	virtual void activate(void) {};
	virtual void deactivate(void) {};

	void setInputsCount(unsigned int inputs);
	void setOutputsCount(unsigned int outputs);

};

} /* namespace soundalchemy */
#endif /* SOUNDEFFECT_H_ */
