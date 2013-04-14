/*
 * lladevice.h
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
#ifndef LLADEVICE_H_
#define LLADEVICE_H_

#include "predef.h"
#include "llacontainer.h"
#include "llaaudiopipe.h"


namespace llaudio {

class llaDevice {
public:
	typedef  llaInputStream* TIStreamListElement;
	typedef  llaOutputStream* TOStreamListElement;

	typedef llaContainer<TStreamId, TIStreamListElement> IStreamList;
	typedef IStreamList::Iterator IStreamIterator;

	typedef llaContainer<TStreamId, TOStreamListElement> OStreamList;
	typedef OStreamList::Iterator OStreamIterator;

	typedef llaAudioPipe::TSampleFormat TSampleFormat;

public:


	llaDevice();
	llaDevice(IStreamList *input_stream_list, OStreamList *output_stream_list);
	virtual ~llaDevice();

	virtual bool isNull(void) { return false; }

	llaInputStream& getInputStream(
			TStreamId id = STREAM_ID_DEFAULT,
			TSampleRate sample_rate = SR_DEFAULT,
			TChannels channels = CH_MONO,
			TSize buffer_size = DEFAULT_BUFFER_SIZE
			//TSampleFormat sample_format = llaAudioBuffer::FORMAT_DEFAULT
			);

	llaInputStream& getInputStream(
			const char* name,
			TSampleRate sample_rate = SR_DEFAULT,
			TChannels channels = CH_MONO,
			TSize buffer_size = DEFAULT_BUFFER_SIZE
			//TSampleFormat sample_format = llaAudioBuffer::FORMAT_DEFAULT
			);

	llaOutputStream& getOutputStream(
			TStreamId id = STREAM_ID_DEFAULT,
			TSampleRate sample_rate = SR_DEFAULT,
			TChannels channels = CH_STEREO,
			TSize buffer_size = DEFAULT_BUFFER_SIZE
			//TSampleFormat sample_format = llaAudioBuffer::FORMAT_DEFAULT
			);

	llaOutputStream& getOutputStream(
			const char* name,
			TSampleRate sample_rate = SR_DEFAULT,
			TChannels channels = CH_STEREO,
			TSize buffer_size = DEFAULT_BUFFER_SIZE
			//TSampleFormat sample_format = llaAudioBuffer::FORMAT_DEFAULT
			);

	virtual const char* getName(bool full = false)=0;

	IStreamIterator getInputStreamIterator(void);
	OStreamIterator getOutputStreamIterator(void);

	// TODO: add mixing functionality
	// TODO: add MIDI functionality

protected:

	IStreamList * input_stream_list_;
	OStreamList * output_stream_list_;
};

class llaNullDevice: public llaDevice {
	const char* getName(bool full = false){ return "NULL device"; }
	bool isNull(void) { return true; }
};

}
#endif /* LLADEVICE_H_ */
