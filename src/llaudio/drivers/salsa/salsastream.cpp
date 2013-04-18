/*
 * salsastream.cpp
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

#include "salsastream.h"
#include <sstream>
#include <iostream>

using namespace std;

namespace llaudio {

const snd_pcm_stream_t SalsaStream::INPUT_STREAM = SND_PCM_STREAM_CAPTURE;
const snd_pcm_stream_t SalsaStream::OUTPUT_STREAM = SND_PCM_STREAM_PLAYBACK;


#define _SNDCALL(snd_func) { int e; \
	if((e=snd_func)) logError(E_ALSA, snd_strerror(e)); }

SalsaStream::SalsaStream(const char* name, const char * id, int card_number,
		int devnum, TDirections direction) {

	pcm_state_ = CLOSED;
	name_.assign(name);
	id_.assign(id);
	device_number_ = devnum;
	card_number_ = card_number;
	direction_ = direction;


	rate_ = SR_CD_QUALITY_44100;
	channels_ = CH_STEREO;
	format_ = lla2alsaFormat(llaAudioPipe::FORMAT_DEFAULT);
	buffer_size_ = 0;
	period_size_ = 0;
}

SalsaStream::~SalsaStream() {

}

#define CHECK_SNDERROR( err, llaerr)  if(err) { LOGGER().error(llaerr, snd_strerror(err)); return llaerr; }

TErrors SalsaStream::_open(int mode) {
	if( pcm_state_ != CLOSED ) return E_OK;

		char cname[10];
		sprintf(cname, "hw:%d,%d", card_number_, device_number_);
		int err;
		err = snd_pcm_open(&pcm_, cname, direction_, mode);
		CHECK_SNDERROR(err, E_OPEN_STREAM);

		//snd_pcm_sw_params_current(pcm_, sw_config_);

		snd_pcm_hw_params_alloca(&hw_config_);
		snd_pcm_hw_params_any(pcm_, hw_config_);

		unsigned int temp = rate_;
		err = snd_pcm_hw_params_set_rate_near(pcm_, hw_config_, &temp, NULL);
		CHECK_SNDERROR(err, E_STREAM_CONFIG);

		if( temp != rate_) {
			// cannot set the rate to the specified but to the nearest
			LOGGER().warning(E_STREAM_CONFIG,
					"Cannot set sampling rate to the specified value");
			rate_ = temp;

		}

		temp = channels_;
		err = snd_pcm_hw_params_set_channels_near(pcm_, hw_config_, &temp);
		CHECK_SNDERROR(err, E_STREAM_CONFIG);
		if( temp != channels_) {
			// channels set  to the nearest possible to given
			channels_ = (TChannels)temp;
			LOGGER().warning(E_STREAM_CONFIG,
							"Cannot set channel count to the specified value");

		}
		snd_pcm_hw_params(pcm_, hw_config_);
		/* allocate software parameters */
	//	err = snd_pcm_sw_params_malloc(&sw_config_);
	//	CHECK_SNDERROR(err, E_STREAM_CONFIG);
	//	err = snd_pcm_sw_params_current(pcm_, sw_config_);
	//	CHECK_SNDERROR(err, E_STREAM_CONFIG);
	//	err = snd_pcm_sw_params_set_start_threshold (pcm_, sw_config_, 0U );
	//	CHECK_SNDERROR(err, E_STREAM_CONFIG);
	//	err = snd_pcm_sw_params_set_avail_min(pcm_, sw_config_, buffer_size_/2);
	//	CHECK_SNDERROR(err, E_STREAM_CONFIG);
	//	snd_pcm_sw_params(pcm_, sw_config_);

		pcm_state_ = OPENED;

		return E_OK;
}

TErrors SalsaStream::open(void) {
	return _open(0);
}

void SalsaStream::close() {

	if(pcm_state_ != CLOSED) snd_pcm_close(pcm_);
	pcm_state_ = CLOSED;
}

const char* SalsaStream::getName(void) {
	return this->id_.c_str();
}

int SalsaStream::getId(void) {
	return this->device_number_;
}

snd_pcm_format_t SalsaStream::lla2alsaFormat(TSampleFormat format){
	if(format.isFloating()) {
		if(format.getBits() <= 32)
			return SND_PCM_FORMAT_FLOAT;
		else
			return SND_PCM_FORMAT_FLOAT64;
	}
	else {
		if(format.getBits() <= 32) {
			if(format.isSigned())
				return SND_PCM_FORMAT_S16;
			else return SND_PCM_FORMAT_U16;
		}
		else {
			if(format.isSigned())
				return SND_PCM_FORMAT_S32;
			else return SND_PCM_FORMAT_U32;
		}

	}

	return SND_PCM_FORMAT_UNKNOWN;
}

const llaAudioPipe::TSampleFormat SalsaStream::alsa2llaFormat(snd_pcm_format_t format) {
	TSampleFormat ret;
	switch(format) {
	default:
		ret = llaAudioPipe::FORMAT_DEFAULT;
		break;
	}
	return ret;
}

TErrors SalsaStream::setSampleRate(TSampleRate sample_rate) {
	if(pcm_state_ != CLOSED) refreshState();
	if(pcm_state_ == OPENED || pcm_state_ == SETUP) {

		snd_pcm_hw_params_alloca(&hw_config_);
		snd_pcm_hw_params_current(pcm_, hw_config_);

		unsigned int rate = sample_rate;
		int err=snd_pcm_hw_params_set_rate_near(pcm_, hw_config_, &rate, NULL);
		CHECK_SNDERROR(err, E_STREAM_CONFIG);

		rate_ = rate;

		if(sample_rate != rate) {
			stringstream s;
			const char* dir = (direction_==INPUT_STREAM)?"Input":"Output";
			s << dir << " stream: "<< name_ << "Sample rate: given = " << sample_rate << "applied = " << rate;
			LOGGER().warning(E_STREAM_PARAM_DIFFERENCE, s.str().c_str());
			return E_STREAM_PARAM_DIFFERENCE;
		}

		snd_pcm_hw_params(pcm_, hw_config_);
	}
	else rate_ = sample_rate;


	return E_OK;
}

TErrors SalsaStream::setChannelCount(TChannels channels) {
	if(pcm_state_ != CLOSED) refreshState();
	if(pcm_state_ == OPENED || pcm_state_ == SETUP) {

		snd_pcm_hw_params_alloca(&hw_config_);
		snd_pcm_hw_params_current(pcm_, hw_config_);

		unsigned int ch = channels;
		int err = snd_pcm_hw_params_set_channels_near(pcm_, hw_config_, &ch);
		CHECK_SNDERROR(err, E_STREAM_CONFIG);

		channels_ = (TChannels) ch;
		if(channels != ch){
			stringstream s;
			const char* dir = (direction_==INPUT_STREAM)?"Input":"Output";
			s << dir<< " stream: "<<
					name_ << "channel number: given = " << channels <<
					"applied = " << ch;
			LOGGER().warning(E_STREAM_PARAM_DIFFERENCE, s.str().c_str());
			return E_STREAM_PARAM_DIFFERENCE;
		}


		snd_pcm_hw_params(pcm_, hw_config_);
	}
	else channels_ = channels;

	return E_OK;
}

TSampleRate SalsaStream::getSampleRate(void) {
	if(pcm_state_ != CLOSED) refreshState();

	return rate_;
}

TChannels SalsaStream::getChannelCount(void) {
	if(pcm_state_ != CLOSED) refreshState();
	// TODO implement safely
	return channels_;
}



TErrors SalsaStream::read(llaAudioPipe& buffer) {
	if(pcm_state_ == CLOSED) {
		LOGGER().warning(E_READ_STREAM, "Attempt to write to a closed stream.");
		return E_READ_STREAM;
	}

	TErrors err;
	if( (err = updateSettings(buffer)) != E_OK) return err;


	char *iraw, **niraw;
	int rc;
	getRawInputBuffers(buffer, &iraw, &niraw);

//	snd_pcm_start(pcm_);
//	if ((snderr = snd_pcm_wait (pcm_, -1)) < 0) {
//			//LOGGER().warning(E_READ_STREAM, "Polling failed");
//			//break;
//			//c++;
//			//if(c>5) break;
//	}
	//else LOGGER().debug("poll success\n");
//	snd_pcm_uframes_t frames_to_deliver = snd_pcm_avail_update(pcm_);
//	frames_to_deliver = frames_to_deliver > buffer_size_ ? buffer_size_ : frames_to_deliver;

	if(organization_ == SND_PCM_ACCESS_RW_NONINTERLEAVED) {
		rc = snd_pcm_readn(pcm_, (void**)niraw, buffer_size_);

	}
	else {
		rc = snd_pcm_readi(pcm_, (void*)iraw, buffer_size_);
	}

	setBufferLastWrite(buffer, rc);
	if (rc == -EPIPE) {
	  /* EPIPE means underrun */
	  snd_pcm_prepare(pcm_);
	} else if (rc < 0) {
		LOGGER().warning(E_READ_STREAM, snd_strerror(rc));
		snd_pcm_recover(pcm_, rc, 0);
	}


	return E_OK;
}

void SalsaStream::refreshState(void) {
	if( pcm_state_ == CLOSED ) return;
	snd_pcm_state_t state = snd_pcm_state(pcm_);

	// mapping ALSA states
	switch(state) {
	case SND_PCM_STATE_OPEN:
	case SND_PCM_STATE_SETUP:
	case SND_PCM_STATE_DRAINING:
		pcm_state_ = OPENED;
		break;

	case SND_PCM_STATE_PREPARED:
		if(pcm_state_ != SETUP) pcm_state_ = OPENED; break;

	case SND_PCM_STATE_PAUSED:
	case SND_PCM_STATE_RUNNING:
		pcm_state_ = RUNNING; break;

	case SND_PCM_STATE_XRUN:
		pcm_state_ = XRUN; break;

	case SND_PCM_STATE_SUSPENDED:
	case SND_PCM_STATE_DISCONNECTED:
		pcm_state_ = CLOSED; break;
	}
}

TErrors SalsaStream::updateSettings(llaAudioPipe& buffer) {

	refreshState();

	llaAudioPipe::TSampleOrg org;
	int err;

	// the real buffer size is twice the size of the required latency in frames

	snd_pcm_uframes_t sndframes = buffer.getBufferLength()*BUFFER_SIZE_MULTIPLIER;
	sndframes = sndframes < REAL_BUFFER_SIZE_MIN ? REAL_BUFFER_SIZE_MIN: sndframes ;
	snd_pcm_access_t acc_required;

	TSampleFormat format;
	if(direction_ == INPUT_STREAM) { format = buffer.getInputBuffer().formatRequested; }
	else  { format = buffer.getOutputBuffer().formatRequested; }

	snd_pcm_format_t sndformat = lla2alsaFormat(format);

	switch(pcm_state_) {
	case CLOSED:
		return E_OK;
	case OPENED: // set and write parameters if they changed since last setup

		// initial params
		snd_pcm_hw_params_alloca(&hw_config_);
		snd_pcm_hw_params_any(pcm_, hw_config_);

		// /////////////////////////////////////////////////////////////////////
		// set buffer organization: interleaved/non_interleaved
		// The llaAudioBuffer object's setting has priority
		if(direction_ == INPUT_STREAM)
			org = buffer.getInputBuffer().organizationRequested;
		else
			org = buffer.getOutputBuffer().organizationRequested;

		acc_required = (org == llaAudioPipe::INTERLEAVED)?
				SND_PCM_ACCESS_RW_INTERLEAVED:SND_PCM_ACCESS_RW_NONINTERLEAVED;


		err = snd_pcm_hw_params_set_access(pcm_, hw_config_,
				acc_required);

		if(err ) {
			// cannot set required access mode, hmm hmm, try to convert
			// the buffer data organization or update  buffer organization if
			// this is an input stream
			acc_required = (acc_required==SND_PCM_ACCESS_RW_NONINTERLEAVED)?
					SND_PCM_ACCESS_RW_INTERLEAVED:SND_PCM_ACCESS_RW_NONINTERLEAVED;

			err = snd_pcm_hw_params_set_access(pcm_, hw_config_,
					acc_required);
			CHECK_SNDERROR(err, E_STREAM_CONFIG);

			llaAudioPipe::TSampleOrg org_required =
					(acc_required==SND_PCM_ACCESS_RW_NONINTERLEAVED)?
					llaAudioPipe::NON_INTERLEAVED:llaAudioPipe::INTERLEAVED;

			if(direction_ == INPUT_STREAM) {
				buffer.getInputBuffer().organizationRequested = org_required;
			} else {
				//buffer.getOutputBuffer().convertOrganization(org_required);
				buffer.getOutputBuffer().organizationRequested = org_required;
			}
		}
		organization_ = acc_required;

		// /////////////////////////////////////////////////////////////////////
		// Sample format base is the buffer object's format with conversion to
		// a suitable alternative form the engine

		err = snd_pcm_hw_params_set_format(pcm_, hw_config_, sndformat);

		if(err) {
			// TODO find a format supported by the engine and convert the
			// buffer data if this is an output stream, but for now

			// set the closest format to required and set the buffer to this
			if(format.isFloating()) {
				if( format.getBits() <= 32 ) {
					// set something signed 16
					sndformat = SND_PCM_FORMAT_S16;
					err = snd_pcm_hw_params_set_format(pcm_, hw_config_,
							sndformat);
					if(direction_ == INPUT_STREAM) {
						buffer.getInputBuffer().formatRequested = llaAudioPipe::FORMAT_S16;
					}
					else {
						buffer.getOutputBuffer().formatRequested = llaAudioPipe::FORMAT_S16;
					}
				}
				else {
					sndformat = SND_PCM_FORMAT_S32;
					err = snd_pcm_hw_params_set_format(pcm_, hw_config_,
							sndformat);
					if(direction_ == INPUT_STREAM) {
						buffer.getInputBuffer().formatRequested = llaAudioPipe::FORMAT_S32;
					}
					else {
						buffer.getOutputBuffer().formatRequested = llaAudioPipe::FORMAT_S32;
					}
				}
				CHECK_SNDERROR(err, E_STREAM_CONFIG);
			}


			if(err) {
				LOGGER().warning(E_BUFFER_DISMATCH, "Pcm format not supported");
				CHECK_SNDERROR(err, E_STREAM_CONFIG);
			}
		}
		else {
			format_ = sndformat;
		}


		// /////////////////////////////////////////////////////////////////////
		// set buffer's channel number: the stream's configuration has priority,
		// the buffer is up-mixed or down-mixed by it's methods

		if(direction_ == OUTPUT_STREAM) {
			if(buffer.getOutputBuffer().channelsRequested != channels_) {
				buffer.getOutputBuffer().changeChannelCount(channels_);
			}
		} else {
			if(buffer.getInputBuffer().channelsRequested != channels_) {
				buffer.getInputBuffer().channelsRequested = channels_;
			}
		}

		// set buffer size /////////////////////////////////////////////////////
		if(buffer_size_ != sndframes) {
			err = snd_pcm_hw_params_set_buffer_size_near(pcm_, hw_config_,
					&sndframes);
			CHECK_SNDERROR(err, E_STREAM_CONFIG);
			buffer_size_ = sndframes;
		}

		sndframes = buffer.getBufferLength()/2;
		//sndframes = sndframes < REAL_PERIOD_SIZE_MIN? REAL_PERIOD_SIZE_MIN : sndframes;
		//sndframes = sndframes > REAL_PERIOD_SIZE_MAX? REAL_PERIOD_SIZE_MAX : sndframes;
		err = snd_pcm_hw_params_set_period_size_near(pcm_, hw_config_,
				&sndframes, NULL);
		CHECK_SNDERROR(err, E_STREAM_CONFIG);
		period_size_ = sndframes;

		// write settings //////////////////////////////////////////////////////
		if(direction_ == INPUT_STREAM)  buffer.getInputBuffer().alloc();
		else buffer.getOutputBuffer().alloc();

		snd_pcm_hw_params(pcm_, hw_config_);
		err = snd_pcm_hw_params_get_period_size(hw_config_, &period_size_, NULL);
		CHECK_SNDERROR(err, E_STREAM_CONFIG);

		char num[10];
		sprintf(num, "%ld", period_size_);
		LOGGER().log(num);

		pcm_state_ = SETUP;
		snd_pcm_prepare(pcm_);

		break;

	case RUNNING:
		break;

	case SETUP:
		// TODO check and renew settings
		break;
	case XRUN:
		snd_pcm_prepare(pcm_);
		break;
	};


	return E_OK;
}

TErrors SalsaStream::write(llaAudioPipe& buffer) {

	if(pcm_state_ == CLOSED) {
		LOGGER().warning(E_WRITE_STREAM, "Attempt to write to a closed stream.");
		return E_WRITE_STREAM;
	}

	TErrors err;
	if( (err = updateSettings(buffer)) != E_OK) return err;

	buffer.onSamplesReady();
	if(buffer.fail()) {
		return E_WRITE_STREAM;
	}

	char *iraw, **niraw;
	int rc;

	getRawInputBuffers(buffer, &iraw, &niraw);

	TSize frames_to_deliver = getBufferLastWrite(buffer);
	if( frames_to_deliver <= 0 || frames_to_deliver > buffer_size_)
		frames_to_deliver = buffer.getBufferLength();

	if(organization_ == SND_PCM_ACCESS_RW_NONINTERLEAVED) {
		rc = snd_pcm_writen(pcm_, (void**)niraw, frames_to_deliver);

	}
	else {
		rc = snd_pcm_writei(pcm_, (void*)iraw, frames_to_deliver);
	}

	if (rc == -EPIPE) {
	  /* EPIPE means underrun */
	  snd_pcm_prepare(pcm_);
	} else if (rc < 0) {
		LOGGER().warning(E_WRITE_STREAM, snd_strerror(rc));
		//snd_pcm_recover(pcm_, rc, 0);
		return E_WRITE_STREAM;
	}

	return E_OK;
}

TErrors SalsaStream::connect( llaOutputStream* output, llaAudioPipe& buffer) {
	TErrors ret = _open(SND_PCM_NONBLOCK);
	// Update pcm settings
	if( ret != E_OK || (ret = updateSettings(buffer)) != E_OK ) return ret;

	if( (ret = output->open()) != E_OK ) {
		close();
		return ret;
	}


	int err = 0;

	// set up the output device ////////////////////////////////////////////////

//	 This is not a nice operation but the output stream has to be a SalsaStream
//	 since at this point the engine is surely the salsa-lib library.
//	SalsaStream *salsaoutput = (SalsaStream*) output;

//	// open the output device
//	if( (ret = salsaoutput->_open(SND_PCM_NONBLOCK)) != E_OK) return ret;
//
//	//Update settings
//	if( ret != E_OK || (ret = salsaoutput->updateSettings(buffer)) != E_OK )
//		return ret;


//	// get the period size
	snd_pcm_uframes_t avail_min = period_size_;
	//avail_min = buffer.getBufferLength()/period_size_ < 4? period_size_: buffer.getBufferLength()/2  ;
	//avail_min = avail_min < REAL_PERIOD_SIZE_MIN? REAL_PERIOD_SIZE_MIN : avail_min;
			char num[20];
					sprintf(num, "%ld", avail_min);
					LOGGER().log(num);

	// Set up the input stream for polling /////////////////////////////////////
	// This is the same process as for the output.

	err = snd_pcm_sw_params_malloc(&sw_config_);
	err = snd_pcm_sw_params_current(pcm_, sw_config_);
	CHECK_SNDERROR(err, E_STREAM_CONFIG);

	snd_pcm_hw_params_alloca(&hw_config_);
	snd_pcm_hw_params_current(pcm_, hw_config_);

//	err = snd_pcm_hw_params_get_period_size(hw_config_,  &period_size, NULL);
//	CHECK_SNDERROR(err, E_STREAM_CONFIG);

	err = snd_pcm_sw_params_set_start_threshold (pcm_, sw_config_, 0U );
	CHECK_SNDERROR(err, E_STREAM_CONFIG);

	err = snd_pcm_sw_params_set_avail_min(pcm_, sw_config_, avail_min  );
	CHECK_SNDERROR(err, E_STREAM_CONFIG);

	err = snd_pcm_sw_params(pcm_, sw_config_);
	CHECK_SNDERROR(err, E_STREAM_CONFIG);

	snd_pcm_prepare(pcm_);





//	// set up for polling write
//	err = snd_pcm_sw_params_malloc(&(salsaoutput->sw_config_));
//	err = snd_pcm_sw_params_current(salsaoutput->pcm_, salsaoutput->sw_config_);
//	CHECK_SNDERROR(err, E_STREAM_CONFIG);
//
//	snd_pcm_hw_params_alloca(&(salsaoutput->hw_config_));
//	snd_pcm_hw_params_current(salsaoutput->pcm_, salsaoutput->hw_config_);
////
////	err = snd_pcm_hw_params_get_period_size(salsaoutput->hw_config_,
////			&period_size, NULL);
////	CHECK_SNDERROR(err, E_STREAM_CONFIG);
//
//
//	err = snd_pcm_sw_params_set_start_threshold (salsaoutput->pcm_,
//			salsaoutput->sw_config_, 0U );
//	CHECK_SNDERROR(err, E_STREAM_CONFIG);
//
//	err = snd_pcm_sw_params_set_avail_min(salsaoutput->pcm_,
//			salsaoutput->sw_config_, buffer.getBufferLength()*8/10 );
//	CHECK_SNDERROR(err, E_STREAM_CONFIG);
//
//	err = snd_pcm_sw_params(salsaoutput->pcm_, salsaoutput->sw_config_);
//	CHECK_SNDERROR(err, E_STREAM_CONFIG);
//
//	snd_pcm_prepare(salsaoutput->pcm_);


	snd_pcm_uframes_t frames;

	char *iraw, **niraw;

	while(!buffer.stop() && !buffer.fail()) {
		snd_pcm_start(pcm_);
		if ((err = snd_pcm_wait (pcm_, -1)) < 0) {
				//LOGGER().warning(E_READ_STREAM, "Polling failed");
				//break;
				//c++;
				//if(c>5) break;
		}
		//else LOGGER().debug("poll success\n");

		frames = snd_pcm_avail_update(pcm_);
		//wr_frames = frames;

//		char num[20];
//				sprintf(num, "%ld", frames);
//				LOGGER().log(num);
		if(frames <=0 || frames > buffer.getBufferLength()) frames = buffer.getBufferLength();

		int rc;
		getRawInputBuffers( buffer, &iraw, &niraw);

		// read from the stream
		if(organization_ == SND_PCM_ACCESS_RW_NONINTERLEAVED) {
			rc = snd_pcm_readn(pcm_, (void**)niraw, frames);
		}
		else {
			rc = snd_pcm_readi(pcm_, (void*)iraw, frames);
		}

//		char num[20];
//				sprintf(num, "%ld", wr_frames);
//				LOGGER().log(num);

		// save the number of frames read
		TSize lastwrite = ((TSize)rc <= buffer.getBufferLength())?
				rc : buffer.getBufferLength();

		setBufferLastWrite(buffer, lastwrite);

		// detect errors
		if (rc == -EPIPE) {
		  /* EPIPE means underrun */
		  snd_pcm_prepare(pcm_);
		} else if (rc < 0) {
			LOGGER().warning(E_READ_STREAM, snd_strerror(rc));
			//snd_pcm_recover(pcm_, rc, 0);
			snd_pcm_sw_params_free(sw_config_);
			close();
			output->close();
			return E_READ_STREAM;
		}

		TErrors reterr;
		if( (reterr=output->write(buffer)) != E_OK ) {
			snd_pcm_sw_params_free(sw_config_);
			close();
			output->close();
		}

		// calling the pipe's callback
//		buffer.onSamplesReady();
//		if(buffer.fail()) {
//			snd_pcm_sw_params_free(sw_config_);
//			snd_pcm_sw_params_free(salsaoutput->sw_config_);
//			close();
//			output->close();
//			return E_BUFFER_DISMATCH;
//		}
//
//		// write the frames to the output stream ///////////////////////////////
//		snd_pcm_start(salsaoutput->pcm_);
//		if ((err = snd_pcm_wait (salsaoutput->pcm_, -1)) < 0) {
//				//LOGGER().warning(E_READ_STREAM, "Polling failed");
//				//break;
//				//c++;
//				//if(c>5) break;
//		}
//
//		wr_frames = snd_pcm_avail_update(salsaoutput->pcm_);
//		frames = wr_frames > lastwrite? lastwrite : wr_frames;
//		getRawOutputBuffers( buffer, &iraw, &niraw);
//
//		if(salsaoutput->organization_ == SND_PCM_ACCESS_RW_NONINTERLEAVED) {
//			rc = snd_pcm_writen(salsaoutput->pcm_, (void**)niraw, frames);
//		}
//		else {
//			rc = snd_pcm_writei(salsaoutput->pcm_, (void*)iraw, frames);
//		}
//
//		// detect errors
//		if (rc == -EPIPE) {
//		  /* EPIPE means underrun */
//		  snd_pcm_prepare(salsaoutput->pcm_);
//		} else if (rc < 0) {
//			LOGGER().warning(E_WRITE_STREAM, snd_strerror(rc));
//			//snd_pcm_recover(pcm_, rc, 0);
//			snd_pcm_sw_params_free(sw_config_);
//			snd_pcm_sw_params_free(salsaoutput->sw_config_);
//			close();
//			output->close();
//			return E_WRITE_STREAM;
//		}

	}

	snd_pcm_sw_params_free(sw_config_);
	//snd_pcm_sw_params_free(salsaoutput->sw_config_);
	close();
	output->close();

	return E_OK;
}

}
