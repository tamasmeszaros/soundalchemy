/*
 * soundeffect.cpp
 *
 *  Created on: Mar 26, 2013
 *      Author: Mészáros Tamás
 */

#include "soundeffect.h"
#include "logs.h"

namespace soundalchemy {

SoundEffect::SoundEffect(unsigned int inputs, unsigned int outputs):
		unique_id_(generateUniqueID()), in_channels_(inputs), out_channels_(outputs){

	inputs_ = new InputPort[in_channels_];
	outputs_ = new OutputPort[in_channels_];
}

SoundEffect::TEffectID SoundEffect::generateUniqueID(void) {
	static TEffectID id = 0;
	TEffectID ret = id;
	id++;
	return ret;
}

//void SoundEffect::setInputChannels(unsigned int count) {
////	inputs_ = (InputPort*) ::operator new[] (sizeof(InputPort)*count);
////	for(int i = 0; i < count; i++ ) inputs_[i] = new InputPort(*this);
//
//}
//
//void SoundEffect::setOutputChannels(unsigned int count) {
//
//}

SoundEffect::~SoundEffect() {
	delete [] inputs_;
	delete [] outputs_;

	for( TParamMapIt it = params_.begin(); it != params_.end(); it++)
		delete it->second;
}



soundalchemy::SoundEffect::Param::Param(const char* name, TParamID id) {
	this->id = id;
	this->name = name;
	this->value = 0.0;
}

SoundEffect::TParamValue SoundEffect::getParam(const char* name) {
	for(TParamMapIt it = params_.begin(); it != params_.end(); it++) {
		if(!it->second->name.compare(name)) return it->second->value;
	}

	log(LEVEL_ERROR, STR_ERRORS[E_INDEX]);
	return 0.0;
}

SoundEffect::TParamValue SoundEffect::getParam(TParamID id) {
	TParamMapIt p = params_.find(id);
	if(p != params_.end()) return p->second->value;

	log(LEVEL_ERROR, STR_ERRORS[E_INDEX]);
	return 0.0;
}

void SoundEffect::setParam(const char* name, TParamValue value) {
}

void SoundEffect::setParam(TParamID id, TParamValue value) {
}

unsigned int SoundEffect::getInputsCount(void) {
	return in_channels_;
}

unsigned int SoundEffect::getOutputsCount(void) {
	return out_channels_;
}

SoundEffect::InputPort * SoundEffect::getInputPort(TPortID index) {
	if(index <= in_channels_ || inputs_ == NULL) {
		log(LEVEL_ERROR, STR_ERRORS[E_INDEX]);
		return NULL;
	}
	return &inputs_[index];
}

SoundEffect::OutputPort * SoundEffect::getOutputPort(TPortID index) {
	if(index <= out_channels_ || outputs_ == NULL) {
		log(LEVEL_ERROR, STR_ERRORS[E_INDEX]);
		return NULL;
	}
	return &outputs_[index];
}

void MixerEffect::setOutputsCount(unsigned int outputs) {
	out_channels_ = outputs;
	delete [] outputs_;
	outputs_ = new OutputPort[out_channels_];

}

void MixerEffect::setInputsCount(unsigned int inputs) {
	in_channels_ = inputs;
	inputs_ = new InputPort[in_channels_];
	delete [] inputs_;
}

MixerEffect::MixerEffect(unsigned int inputs, unsigned int outputs):
		SoundEffect(inputs, outputs) {
	Param *p = new Param("volume", 0);
	p->value = 1.0;
	params_[0] = p;
}

void MixerEffect::process(unsigned int sample_count) {

}

}



 /* namespace soundalchemy */
