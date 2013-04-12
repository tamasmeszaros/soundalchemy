/*
 * salsadriver.cpp
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


#include "salsadriver.h"
#include "salsadevice.h"
#include <sstream>
#include <cstdio>

using namespace std;

namespace llaudio {

void _trim(string& str);

SalsaDriver::SalsaDriver() {
	detectDevices();

}

SalsaDriver::~SalsaDriver() {
//	for(TIsIt it = istreamcache_.begin(); it != istreamcache_.end(); it++)
//		delete it->second;
	istreamcache_.clear();
//	for(TOsIt it = ostreamcache_.begin(); it != ostreamcache_.end(); it++)
//			delete it->second;
	ostreamcache_.clear();
}

TErrors SalsaDriver::detectDevices() {

	devlist_->clear();
	istreamcache_.clear();
	ostreamcache_.clear();

#ifdef USE_EXCEPTIONS
	TErrors e = E_OK;
	try {
		cacheStreams();
		findDevices();
	} catch (llaErrorHandler& err) {
		return err.getLastError();
	}
#else
	TErrors e = cacheStreams();
	if( e == E_OK ) e = findDevices();
#endif

	return e;
}

TErrors SalsaDriver::findDevices(void)
{
	TErrors errstate = E_OK;

	FILE * cards;
	cards = fopen("/proc/asound/cards", "r");
	if(cards == NULL )
	{
		LOGGER().error(E_DETECT_DEVICES, "Cannot access ALSA driver");
		return E_DETECT_DEVICES;
	}

	string cards_content;

	char buffer[20];
	while( fgets(buffer, 20, cards)!=NULL) cards_content.append(buffer);
	fclose(cards);

	LOGGER().debug(cards_content.c_str());

	enum e_parserstates
	{
		CARD_NUM,
		CARD_SHORTNAME,
		CARD_FULL_NAME,
		DDOT,
		JUNK,
		CREATE_DEVICE
	} state = CARD_NUM;

	string card_num;
	string card_shortname;
	string card_fullname;
	SalsaDevice* device = NULL;

	for(string::iterator it=cards_content.begin(); it != cards_content.end(); it++ )
	{
		switch(state)
		{
		case CARD_NUM:
			if(*it == ' ') continue;
			else if(*it >= '0' && *it <= '9' ) card_num.push_back(*it);
			else if(*it == '[') state = CARD_SHORTNAME;
			break;
		case CARD_SHORTNAME:
			if(*it == ':') state = DDOT;
			else if(*it != ' ' && *it != ']') card_shortname.push_back(*it);
			break;
		case DDOT:
			if(*it == ' ') continue;
			else
			{
				card_fullname.push_back(*it);
				state=CARD_FULL_NAME;
			}
			break;
		case CARD_FULL_NAME:
			if(*it != '\n') card_fullname.push_back(*it);
			else state = CREATE_DEVICE;
			break;
		case JUNK:
			if(*it != '\n') continue;
			else state = CARD_NUM;
			break;
		case CREATE_DEVICE:
			break;
		}

		if(state == CREATE_DEVICE)
		{
			int card;
			istringstream(card_num) >> card;
			device = new SalsaDevice(card_shortname, card, card_fullname);

			pair<TOsIt, TOsIt> ret;
			TOsIt it;
			ret = ostreamcache_.equal_range(device->getId());
			SalsaStream * os;


			for(it = ret.first; it != ret.second && it != ostreamcache_.end(); it++)
			{
				os = it->second;
				os->setOwner(*device);
				device->getOutputList()->add(os->getId(), os);
			}

			pair<TIsIt, TIsIt> ret_in;
			TIsIt it_in;
			ret_in = istreamcache_.equal_range(device->getId());
			SalsaStream * is;

			for(it_in = ret_in.first; it_in != ret_in.second && it != istreamcache_.end(); it_in++)
			{
				is = it_in->second;
				is->setOwner(*device);
				device->getInputList()->add(is->getId(), is);
			}

			//(*store)[card_shortname] = device;
			devlist_->add(card_shortname.c_str(), device);

			card_fullname.clear();
			card_shortname.clear();
			card_num.clear();
			state = JUNK;
		}
	}

	return errstate;
}

TErrors SalsaDriver::cacheStreams(void)
{
	TErrors errstate = E_OK;
	string pcms_content;
	string card_num;
	string dev_num;
	string dev_id ;
	string dev_name;
	string isplayback;
	string iscapture;
	bool playback = false, capture = false;
	TAlsaCardId card = -1, dev = -1;

	if( pcms_content.empty() )
	{
		FILE * pcms;
		pcms = fopen("/proc/asound/pcm", "r");
		if(pcms == NULL)
		{
			LOGGER().error(E_DETECT_STREAMS, "Kernel driver maybe not present!");
			return E_DETECT_STREAMS;
		}
		char buffer[20];
		while( fgets(buffer, 20, pcms)!=NULL) pcms_content.append(buffer);
		fclose(pcms);
		LOGGER().debug( pcms_content.c_str());
	}

	enum e_parserstates
	{
		CARD_NUM,
		DEV_NUM,
		STREAM_ID,
		STREAM_NAME,
		PLAYBACK,
		CAPTURE,
	} state = CARD_NUM;
	bool save_data = 0;

	for( string::iterator it=pcms_content.begin();
	     it != pcms_content.end() && errstate == E_OK;
	     it++
	   )
	{
		switch(state)
		{
		case CARD_NUM:
			if( *it >= '0' && *it <= '9') card_num.push_back(*it);
			else if( *it == '-' ) state = DEV_NUM;
			else errstate= E_DETECT_STREAMS;
			break;
		case DEV_NUM:
			if( *it >= '0' && *it <= '9') dev_num.push_back(*it);
			else if( *it == ':' ) state = STREAM_ID;
			else errstate = E_DETECT_STREAMS;
			break;
		case STREAM_ID:
			if( *it != ':' ) dev_id.push_back(*it);
			else state = STREAM_NAME;
			break;
		case STREAM_NAME:
			if( *it != ':' ) dev_name.push_back(*it);
			else state = PLAYBACK;
			break;
		case PLAYBACK:
			if(*it >= '0' && *it <= '9') continue;
			else if( *it != ':' && *it != '\n') isplayback.push_back(*it);
			else if (*it == '\n')
			{
				save_data = true;
				break;
			}
			else if ( *it == ':' ) state = CAPTURE;
			else errstate = E_DETECT_STREAMS;
			break;
		case CAPTURE:
			if(*it >= '0' && *it <= '9') continue;
			if( *it != '\n' ) iscapture.push_back(*it);
			else save_data = true;
			break;
		}

		if( errstate != E_OK)
		{
			LOGGER().error(errstate, "Cannot parse /proc/asound/cards");
			break;
		}

		if(save_data)
		{
			istringstream(card_num) >> card;
			istringstream(dev_num) >> dev;
			_trim(dev_id);
			_trim(dev_name);
			_trim(isplayback);
			_trim(iscapture);

			if( !isplayback.compare("playback") ) playback = true;
			else playback = false;
			if( !iscapture.compare("capture") ) capture = true;
			else capture = false;
			if(playback)
			ostreamcache_.insert(
					OStreamCache::value_type( card,
						new SalsaStream( dev_name.c_str(), dev_id.c_str(), card,
								dev, SalsaStream::OUTPUT_STREAM)
					)
			);

			if(capture)
				istreamcache_.insert(
						IStreamCache::value_type( card,
						new SalsaStream( dev_name.c_str(), dev_id.c_str(),
								card, dev, SalsaStream::INPUT_STREAM)
						)
				);
			save_data = false;

			dev_id.clear();
			dev_name.clear();
			card = -1;
			dev = -1;
			playback = false;
			capture = false;
			isplayback.clear();
			iscapture.clear();
			card_num.clear();
			dev_num.clear();
			state = CARD_NUM;
		}



	}

	return errstate;

}
/* Salsa driver: END ******************************************************** */


/* help functions *************************************************************
 *
 */
void _trim(string& str)
{
	string::iterator it = str.begin();
	while((*it == ' ' || *it == '\n' || *it == '\t' || *it == '\r' )
		  && it != str.end() ) { str.erase(it); it++; }

	it = str.end();
	it--;
	while((*it == ' ' || *it == '\n' || *it == '\t' || *it == '\r' )
			&& it != str.begin() ) { str.erase(it); it--; }

}

}
