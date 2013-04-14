/*
 * soundeffect.cpp
 *
 *  Created on: Mar 26, 2013
 *      Author: Mészáros Tamás
 */

#include "soundeffect.h"
#include "logs.h"
#include <cstring>

namespace soundalchemy {

SoundEffect::SoundEffect(llaudio::TSampleRate sample_rate, std::string name):
		name_(name), sample_rate_(sample_rate), mutex_(Thread::getMutex()){}

//SoundEffect::TEffectID SoundEffect::generateUniqueID(void) {
//	static TEffectID id = 0;
//	TEffectID ret = id;
//	id++;
//	return ret;
//}

SoundEffect::~SoundEffect() {
	for(TParamVector::iterator p = params_.begin(); p != params_.end(); p++) {
		delete *p;
	}

	for(TPortVector::iterator p = inputs_.begin(); p != inputs_.end(); p++) {
		delete *p;
	}

	for(TPortVector::iterator p = outputs_.begin(); p != outputs_.end(); p++) {
		delete *p;
	}

	delete mutex_;

}

void SoundEffect::addParam(Param *param) {
	if( param != NULL ) {
		param->id_ = params_.size();
		params_.push_back(param);
	}
}

void SoundEffect::addPort(Port *port) {
	if(port != NULL ) {
		if(port->getDirection() == INPUT_PORT) inputs_.push_back(port);
		else outputs_.push_back(port);
	}
}

SoundEffect::Param* SoundEffect::getParam(std::string name) {
	for(TParamVector::iterator p = params_.begin(); p != params_.end(); p++) {
		if(! (*p)->getName().compare(name)) return *p;
	}

	log(LEVEL_ERROR, STR_ERRORS[E_INDEX]);
	return NULL;
}

SoundEffect::Param* SoundEffect::getParam(TParamID id) {
	if(id >= params_.size()) {
		log(LEVEL_ERROR, STR_ERRORS[E_INDEX]);
		return NULL;
	}

	return params_[id];
}

unsigned int SoundEffect::getInputsCount(void) {
	return inputs_.size();
}

unsigned int SoundEffect::getOutputsCount(void) {
	return outputs_.size();
}

SoundEffect::Port * SoundEffect::getInputPort(TPortID index) {
	if(index >= inputs_.size() ) {
		log(LEVEL_ERROR, STR_ERRORS[E_INDEX]);
		return NULL;
	}
	return inputs_[index];
}

SoundEffect::Port * SoundEffect::getOutputPort(TPortID index) {
	if(index >= outputs_.size() ) {
		log(LEVEL_ERROR, STR_ERRORS[E_INDEX]);
		return NULL;
	}
	return outputs_[index];
}

MixerEffect::MixerEffect(std::string name):SoundEffect(llaudio::SR_DEFAULT, name) {

	Param *p = new MixerParam( "volume");
	p->setValue(1.0);
	addParam(p);
}

void MixerEffect::setInputsCount(unsigned int count) {

	// do nothing if the count is already set
	if( inputs_.size() == count ) return;

	for(TPortVector::iterator it = inputs_.begin(); it != inputs_.end(); it++ ) {
		delete (*it);
	}
	inputs_.clear();
	for ( unsigned int i = 0; i < count; i++ ) {
		addPort(new MixerPort(SoundEffect::INPUT_PORT, ""));
	}
}

void MixerEffect::setOutputsCount(unsigned int count) {

	// do nothing if the count is already set
	if( inputs_.size() == count ) return;

	for(TPortVector::iterator it = outputs_.begin(); it != outputs_.end(); it++ ) {
		delete (*it);
	}
	outputs_.clear();
	for ( unsigned int i = 0; i < count; i++ ) {
		addPort(new MixerPort(SoundEffect::OUTPUT_PORT, ""));
	}
}

void MixerEffect::process(unsigned int sample_count) {


	if( getInputsCount() == getOutputsCount() ) {
		// simply copy the buffer out with volume adjustment
		for(unsigned int j = 0; j < getInputsCount(); j++) {
			for(unsigned int i = 0; i < sample_count; i++) {
				getOutputPort(j)->getBuffer() [i] = getInputPort(j)->getBuffer() [i] * getParam(0)->getValue();
			}
		}
	}
	else if( getInputsCount() > getOutputsCount() ) {
		// downmix
		for(unsigned int j = 0; j < getOutputsCount(); j++) {
			for(unsigned int i = 0; i < sample_count; i++) {
				// frame for channel A: frA
				// frame for channel B: frB (next channel after A)
				// frA = Volume*(frA+frB)/2
				TSample * inbuff1 = getInputPort(j)->getBuffer();
				TSample * inbuff2 = getInputPort(j+1)->getBuffer();
				TSample * outbuff = getOutputPort(j)->getBuffer();
				//outbuff [i] = 0.5 * (inbuff1 [i]);
				outbuff [i] = getParam(0)->getValue() * ((inbuff1 [i]) + (inbuff2 [i]))*0.5 ;
			}
		}
	}
	else {
		// upmix
		for(unsigned int j = 0; j < getOutputsCount(); j++) {
			for(unsigned int i = 0; i < sample_count; i++) {
				//simply copy the first channel to all outputs
				getOutputPort(j)->getBuffer() [i] =
						getParam(0)->getValue() * (getInputPort(0)->getBuffer() [i]);
			}
		}
	}
}

} /* namespace soundalchemy */
