/*
 * llastream.cpp
 *
 *  Created on: Mar 3, 2013
 *      Author: Mészáros Tamás
 */
#include "llastream.h"
#include "llaaudiobuffer.h"
#include "lladevicemanager.h"
#include <string>

using namespace llaudio;
using namespace std;

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

TErrors
llaInputStream::connect( llaOutputStream* output, llaAudioBuffer& buffer) {
	TErrors ret = E_OK;
	ret = this->open();
	ret = output->open();
	if(getSampleRate() != output->getSampleRate()) {
		LOGGER().warning(E_STREAM_INCOMPATIBLE, "Sampling rate not equivalent");
	}
	while(!buffer.stop() && ret == E_OK) {
		if(ret == E_OK) {
			ret = read(buffer);
			if(ret != E_OK) break;
			buffer.onSamplesReady();
			ret = output->write(buffer);
			if(ret != E_OK) break;
		}
	}
	this->close();
	output->close();
	return ret;
}

void llaStream::getRawBuffers(llaAudioBuffer& buffer, char **iraw, char ***niraw) {
	if(iraw != NULL) *iraw = buffer.iraw_;
	if(niraw != NULL ) *niraw = buffer.niraw_;
}

void llaStream::setBufferOrganization(llaAudioBuffer& buffer,
			llaAudioBuffer::TSampleOrg org){

//	TSize frames = buffer.getBufferLength();
//	buffer.clear();
	buffer.sampleorg_ = org;
}

llaAudioBuffer::TSampleOrg
llaStream::getBufferOrganization(llaAudioBuffer& buffer) {
	return buffer.sampleorg_;
}

llaudio::llaFileStream::llaFileStream(const char* file) {
	filename_ = file;
	file_ = NULL;
	data_begin_ = 0;
}

TErrors llaudio::llaFileStream::open(void) {

	file_ = fopen(filename_, "rwb");
	if(file_ == NULL) {
		LOGGER().error(E_OPEN_STREAM,
				(string("Failed to open file: ")+filename_).c_str());
		return E_OPEN_STREAM;
	}

	int more_chunks = 1;

	struct riff_wave_header {
		uint32_t riff_id;
		uint32_t riff_sz;
		uint32_t wave_id;
	} wheader;

	struct chunk_header {
		uint32_t id;
		uint32_t sz;
	} chunk_h;

	fread(&wheader, sizeof(wheader), 1, file_);
	if ((wheader.riff_id != ID_RIFF) || (wheader.wave_id != ID_WAVE)) {
		fclose(file_);
		file_ = NULL;
		LOGGER().error(E_OPEN_STREAM, "Cannot detect file format!");
		return E_OPEN_STREAM;
	}

	do {
		fread(&chunk_h, sizeof(chunk_h), 1, file_);

		switch (chunk_h.id) {
		case ID_FMT:
			fread(&wave_info_, sizeof(TWaveFmt), 1, file_);
			/* If the format header is larger, skip the rest */
			if (chunk_h.sz > sizeof(TWaveFmt))
				fseek(file_, chunk_h.sz - sizeof(TWaveFmt), SEEK_CUR);
			break;
		case ID_DATA:
			/* Stop looking for chunks */
			more_chunks = 0;
			break;
		default:
			/* Unknown chunk, skip bytes */
			fseek(file_, chunk_h.sz, SEEK_CUR);
			break;
		}
	} while (more_chunks);

	data_begin_ = ftell(file_);

	if(wave_info_.audio_format != 1) {
		LOGGER().error(E_OPEN_STREAM, "Compressed audio format not supported");
		return E_OPEN_STREAM;
	}
	return E_OK;
}

void llaudio::llaFileStream::close(void) {
	fclose(file_);
	file_ = NULL;
}

const char* llaudio::llaFileStream::getName(void) {
	return filename_;
}

int llaudio::llaFileStream::getId(void) {
	return -1;
}

TSampleRate llaudio::llaFileStream::getSampleRate(void) {
	return wave_info_.sample_rate;
}

TChannels llaudio::llaFileStream::getChannelCount(void) {
	return (TChannels) wave_info_.num_channels;
}

TErrors llaudio::llaFileStream::read(llaAudioBuffer& buffer) {
	if ( file_ == NULL ) {
		LOGGER().error(E_READ_STREAM, "Stream closed!");
		return E_READ_STREAM;
	}
	if( feof(file_) ) {
		fseek(file_, data_begin_, SEEK_SET);
		return E_READ_STREAM;
	}

	bool change = false;
	if(buffer.getChannels() != getChannelCount()) {
		setBufferChannels(buffer, getChannelCount());
		change = true;
	}


	TSampleFormat fmt = getBufferFormat(buffer);
	llaAudioBuffer::TSampleOrg org = getBufferOrganization(buffer);
	if(fmt.isFloating() || !fmt.isLittleEndian() || !fmt.isSigned() ||
			fmt.getBits() != wave_info_.bits_per_sample ) {
		if(wave_info_.bits_per_sample <= 16) {
			setBufferFormat(buffer, llaAudioBuffer::FORMAT_S16LE);
		} else setBufferFormat(buffer, llaAudioBuffer::FORMAT_S32LE);

		if(org != llaAudioBuffer::INTERLEAVED)
			setBufferOrganization(buffer, llaAudioBuffer::INTERLEAVED);
		change = true;
	}

	if(change || !isBufferAlloced(buffer)) allocBuffer(buffer);

	char* iraw;
	getRawBuffers(buffer, &iraw, NULL);

	TSize frames = buffer.getBufferLength();
	TSize bytes = wave_info_.bits_per_sample*wave_info_.num_channels/8;
    int n = fread((void*) iraw, bytes, frames, file_);
    setBufferLastWrite(buffer, n);


	return E_OK;
}


TErrors  llaudio::llaFileStream::write(llaAudioBuffer& buffer) {
	return E_UNIMPLEMENTED;
}


/* llaFileStream implementation */




