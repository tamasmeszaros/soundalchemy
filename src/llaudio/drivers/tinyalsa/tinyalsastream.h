/*
 * tinyalsastream.h
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
 * tinyalsastream.h
 *
 *  Created on: Nov 30, 2012
 *      Author: quarky
 */

#ifndef TINYALSASTREAM_H_
#define TINYALSASTREAM_H_

#include <llaudio.h>
#include <tinyalsa/asoundlib.h>

#include "../../devicestore.h"
#include "tinyalsadriver.h"
#include <string>

namespace llaudio
{

class TinyalsaStream: public llaInputStream,
					  public llaOutputStream
{

public:
	enum e_direction
	{
		INPUT_STREAM = PCM_IN,
		OUTPUT_STREAM = PCM_OUT
	};


	TinyalsaStream( const char* name,
					const char * id,
					int card_number,
					int devnum, enum e_direction direction );

	enum e_errors open(void);

	void close(void);

	const char * getName(void);

	int getId(void);


	void setBitDepth(lla_size_t bit_depth);

	void setSampleRate(lla_size_t sample_rate);

	void setChannelNum(enum e_channels channels);

	void setBufferLength(lla_size_t size);

	void setCustomParam(int paramid, void * value);


	lla_size_t getBitDepth(void);

	lla_size_t getSampleRate(void);

	enum e_channels getChannelNum(void);

	enum e_errors write( llaAudioBuffer& buffer );

	enum e_errors read( llaAudioBuffer& buffer);

	double getLatency(void);

	lla_size_t getBufferSize(void);


protected:

	void writeBuffer( llaAudioBuffer& lla_buffer);
	void readBuffer(llaAudioBuffer& lla_buffer);

	std::string name_;
	std::string id_;
	int device_number_;
	int card_number_ ;

	struct pcm_config pcmcfg_;
	struct pcm* pcm_;
	int bits_;

	bool opened_;
	enum e_direction direction_;

	char * tempbuffer_;
	lla_size_t tempbuf_len_;

	friend class TinyalsaDevice;

};

}


#endif /* TINYALSASTREAM_H_ */
