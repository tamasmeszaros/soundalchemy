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
#include "llaaudiopipe.h"
#include "llastream.h"
#include "lladevicemanager.h"
#include <stdint.h>

using namespace llaudio;

const llaAudioPipe::TSampleFormat llaAudioPipe::FORMAT_S16 = TSampleFormat(false, true, !isBigEndianArch(), 16);
const llaAudioPipe::TSampleFormat llaAudioPipe::FORMAT_S16LE = TSampleFormat(false, true, true, 16);
const llaAudioPipe::TSampleFormat llaAudioPipe::FORMAT_S24 = TSampleFormat(false, true, !isBigEndianArch(), 24);
const llaAudioPipe::TSampleFormat llaAudioPipe::FORMAT_S24LE = TSampleFormat(false, true, true, 24);
const llaAudioPipe::TSampleFormat llaAudioPipe::FORMAT_S32 = TSampleFormat(false, true, !isBigEndianArch(), 32);
const llaAudioPipe::TSampleFormat llaAudioPipe::FORMAT_S32LE = TSampleFormat(false, true, true, 24);
const llaAudioPipe::TSampleFormat llaAudioPipe::FORMAT_FLOAT = TSampleFormat(true, false, !isBigEndianArch(), 32);
const llaAudioPipe::TSampleFormat llaAudioPipe::FORMAT_FLOAT64 = TSampleFormat(true, false, !isBigEndianArch(), 64);
const llaAudioPipe::TSampleFormat llaAudioPipe::FORMAT_DEFAULT = FORMAT_FLOAT;

llaudio::llaAudioPipe::llaAudioPipe(TSize frames)  {
	fail_state_ = false;
	lastwrite_ = 0;

	input_buffer_.framesRequested = frames;
	output_buffer_.framesRequested = frames;
	frames_count_ = frames;
}

llaAudioPipe::Buffer::Buffer() {
	fail_state_ = false;
	buffer_alloced_ = false;
	rawfp_non_i_ = NULL;
	iraw_ = NULL;
	niraw_ = NULL;

	sampleorg_alloced_ = NON_INTERLEAVED;
	frames_alloced_ = 0;
	channels_alloced_ = CH_NONE;
	format_alloced_ = FORMAT_DEFAULT;

	organizationRequested = NON_INTERLEAVED;
	formatRequested = FORMAT_DEFAULT;
	channelsRequested = CH_DEFAULT;
	framesRequested = 0;
}

void llaudio::llaAudioPipe::setBufferLength(TSize frames) {
	input_buffer_.framesRequested = frames;
	output_buffer_.framesRequested = frames;
	frames_count_ = frames;
}

llaudio::llaAudioPipe::~llaAudioPipe() {
	clearBuffers();
}

float** llaudio::llaAudioPipe::Buffer::getSamples( void ) {
	if(!buffer_alloced_) return NULL;
	unsigned int bytes = format_alloced_.sample_width/8;

	// 64 bit floating not supported for now
	if(sizeof(float) < bytes ) return NULL;

	if(sampleorg_alloced_ == INTERLEAVED) {
		if( (format_alloced_.isLittleEndian() && !isBigEndianArch()) ||
			(!format_alloced_.isLittleEndian() && isBigEndianArch()) ) {
			// endianness matches
			if(format_alloced_.isFloating()) {
				float * fptr = (float*)iraw_;
				for(TSize i = 0; i < frames_alloced_*channels_alloced_; i++, fptr++) {
					rawfp_non_i_[i%channels_alloced_] [i/channels_alloced_] = *iraw_;
				}
			}
			else if(!format_alloced_.isSigned()) { // fixed point unsigned
				switch(format_alloced_.sample_width) {
				case 16:
					unsigned2float<uint16_t>();
					break;
				case 24:
				case 32:
					unsigned2float<uint32_t>();
					break;
				} // switch
			} // fixed point unsigned
			else { // fixed point signed
				switch(format_alloced_.sample_width) {
				case 16:
					signed2float<int16_t>();
					break;
				case 24:
				case 32:
					signed2float<int32_t>();
					break;
				} // switch
			}
		} else {
			// not matching endianness
		}
	}
	else {
		return (float**) niraw_;
	}


	return rawfp_non_i_;
}

// NOT TESTED!!!
void llaAudioPipe::Buffer::changeChannelCount(TChannels channels) {
	if(channels == channels_alloced_) return;


	// get the actual audio data
	float **samples = getSamples();

	// make a temp buffer for the mix
	float **temp = new float*[channels];
	TSize frames = getLength();

	// create the mix in the temp buffer
	for(int ch = 0; ch < channels; ch++) {
		temp[ch] = new float[frames];

		if(channels < channels_alloced_) // downmix
			for(TSize fr = 0; fr < frames; fr++) temp[ch] [fr] = ((samples[ch] [fr]) + (samples [ch+1] [fr]))/2;
		else // upmix
			for(TSize fr = 0; fr < frames; fr++) temp[ch] [fr] = samples[0] [fr];
	}

	// clear internal buffers and allocate the new channel count
	clear();
	channelsRequested = channels;
	framesRequested = frames;
	alloc();

	// get an output buffer for writing the mixed temp buffer
	samples = getSamples();


	// copy the temporary buffer to the output buffer
	for(int ch = 0; ch < channels; ch++)
		for(TSize fr = 0; fr < frames; fr++) samples[ch] [fr] = temp [ch] [fr];

	writeSamples();

	for(int ch = CH_NONE; ch < channels; ch++)
		delete [] temp[ch];

	delete [] temp;

}

void llaudio::llaAudioPipe::Buffer::writeSamples() {
	unsigned int bytes = format_alloced_.sample_width/8;

	// 64 bit floating not supported for now
	if(sizeof(float) < bytes ) return;

	if(sampleorg_alloced_ == INTERLEAVED) {

		if( (format_alloced_.isLittleEndian() && !isBigEndianArch()) ||
			(!format_alloced_.isLittleEndian() && isBigEndianArch()) ) {
		// matching endianness
			if(format_alloced_.isFloating()) {
				// floating format
				float* ptr = (float*) iraw_;
				for(TSize i = 0; i < frames_alloced_*channels_alloced_; i++, ptr++) {
					*iraw_ = rawfp_non_i_[i%channels_alloced_] [i/channels_alloced_];
				}
			} else if( !format_alloced_.isSigned()) {
				// fixed point unsigned

				switch(format_alloced_.sample_width) {
				case 16:
					float2unsigned<uint16_t>();
					break;
				case 24:
				case 32:
					float2unsigned<uint32_t>();
					break;

				}
			} else {
				// fixed point signed
				switch(format_alloced_.sample_width) {
				case 16:
					float2signed<int16_t>();
					break;
				case 24:
				case 32:
					float2signed<int32_t>();
					break;

				}
			}
		} // endianness
	}

}

void llaudio::llaAudioPipe::Buffer::convertOrganization(TSampleOrg organization) {
	LOGGER().warning(llaudio::E_BUFFER_DISMATCH, "Buffer organization conversion unimplemented!");
	fail_state_ = true;
}


void llaudio::llaAudioPipe::Buffer::clear(void) {
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

	if(rawfp_non_i_ != NULL ) {
		for(TSize i = 0; i < channels_alloced_; i++) {
			delete [] rawfp_non_i_[i];
		}
		delete [] rawfp_non_i_;
		rawfp_non_i_ = NULL;
	}

	fail_state_ = false;
	buffer_alloced_ = false;
	channels_alloced_ = CH_NONE;
	frames_alloced_ = 0;
}

void llaAudioPipe::Buffer::alloc(void) {

	clear();
	if( fail_state_ ) return;

	if(organizationRequested == INTERLEAVED) {
		iraw_ = new char[framesRequested*channelsRequested*formatRequested.sample_width/8];
	} else {
		niraw_ = new char*[channelsRequested];
		for(int i = 0; i < channelsRequested; i++) {
			niraw_[i] = new char[framesRequested*formatRequested.sample_width/8];
		}
	}

	rawfp_non_i_ = new float*[channelsRequested];
	for(TSize i = 0; i < channelsRequested; i++) {
		rawfp_non_i_[i] = new float[framesRequested*channelsRequested];
	}

	buffer_alloced_ = true;

	sampleorg_alloced_ = organizationRequested;
	frames_alloced_ = framesRequested;
	channels_alloced_ = channelsRequested;
	format_alloced_ = formatRequested;
}

TErrors llaudio::llaAudioPipe::connectStreams(llaInputStream& input,
		llaOutputStream& output) {
	return input.connect(&output, *this);
}
