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

#include <cmath>

namespace llaudio {

/**
 * The class represents the audio data buffer and can be thoughts as a smart
 * sample container which manages it's internals for it's own. With operators
 * overloaded standard operations like addition, subtraction or multiplication
 * of sample values is possible without knowing the actual byte representation
 * of the samples.
 */
class llaAudioPipe {
public:

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
		friend class llaAudioPipe;
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
	 * Sample organization
	 */
	typedef enum {
		INTERLEAVED,    //!< Samples for each channel following each other.
		NON_INTERLEAVED,//!< Channels are separated in multiple buffers
	} TSampleOrg;

	class Buffer {
	protected:

		TSampleOrg sampleorg_alloced_;
		TSize frames_alloced_;
		TChannels channels_alloced_;
		TSampleFormat format_alloced_;

		bool buffer_alloced_;
		char* iraw_;
		char** niraw_;

		// raw pointer to non-interleaved floating point samples
		// used to deliver frames in a unified format independent of the internal
		// formats
		float** rawfp_non_i_;



		friend class llaAudioPipe;
		friend class llaStream;

		TSize framesRequested;

		/**
		 * @return The size of the buffer given with the number of frames
		 */
		TSize getLength(void) { return frames_alloced_; }
		bool fail_state_;


		// map the unsigned fixed point sample buffer to a float buffer
		// with samples ranging [-1.0, 1.0]
		// float = -1 + 2 * uint / (2^width - 1)
		template<class I> void unsigned2float(void) {
			I *ptr = (I*) iraw_;
			float w = exp2(format_alloced_.sample_width) - 1;
			TSize cycles = frames_alloced_*channels_alloced_;
			for(TSize i = 0; i < cycles; i++, ptr++)
				rawfp_non_i_[i%channels_alloced_] [i/channels_alloced_] =
						-1.0 + 2.0*((float) *ptr)/w;
		};


		// convert the float value to an unsigned fixed point sample with
		// with bit length 'width'
		// uint =  ( (float + 1) + (2^width -1) ) / 2
		// class I is an unsigned integer type
		template<class I> void float2unsigned(void) {
			I *ptr = (I*) iraw_;
			float w = exp2(format_alloced_.sample_width) - 1;
			TSize cycles = frames_alloced_*channels_alloced_;
			for(TSize i = 0; i < cycles; i++, ptr++)
				*ptr = (I)  w*(rawfp_non_i_[i%channels_alloced_] [i/channels_alloced_]+1.0)/2.0;
		};

		// signed sample to a float
		// float = 2 * int / 2^width
		// I is a signed integer type
		template<class I> void signed2float(void) {
			I *ptr = (I*) iraw_;
			float w = exp2(format_alloced_.sample_width);
			TSize cycles = frames_alloced_*channels_alloced_;
			for(TSize i = 0; i < cycles; i++, ptr++)
				rawfp_non_i_[i%channels_alloced_] [i/channels_alloced_] = 2.0*((float) *ptr)/w;
		}

		// int = 2^width * float / 2
		template<class I> void float2signed(void) {
			I *ptr = (I*) iraw_;
			float w = exp2(format_alloced_.sample_width);
			TSize cycles = frames_alloced_*channels_alloced_;
			for(TSize i = 0; i < cycles; i++, ptr++)
				*ptr = (I) w * rawfp_non_i_[i%channels_alloced_] [i/channels_alloced_] / 2.0;
		}

	public:

		/// Variables which are handled as requested values. No warranty for
		/// acceptance. The allocation is done in the read(), write(), and
		/// connect() methods of a llaStream object and these parameters
		/// are decided in conjunction with the features of the audio device.
		/// The final parameters can be retrieved with the getter functions.
		TSampleOrg organizationRequested;
		TChannels channelsRequested;
		TSampleFormat formatRequested;

		Buffer();
		virtual ~Buffer() {}

		/**
		 *
		 * @param channels
		 */
		void changeChannelCount(TChannels channels);

		void convertOrganization(TSampleOrg organization);

		/**
		 *
		 * @return
		 */
		const TSampleFormat getFormat(void) { return format_alloced_; }

		/**
		 * @return Returns the number of channels currently allocated.
		 */
		TChannels getChannels(void) { return channels_alloced_; }

		/**
		 * This method prepares a raw float sample matrix which has
		 * channels*frames dimensions. The array is a copy of the samples
		 * in the buffer.
		 * @return Returns a channels*frames dimensional float matrix
		 */
		virtual float** getSamples( void );


		void writeSamples();

		void alloc(void);
		void clear(void);

		bool isAlloced() { return buffer_alloced_; }

	};

	class OutputBuffer: public Buffer {
	public:
		// no need for converting empty raw samples
		float** getSamples() { return rawfp_non_i_;}
	};

	////////////////////////////////////////////////////////////////////////////
	/// Class methods
	////////////////////////////////////////////////////////////////////////////


	bool fail(void) {
		return fail_state_ || input_buffer_.fail_state_ || output_buffer_.fail_state_;
	}

	/**
	 *
	 * @return
	 */
	Buffer& getInputBuffer() { return input_buffer_; }

	/**
	 *
	 * @return
	 */
	Buffer& getOutputBuffer() { return output_buffer_; }


	/**
	 * @return The size of the buffer given with the number of frames
	 */
	TSize getBufferLength(void) { return frames_count_; }

	/**
	 *
	 * @param frames
	 */
	void setBufferLength(TSize frames);


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
	llaAudioPipe(TSize frames = DEFAULT_BUFFER_SIZE);

	/// Default constructor
	virtual ~llaAudioPipe();

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

	// if set, the buffer is unusable until a call is made for clear()
	bool fail_state_;
	TSize lastwrite_;

private:

	Buffer input_buffer_;
	OutputBuffer output_buffer_;

	TSize frames_count_;


	// This class has a friend llaStream. The internal parameters are
	// dynamically set by the stream which uses this buffer
	friend class llaStream;

};


}
#endif /* LLAAUDIOBUFFER_H_ */
