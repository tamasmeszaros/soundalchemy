/*
 * llaaudiobuffer.h
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

#ifndef LLAAUDIOBUFFER_H_
#define LLAAUDIOBUFFER_H_

#include "predef.h"

namespace llaudio {

/**
 * The class represents the audio data buffer and can be thoughts as a smart
 * sample container which manages it's internals for it's own. With operators
 * overloaded standard operations like addition, subtraction or multiplication
 * of sample values is possible without knowing the actual byte representation
 * of the samples.
 */
class llaAudioBuffer {
public:

	////////////////////////////////////////////////////////////////////////////
	/// Data types bound to this class
	////////////////////////////////////////////////////////////////////////////

	/**
	 * This struct is intended to be used as a data type with it's defined
	 * constant values. Implemented as a struct to be easily evaluated.
	 */
	struct TSampleFormat {
		TSampleFormat() {}
		bool isSigned() { return sig; }
		bool isFloating() {return floating; }
		bool isLittleEndian() { return littleendian; }
		TSize getBits() { return sample_width; }
	private:
		TSampleFormat( bool fp, bool s, bool le, TSize w) {
			floating = fp;
			littleendian = le;
			sample_width = w;
			sig = s;
		}
		bool floating;
		bool littleendian;
		bool sig;
		TSize sample_width;
		friend class llaAudioBuffer;
	};

	/// sample formats
	static const TSampleFormat FORMAT_DEFAULT; 	//!< Default format on init
	static const TSampleFormat FORMAT_S16;		//!< Signed 16bit
	static const TSampleFormat FORMAT_S16LE;	//!< Signed 16 bit little endian
	static const TSampleFormat FORMAT_S24;		//!< Signed 24 bit
	static const TSampleFormat FORMAT_S24LE;	//!< Signed 24 bit little endian
	static const TSampleFormat FORMAT_S32;		//!< Signed 32 bit
	static const TSampleFormat FORMAT_S32LE;	//!< Signed 32 bit little endian
	static const TSampleFormat FORMAT_FLOAT;	//!< Floating 32bit
	static const TSampleFormat FORMAT_FLOAT64;	//!< Floating 64bit

	/**
	 * Sample organization types
	 */
	typedef enum {
		INTERLEAVED,    //!< Samples for each channel following each other.
		NON_INTERLEAVED,//!< Channels are separated in multiple buffers
	} TSampleOrg;

	////////////////////////////////////////////////////////////////////////////
	/// Class methods
	////////////////////////////////////////////////////////////////////////////

	/**
	 * @return The size of the buffer given with the number of frames
	 */
	TSize getBufferLength(void) { return frames_; }

	/**
	 * @return Returns the number of channels.
	 */
	TChannels getChannels(void) { return channels_; }

	/**
	 * This method prepares a raw float sample matrix which has channels*frames
	 * dimensions. The array is a copy of the samples in the buffer.
	 * @return Returns a channels*frames dimensional float matrix
	 */
	float** getSamples( void );

	/**
	 * Writes the changes made in the obtained float matrix.
	 */
	void writeSamples();

	/**
	 *
	 * @param channels
	 */
	void changeChannelCount(TChannels channels);

	/**
	 *
	 * @param input
	 * @param output
	 * @return
	 */
	TErrors connectStreams(llaInputStream& input, llaOutputStream& output );

	/**
	 *
	 * @param frames
	 */
	llaAudioBuffer(TSize frames = DEFAULT_BUFFER_SIZE);

	/// Default constructor
	virtual ~llaAudioBuffer();

	////////////////////////////////////////////////////////////////////////////
	/// functions to overload
	////////////////////////////////////////////////////////////////////////////

	/**
	 *
	 */
	virtual void onSamplesReady(void) {
		;
	}

	/**
	 *
	 * @return
	 */
	virtual bool stop(void) { return false; }

	/* Operator overloads */


protected:

	/**
	 *
	 * @param frames
	 */
	void setBufferLength(TSize frames) { frames_ = frames; }

	/**
	 *
	 * @param channels
	 */
	void setChannels(TChannels channels) { channels_ = channels; }

	/**
	 *
	 * @param fmt
	 */
	void setFormat(const TSampleFormat fmt) { format_ = fmt; }

	/**
	 *
	 * @return
	 */
	const TSampleFormat getFormat(void) { return format_; }

	void alloc(void);
	void clear(void);

	// raw pointer to non-interleaved floating point samples
	float** rawfp_non_i_;

	TSampleOrg sampleorg_;
	TSize frames_;
	TChannels channels_;
	TSampleFormat format_;

	TSampleOrg sampleorg_alloced_;
	TSize frames_alloced_;
	TChannels channels_alloced_;
	TSampleFormat format_alloced_;

	TSize lastwrite_;
	//double latency_;

	bool buffer_alloced_;
	char* iraw_;
	char** niraw_;


	// This class has a friend llaStream. The internal parameters are
	// dynamically set by the stream which uses this buffer
	friend class llaStream;

};


}
#endif /* LLAAUDIOBUFFER_H_ */
