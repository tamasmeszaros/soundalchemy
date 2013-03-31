/*
 * lladevicemanager.h
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
#ifndef LLADEVICEMANAGER_H_
#define LLADEVICEMANAGER_H_

#include "predef.h"
#include "llacontainer.h"
#include "llaerrorhandler.h"

namespace llaudio {

typedef llaDevice* TDeviceListElement;
typedef llaContainer<TDeviceId, TDeviceListElement> llaDeviceList;
typedef llaDeviceList::Iterator llaDeviceIterator;
typedef llaContainer<const char*, llaFileStream*> llaFileStreamList;
typedef llaFileStreamList::Iterator llaFStreamIterator;

class llaDeviceManager {


public:

	static llaDeviceManager& getInstance(llaErrorHandler& errhandl,
			TDrivers audio_driver = DEFAULT_DRIVER);

	static llaDeviceManager& getInstance( TDrivers audio_driver = DEFAULT_DRIVER);

	static llaErrorHandler& getErrorHandler(void);

	void setErrorHandler(llaErrorHandler& handler);

	llaDeviceIterator getDeviceIterator(void);
	llaDevice& getDevice(TDeviceId id);
	llaDevice& getDefaultDevice(void);

	llaInputStream& getInputStream(TDeviceId device,
			TStreamId id = STREAM_ID_DEFAULT);
	llaOutputStream& getOutputStream(TDeviceId device,
			TStreamId id = STREAM_ID_DEFAULT);

	llaFileStream& getFileStream(const char* file);

	TErrors setDriver(TDrivers audio_driver);

	TErrors refresh();

	static void destroy(void) __LLATHROW;

	~llaDeviceManager() __LLATHROW;



private:
	llaDeviceManager();

	// Private so that it can not be called
	llaDeviceManager(llaDeviceManager const&) {
	}

	// copy constructor is private
	llaDeviceManager const& operator=(llaDeviceManager const& ref) {
		return ref;
	}


	static llaDeviceManager* m_pInstance_;
	static llaErrorHandler *errorHandler_;
	static bool errorhandler_builtin_;

	llaDriver *driver_;
	llaFileStreamList * fstreamlist_;
};
}
#endif /* LLADEVICEMANAGER_H_ */
