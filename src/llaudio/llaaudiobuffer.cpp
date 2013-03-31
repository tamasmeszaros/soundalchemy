/*
 * llaaudiobuffer.cpp
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

#include "llaaudiobuffer.h"
#include "llastream.h"
#include <stdint.h>

using namespace llaudio;

const llaAudioBuffer::TSampleFormat llaAudioBuffer::FORMAT_S16 = TSampleFormat(false, true, !isBigEndianArch(), 16);
const llaAudioBuffer::TSampleFormat llaAudioBuffer::FORMAT_S16LE = TSampleFormat(false, true, true, 16);
const llaAudioBuffer::TSampleFormat llaAudioBuffer::FORMAT_S24 = TSampleFormat(false, true, !isBigEndianArch(), 24);
const llaAudioBuffer::TSampleFormat llaAudioBuffer::FORMAT_S24LE = TSampleFormat(false, true, true, 24);
const llaAudioBuffer::TSampleFormat llaAudioBuffer::FORMAT_S32 = TSampleFormat(false, true, !isBigEndianArch(), 32);
const llaAudioBuffer::TSampleFormat llaAudioBuffer::FORMAT_S32LE = TSampleFormat(false, true, true, 24);
const llaAudioBuffer::TSampleFormat llaAudioBuffer::FORMAT_FLOAT = TSampleFormat(true, false, !isBigEndianArch(), 32);
const llaAudioBuffer::TSampleFormat llaAudioBuffer::FORMAT_FLOAT64 = TSampleFormat(true, false, !isBigEndianArch(), 64);
const llaAudioBuffer::TSampleFormat llaAudioBuffer::FORMAT_DEFAULT = FORMAT_FLOAT;

llaudio::llaAudioBuffer::llaAudioBuffer(TSize frames)  {

	buffer_alloced_ = false;
	rawfp_non_i_ = NULL;
	iraw_ = NULL;
	niraw_ = NULL;

	sampleorg_ = NON_INTERLEAVED;
	format_ = FORMAT_DEFAULT;
	channels_ = CH_DEFAULT;
	frames_ = frames;

	sampleorg_alloced_ = NON_INTERLEAVED;
	frames_alloced_ = 0;
	channels_alloced_ = CH_NONE;
	format_alloced_ = FORMAT_DEFAULT;

	lastwrite_ = 0;

	//setBufferLength(frames);

}

llaudio::llaAudioBuffer::~llaAudioBuffer() {
	clear();

	// TODO free the memory for rawfp_non_i_ and for raw buffers
	// if(rawfp_non_i_ != NULL) delete rawfp_non_i_;

}

float** llaudio::llaAudioBuffer::getSamples( void ) {

	unsigned int bytes = format_.sample_width/8;
	if(sizeof(float) < bytes ) return NULL;

	typedef char rawfloat[sizeof(float)];

	unsigned int j = 0;

	if(sampleorg_ == INTERLEAVED) {
		if(rawfp_non_i_ != NULL ) {
			for(TSize i = 0; i < channels_; i++) {
				delete [] rawfp_non_i_[i];
			}
			delete [] rawfp_non_i_;
		}

		rawfp_non_i_ = new float*[channels_];
		for(TSize i = 0; i < channels_; i++) {
			rawfp_non_i_[i] = new float[frames_*channels_];
		}

		char* sptr = iraw_;
		for(TSize i = 0; i < frames_*channels_; i++) {
			rawfloat rf = {0};

			// copy the bytes for 1 sample to the rawfloat pool
			if(!isBigEndianArch() && format_.littleendian)
				for(j = 0; j < bytes; j++, sptr++) rf[j] = *sptr;
			else
				for(j = bytes-1; j >= 0; j--, sptr++) rf[j] = *sptr;

			int dst_channel = (i+j) % channels_;
			float f = 0.0;
			if(format_.floating) f = *((float*) rf);
			else {

				//return NULL;
				for(TSize x = 0; x < sizeof(float); x++)
					f+= (1 << (bytes-x))*rf[x];
			}
			rawfp_non_i_[dst_channel] [i] = f;
		}
	}
	else {


		return (float**) niraw_;
	}


	return rawfp_non_i_;
}

void llaAudioBuffer::changeChannelCount(TChannels channels) {
	// TODO mix teh channels up/down
	clear();
	channels_ = channels;
	if(buffer_alloced_) alloc();
}

void llaudio::llaAudioBuffer::clear(void) {
	if(buffer_alloced_) {
		if(sampleorg_alloced_ == INTERLEAVED) {
			delete [] iraw_;
			iraw_ = NULL;
		}
		else {
			// NON_INTERLEAVED
			for(int ch = 0; ch < channels_alloced_; ch++)
				delete [] niraw_[ch];

			delete [] niraw_;
			niraw_ = NULL;
		}
	}

	buffer_alloced_ = false;
}

void llaAudioBuffer::alloc(void) {
	clear();

	if(sampleorg_ == INTERLEAVED) {
		iraw_ = new char[frames_*channels_*format_.sample_width/8];
	} else {
		niraw_ = new char*[channels_];
		for(int i = 0; i < channels_; i++) {
			niraw_[i] = new char[frames_*format_.sample_width/8];
		}
	}

	buffer_alloced_ = true;

	sampleorg_alloced_ = sampleorg_;
	frames_alloced_ = frames_;
	channels_alloced_ = channels_;
	format_alloced_ = format_;
}

TErrors llaudio::llaAudioBuffer::connectStreams(llaInputStream& input,
		llaOutputStream& output) {
	return input.connect(&output, *this);
}
