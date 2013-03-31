/*
 * soundeffect.h
 *
 *  Created on: Mar 26, 2013
 *      Author: Mészáros Tamás
 */

#ifndef SOUNDEFFECT_H_
#define SOUNDEFFECT_H_
#include <map>
#include <string>

namespace soundalchemy {

class SoundEffect {
public:

	typedef unsigned long TParamID;
	typedef float TSample;
	typedef double TParamValue;
	typedef unsigned int TPortID;

	struct Param {
		Param(const char* name, TParamID id);
		std::string name;
		TParamID id;
		double value;
		void* valptr;
	};

	class Port {
		SoundEffect&  owner_;
		TSample *stream_;
	public:
		void setStream(TSample *stream) { stream_ = stream;}
		SoundEffect& getOwner(void) { return owner_; }
	protected:
		Port(SoundEffect& e):owner_(e){}
	};

	class OutputPort;
	class InputPort: public Port {
		OutputPort *source_;
	public:

		OutputPort* getSource(void) { return source_; }
		void setSource(OutputPort& source) { source_ = &source; }

		InputPort():Port(*this), source_(NULL){}
	};

	class OutputPort: public Port {
		InputPort *sink_;
	public:
		InputPort* getSink(void) { return sink_; }
		void setSink(InputPort& sink) { sink_ = &sink; }

		OutputPort():Port(*this), sink_(NULL){}
	};

	SoundEffect(unsigned int inputs, unsigned int outputs);
	virtual ~SoundEffect();

	TParamValue getParam(const char * name);
	TParamValue getParam(TParamID id);

	virtual void setParam(const char* name, TParamValue value);
	virtual void setParam(TParamID id, TParamValue value);

	unsigned int getInputsCount(void);
	unsigned int getOutputsCount(void);

	InputPort* getInputPort(TPortID index);
	OutputPort* getOutputPort(TPortID index);

	virtual void process(unsigned int sample_count) = 0;

protected:

	unsigned int in_channels_;
	unsigned int out_channels_;

	InputPort *inputs_;
	OutputPort *outputs_;

	typedef std::map<TParamID, Param*> TParamMap;
	typedef TParamMap::iterator TParamMapIt;

	TParamMap params_;

};

class LADSPAEffect: public SoundEffect {
public:
	void process(unsigned int sample_count);
};

class MixerEffect: public SoundEffect {
public:
	MixerEffect(unsigned int inputs, unsigned int outputs);

	void setInputsCount(unsigned int inputs);
	void setOutputsCount(unsigned int outputs);


	void process(unsigned int sample_count);
};

} /* namespace soundalchemy */
#endif /* SOUNDEFFECT_H_ */
