/*
 * salsastream.h
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

#ifndef SALSASTREAM_H_
#define SALSASTREAM_H_

#include "../../llaudioprivate.h"
#include <string>

#ifdef ANDROID
#include <../../../lib/salsa-lib/src/asoundlib.h>
#else
#include  <alsa/asoundlib.h>
#endif

namespace llaudio {

class SalsaStream: public llaInputStream, public llaOutputStream {
public:

	typedef snd_pcm_stream_t TDirections;

	static const snd_pcm_stream_t INPUT_STREAM;
	static const snd_pcm_stream_t OUTPUT_STREAM;


	typedef enum  {
		CLOSED,
		OPENED,
		SETUP,
		RUNNING,
		XRUN,
	} TState;


	SalsaStream(const char* name, const char * id, int card_number, int devnum,
			TDirections direction);
	~SalsaStream();

	/* implementable methods from the interface llaStream: */
	TErrors open(void);
	void close();

	const char * getName(void);
	int getId(void);

	TErrors setSampleRate(TSampleRate sample_rate);
	TErrors setChannelCount(TChannels channels);
	//TErrors setBufferLength(TSize samples_count);

	// Breaks the concept! just for development purposes.
	TErrors setCustomParam(int paramid, void * value) {
		return E_OK;
	}

	TSampleRate getSampleRate(void);
	TChannels getChannelCount(void);


	/* implementable methods from llaInputStream: */
	TErrors read(llaAudioBuffer& buffer);

	/* implementable methods from llaOutputStream: */
	TErrors write(llaAudioBuffer& buffer);

	TErrors connect( llaOutputStream* output, llaAudioBuffer& buffer);

private:
	TErrors _open(int mode);
	static snd_pcm_format_t lla2alsaFormat(llaAudioBuffer::TSampleFormat format);
	static const llaAudioBuffer::TSampleFormat alsa2llaFormat(snd_pcm_format_t format);

	//static void onCaptureComplete(snd_async_handler_t *ahandler);

	void refreshState(void);
	TErrors updateSettings(llaAudioBuffer& buffer);

	std::string name_;
	std::string id_;
	int device_number_;
	int card_number_ ;

	// snd handles
	snd_pcm_t *pcm_;
	snd_pcm_hw_params_t *hw_config_;
	snd_pcm_sw_params_t *sw_config_;

	TSize rate_;
	TChannels channels_;
	snd_pcm_format_t format_;
	snd_pcm_uframes_t buffer_size_;
	snd_pcm_access_t organization_;

	bool buffer_native_;


	TState pcm_state_;
	TDirections direction_;
};

}
#endif /* SALSASTREAM_H_ */
