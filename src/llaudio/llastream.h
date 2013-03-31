/*
 * llastream.h - llaStream interface (abstract class) definition
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
#ifndef LLASTREAM_H_
#define LLASTREAM_H_

#include "predef.h"
#include "llaaudiobuffer.h"

// TODO replace this with a generic llaFile interface to not depend on this
#include <cstdio>

namespace llaudio {

/**
 * Abstract class for an audio stream. An audio stream is, like a file stream,
 * readable or writable, ensuring the ability of playing-back or capture an
 * audio signal. Most of the methods are abstract and relies on the
 * implementation of the engine (e.g. ALSA).
 */
class llaStream {
protected:

	/// Sample format type shortcut
	typedef llaAudioBuffer::TSampleFormat TSampleFormat;

public:

	/**
	 * Opens the stream and initializes it.
	 * @return Return a E_OK or an TErrors value on error.
	 */
	virtual TErrors open(void) = 0;

	/**
	 * Closes the stream deallocation all resources.
	 */
	virtual void close(void) = 0;

	/**
	 * Get the name of the stream as string representation.
	 * @return Returns the name of the stream as a C string
	 */
	virtual const char * getName(void)=0;

	/**
	 * Get the id (primary key) of the stream. The ID is used by llaudio to
	 * uniquely identify the stream. It can be equal with the engine's id, like
	 * ALSA but can differ from it. It is not recommended to rely on the ID
	 * compatibility with the engine.
	 * @return Returns the integral ID of the stream used by llaudio.
	 */
	virtual int getId(void)=0;

	/**
	 * Sets the sample rate near the desired value. If the applied value differs
	 * the return value is an E_STREAM_PARAM_DIFFERENCE code and a warning is
	 * sent to the log output if the logging is enabled.
	 * @param sample_rate The desired sample rate value;
	 * @return Returns the integral ID of the stream used by llaudio.
	 */
	virtual TErrors setSampleRate(TSampleRate sample_rate) = 0;

	/**
	 * Sets the channels count near the desired value. If the applied value
	 * differs, the return value is an E_STREAM_PARAM_DIFFERENCE code and a
	 * warning is sent to the log output if the logging is enabled.
	 * @param channels Desired channels count.
	 * @return Returns the integral ID of the stream used by llaudio.
	 */
	virtual TErrors setChannelCount(TChannels channels) = 0;

	// Breaks the concept! just for development purposes.
	virtual TErrors setCustomParam(int paramid, void * value) {
		return E_OK;
	}

	/**
	 *
	 * @return Returns the current sample rate of the stream.
	 */
	virtual TSampleRate getSampleRate(void) = 0;

	/**
	 *
	 * @return Returns the current channel count of the stream.
	 */
	virtual TChannels getChannelCount(void) = 0;

	/**
	 * Returns the latency in milliseconds caused by the stream. (not reliable)
	 * @return
	 */
	virtual double getLatency(void) {
		return 1.0 / 0.0;
	}

	/**
	 * Get the range of supported sample rates.
	 * @param min Place-holder for the minimal sample rate value.
	 * @param max Variable for returning the maximal sample rate.
	 * @return Returns E_OK or a TErros error code on failure.
	 */
	TErrors getSampleRateRange(TSampleRate& min, TSampleRate& max) {
		//LOGGER().warning(E_UNIMPLEMENTED, "getSampleRateRange");
		return E_UNIMPLEMENTED;
	}

	/**
	 * Get the range of channel counts supported by the stream.
	 * @param min The minimal channel count.
	 * @param max The maximal channel count.
	 * @return Returns E_OK or a TErros error code on failure.
	 */
	TErrors getChannelCountRange(TChannels& min, TChannels& max) {
		//LOGGER().warning(E_UNIMPLEMENTED, "getChannelCountRange");
		return E_UNIMPLEMENTED;
	}

	/// Default destructor. Does nothing.
	virtual ~llaStream() {}

protected:

	////////////////////////////////////////////////////////////////////////////
	/// Functions for manipulating an llaAudioBuffer object's private properties
	////////////////////////////////////////////////////////////////////////////


	static void getRawBuffers(llaAudioBuffer& buffer, char **iraw,
			char ***niraw);

	static void setBufferOrganization(llaAudioBuffer& buffer,
			llaAudioBuffer::TSampleOrg org);

	static llaAudioBuffer::TSampleOrg getBufferOrganization(
			llaAudioBuffer& buffer);

	static TSampleFormat getBufferFormat(llaAudioBuffer& buffer) {
		return buffer.getFormat();
	}

	static void setBufferFormat(llaAudioBuffer& buffer, TSampleFormat format) {
		buffer.setFormat(format);

	}

	static void setBufferChannels(llaAudioBuffer& buffer, TChannels channels) {
		buffer.setChannels(channels);
	}

	static void setBufferLastWrite(llaAudioBuffer& buffer, TSize frames) {
		buffer.lastwrite_ = frames;
	}

	static TSize getBufferLastWrite(llaAudioBuffer& buffer) {
		return buffer.lastwrite_;
	}

	static void allocBuffer(llaAudioBuffer& buffer) { buffer.alloc(); }

	static bool isBufferAlloced(llaAudioBuffer& buffer) {
		return buffer.buffer_alloced_;
	}
};

/**
 * llaInputStream - represents an input stream of audio data.
 */
class llaInputStream: public virtual llaStream {
public:

	/**
	 * Read samples from the stream.
	 * @param buffer An llaAudioBuffer type object which stores the samples.
	 * @return Returns E_OK or an error code when on failure.
	 */
	virtual TErrors read(llaAudioBuffer& buffer) = 0;

	/**
	 * Connects the input stream with an output stream making them ready for
	 * processing. It has to call the buffer's onSamplesReady() method to apply
	 * the processing on the samples.
	 * @param output An output llaStream.
	 * @param buffer An llaAudioBuffer object as the connecting buffer.
	 * @return Returns E_OK or an error code when on failure.
	 */
	virtual TErrors connect( llaOutputStream* output, llaAudioBuffer& buffer);


	/// Default destructor, does nothing.
	~llaInputStream() {
	}

};

/**
 * Class for an output stream for playback.
 */
class llaOutputStream: public virtual llaStream {
public:

	/**
	 * Writing audio samples to the stream.
	 * @param buffer Buffer holding the audio data.
	 * @return Returns E_OK or an error code when on failure.
	 */
	virtual TErrors write(llaAudioBuffer& buffer) = 0;


	/// Default destructor, does nothing.
	~llaOutputStream() {
	}


};

/**
 * A file stream that represents audio files. Currently supported format is the
 * microsoft riff wave (.wav) and only for playback. The files can be read and
 * written in order to play them or save audio data to them.
 */
class llaFileStream: public llaInputStream, public llaOutputStream {
public:

	/**
	 * Constructor
	 * @param file Name together with the full path of the audio file.
	 */
	llaFileStream(const char* file);

	/// Default destructor, does nothing.
	virtual ~llaFileStream() {}

	////////////////////////////////////////////////////////////////////////////
	/// Methods overloaded from llaStream, llaInputStream and llaOutputStream
	////////////////////////////////////////////////////////////////////////////

	virtual TErrors open(void) ;
	virtual void close(void) ;

	virtual const char * getName(void);
	virtual int getId(void);


	virtual TErrors setSampleRate(TSampleRate sample_rate) { return E_OK;}
	virtual TErrors setChannelCount(TChannels channels) { return E_OK;}

	virtual TSampleRate getSampleRate(void);
	virtual TChannels getChannelCount(void);

	virtual TErrors write(llaAudioBuffer& buffer);
	virtual TErrors read(llaAudioBuffer& buffer);

protected:

	/// Struct for holding riff wave metadata
	struct TWaveFmt {
		uint16_t audio_format;
		uint16_t num_channels;
		uint32_t sample_rate;
		uint32_t byte_rate;
		uint16_t block_align;
		uint16_t bits_per_sample;
	} wave_info_;


	/// General file type
	typedef FILE* llaFile;

	llaFile file_;
	const char* filename_;
	long int data_begin_;

};

class llaNullStream: public llaInputStream, public llaOutputStream {
	TErrors open(void) { return E_OK; }
	void close(void) {}

	const char * getName(void) { return "NULL stream"; }
	int getId(void) { return -1;}


	TErrors setSampleRate(TSampleRate sample_rate) { return E_OK;}
	TErrors setChannelCount(TChannels channels) { return E_OK;}

	TSampleRate getSampleRate(void) {return 0;}
	TChannels getChannelCount(void) { return CH_NONE; }

	TErrors write(llaAudioBuffer& buffer) { return E_OK;}
	TErrors read(llaAudioBuffer& buffer) { return E_OK; }
};

}
#endif /* LLASTREAM_H_ */
