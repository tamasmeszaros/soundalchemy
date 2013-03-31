/*
/*
 * tinyalsadevice.cpp
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
 * tinyalsadevice.cpp
 *
 *  Created on: Nov 30, 2012
 *      Author: quarky
 */

#include "tinyalsadevice.h"

using namespace std;

namespace llaudio {

TinyalsaDevice::TinyalsaDevice(string name, int card, string fullname )
{
	/* Find ports of this card and register them as streams */
	name_ = name;
	card_ = card;
	fullname_ = fullname;
}

llaInputStream * TinyalsaDevice::_getInputStream(int id )
{
	llaInputStream * is = NULL;

	TIstreamList::iterator ip = istreams_.find(id);
	if(ip == istreams_.end())
	{
		llaDriver::logError(E_DETECT_STREAMS, true, "No stream is associated with the given id!");
		is = NULL;
	}
	else
	{
		is = ip->second;
		is->setBitDepth(16);
		is->setChannelNum(llaStream::CH_STEREO);
		is->setSampleRate(44100);
	}

	return is;
};

llaOutputStream * TinyalsaDevice::_getOutputStream(int id )
{
	llaOutputStream * os = NULL;

	TOstreamList::iterator op = ostreams_.find(id);
	if(op == ostreams_.end())
	{
		llaDriver::logError(E_DETECT_STREAMS, true, "No stream is associated with the given id!");
		os = NULL;
	}
	else
	{
		os = op->second;
		os->setBitDepth(16);
		os->setChannelNum(llaStream::CH_STEREO);
		os->setSampleRate(44100);
	}

	return os;
};

const char* TinyalsaDevice::getName(bool full)
{
	if(full)
		return this->fullname_.c_str();

	return this->name_.c_str();
}

TinyalsaDevice::~TinyalsaDevice()
{
	for(TOstreamListIt it = ostreams_.begin(); it != ostreams_.end(); it++)
		delete it->second;

	for(TIstreamListIt it = istreams_.begin(); it != istreams_.end(); it++)
				delete it->second;
};

}
