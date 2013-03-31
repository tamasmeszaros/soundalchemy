/*
/*
 * tinyalsastream.cpp
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
 * Copyright (c) 2013 Mészáros Tamás.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 * 
 * Contributors:
 *     Mészáros Tamás - initial API and implementation
 */
#-------------------------------------------------------------------------------
# Copyright (c) 2013 Mészáros Tamás.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v3.0
# which accompanies this distribution, and is available at
# http://www.gnu.org/licenses/gpl.html
# 
# Contributors:
#     Mészáros Tamás - initial API and implementation
#-------------------------------------------------------------------------------
/*
 * tinyalsastream.cpp
 *
 *  Created on: Nov 30, 2012
 *      Author: quarky
 */

#include "tinyalsastream.h"
#include <cmath>

using namespace std;

namespace llaudio {

TinyalsaStream::TinyalsaStream( const char* name,
				const char * id,
				int card_number,
				int devnum,
				enum e_direction direction)
{
	opened_ = false;
	name_.assign(name);
	id_.assign(id);
	device_number_ = devnum;
	card_number_ = card_number;
	direction_ = direction;
	pcmcfg_.period_size = 1024;
	pcmcfg_.period_count = HW_BUFFER_COUNT;
	pcmcfg_.start_threshold = 0;
	pcmcfg_.stop_threshold = 0;
	pcmcfg_.silence_threshold = 0;
	tempbuf_len_ = 0;
}

enum e_errors TinyalsaStream::open(void)
{
	if(!opened_)
	{
		pcm_ = pcm_open(card_number_, device_number_, direction_, &pcmcfg_);
		if (!pcm_ || !pcm_is_ready(pcm_)) {
			llaDriver::logError(E_OPEN_STREAM, false, pcm_get_error(pcm_));
			return E_OPEN_STREAM;
		}

		int size = pcm_frames_to_bytes(pcm_, pcm_get_buffer_size(pcm_));

		tempbuffer_ = new char[size];
		tempbuf_len_ = size;

		opened_ = true;
	}

	return E_OK;
}

void TinyalsaStream::close()
{
	if(opened_)
	{
		pcm_close(pcm_);
		delete tempbuffer_;
		tempbuf_len_ = 0;
		opened_ = false;
	}
	else
	{
		llaDriver::logError(E_DEVICE_CLOSED, false, "TinyalsaStream::close()");
	}
}

const char * TinyalsaStream::getName(void) { return id_.c_str(); }

int TinyalsaStream::getId(void) { return device_number_; }

void TinyalsaStream::setBitDepth(lla_size_t bit_depth)
{
	if(bit_depth < 24) {
		pcmcfg_.format = PCM_FORMAT_S16_LE;
		bits_ = 16;
	}
	else {
		pcmcfg_.format = PCM_FORMAT_S32_LE;
		bits_ = 32;
	}
}

void TinyalsaStream::setSampleRate(lla_size_t sample_rate)
{
	pcmcfg_.rate = sample_rate;
}

void TinyalsaStream::setChannelNum(enum e_channels channels)
{
	pcmcfg_.channels = channels;
}

void TinyalsaStream::setCustomParam(int paramid, void * value)
{
	int c = *((int*) value);
	pcmcfg_.period_count = c;
}

lla_size_t TinyalsaStream::getBitDepth(void) { return bits_; }

lla_size_t TinyalsaStream::getSampleRate(void) { return pcmcfg_.rate; }

enum llaStream::e_channels TinyalsaStream::getChannelNum(void)
{
	return (enum e_channels ) (pcmcfg_.channels);
}

void TinyalsaStream::writeBuffer( llaAudioBuffer& lla_buffer)
{
	lla_size_t ch = lla_buffer.getChannelsCount();

	if(ch != this->pcmcfg_.channels )
	{
		llaDriver::logError( E_BUFFER_DISMATCH, true,
				             "Channel count doesn't match" );
		return ;
	}

	lla_size_t buffer_len = tempbuf_len_;
	lla_size_t word_size = 0;
	float x_max;
	float f_min = -1;
	float f_max = 1;

	switch(pcmcfg_.format)
	{
	case PCM_FORMAT_S16_LE:
		word_size = 2;
		x_max = 65535.0;
		break;
	case PCM_FORMAT_MAX:
	case PCM_FORMAT_S32_LE:
		word_size = 4;
		x_max = 4294967295.0;
		break;
	}

	int sample_divider = word_size*ch;
	if( lla_buffer.getLength() != buffer_len/sample_divider )
	{
		lla_buffer.clear();
		lla_buffer.setLength( buffer_len/sample_divider );
	}

	float part = 0;
	float * num = NULL;
	float step = (f_max - f_min)/x_max;
	for(lla_size_t i = 0, ch_sel=0, sample = 0; i < buffer_len; i++, sample = i/sample_divider, ch_sel = (i/word_size)%ch)
	{
		//printf("i = %d; sample = %d; ch_sel = %d\n", i, sample, ch_sel);
		part = (i%word_size);
		num = &lla_buffer[ch_sel] [sample];
		*num += ((unsigned char)tempbuffer_[i])* pow(256, part);;
		if(part == word_size-1) *num = f_min + *num * step;
	}
	//lla_buffer.raw = tempbuffer_;


}

void TinyalsaStream::readBuffer(llaAudioBuffer& lla_buffer)
{
	lla_size_t ch = lla_buffer.getChannelsCount();

	if(ch != this->pcmcfg_.channels )
	{
		llaDriver::logError( E_BUFFER_DISMATCH, true,
							 "Channel count doesn't match" );
		return ;
	}

	lla_size_t word_size = 0;

	float y_max;
	switch(pcmcfg_.format)
	{
	case PCM_FORMAT_S16_LE:
		word_size = 2;
		y_max = 65535.0;
		break;
	case PCM_FORMAT_MAX:
	case PCM_FORMAT_S32_LE:
		word_size = 4;
		y_max = 4294967295.0;
		break;
	}

	float b = ceil(y_max/2);
	float a = b;
	unsigned long num;
	unsigned char c = 0;
	lla_size_t tempbuf_index = 0;
	lla_size_t j = 0;
	lla_size_t length = lla_buffer.getLength();

	for (lla_size_t i = 0; i <length; i++ )
	{
		for(lla_size_t ch_sel = 0; ch_sel < ch; ch_sel++)
		{
			num = floor(a*lla_buffer[ch_sel] [i]+b);

			for( j = 0; j < word_size; j++)
			{
				c = (unsigned char) num % 256;
				num /= 256;
				tempbuffer_[tempbuf_index+j] = c;

			}
			tempbuf_index+=j;
		}
	}

	//tempbuffer_ = lla_buffer.raw;

}


enum e_errors TinyalsaStream::write(llaAudioBuffer& buffer) {
	enum e_errors ret = E_OK;
	if (opened_) {
//		int size = pcm_frames_to_bytes(pcm_, pcm_get_buffer_size(pcm_));
//
//		//memset(tempbuffer_, 0, size);

		readBuffer(buffer);
		if( pcm_write(pcm_, tempbuffer_, tempbuf_len_) )
		{
			llaDriver::logError(E_WRITE_STREAM, true);
		}


	} else {
		llaDriver::logError(E_WRITE_STREAM, false, "Stream is closed.");
		ret = E_WRITE_STREAM;
	}
	return ret;
}

enum e_errors TinyalsaStream::read(llaAudioBuffer& buffer)
{
	enum e_errors ret = E_OK;
	if (opened_) {
//		int size = pcm_frames_to_bytes(pcm_, pcm_get_buffer_size(pcm_));
//
//
		if( pcm_read(pcm_, tempbuffer_, tempbuf_len_) )
		{
			llaDriver::logError(E_READ_STREAM, true);
		}
//
//		for (int i = 0; i < size; i++)
//			buffer.push_back(tempbuffer_[i]);

		//memset(tempbuffer_, 0, size);
		this->writeBuffer(buffer);

	}
	else {
		llaDriver::logError(E_WRITE_STREAM, false, "Stream is closed.");
		ret = E_WRITE_STREAM;
	}
	return ret;
}

void TinyalsaStream::setBufferLength(lla_size_t size)
{
	pcmcfg_.period_size = size;
}

double TinyalsaStream::getLatency(void)
{
	if (opened_) {
		return 1000.0 * pcm_get_buffer_size(pcm_) / (double) (getSampleRate());
	} else
		llaDriver::logError(E_DEVICE_CLOSED, false, "TinyalsaStream::getLatency");

	return 1.0 / 0.0;
}

lla_size_t TinyalsaStream::getBufferSize(void)
{
//	if(!opened_)
//	{
//		llaDriver::logError(E_DEVICE_CLOSED, false, "Cannot get buffer size!");
//		return 0;
//	}
//
//	return pcm_frames_to_bytes(pcm_, pcm_get_buffer_size(pcm_));
	return tempbuf_len_;
}

}
