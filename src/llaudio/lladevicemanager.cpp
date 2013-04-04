/*
 * lladevicemanager.cpp
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

#include "lladevicemanager.h"
#include "defaultcontainer.h"
#include "drivers/salsa/salsadriver.h"


using namespace std;
using namespace llaudio;

bool llaudio::isBigEndianArch(void) {
	union {
		uint32_t i;
		char c[4];
	} bint = {0x01020304};

	return bint.c[0] == 1;
}

/* *****************************************************************************
 * llaDeviceManager class implementation:
 */


llaDeviceManager* llaDeviceManager::m_pInstance_ = NULL;
llaErrorHandler *llaDeviceManager::errorHandler_ = NULL;
bool llaDeviceManager::errorhandler_builtin_ = false;


llaDeviceManager& llaDeviceManager::getInstance(llaErrorHandler& errhandl,
		TDrivers audio_driver)
{

	if(!m_pInstance_) {
		m_pInstance_ = new llaDeviceManager();

			errorhandler_builtin_ = false;
			errorHandler_ = &errhandl;


		if( m_pInstance_->setDriver(audio_driver) != E_OK) {
			delete m_pInstance_;
			m_pInstance_ = NULL;
		}

	}

	return *m_pInstance_;
}

llaDeviceManager& llaDeviceManager::getInstance( TDrivers audio_driver)
{

	if(!m_pInstance_) {
		m_pInstance_ = new llaDeviceManager();


		errorhandler_builtin_ = true;
		errorHandler_ = new llaErrorHandler();

		if( m_pInstance_->setDriver(audio_driver) != E_OK) {
			delete m_pInstance_;
			m_pInstance_ = NULL;
		}

	}

	return *m_pInstance_;
}

TErrors llaDeviceManager::setDriver(TDrivers audio_driver) {
	switch(audio_driver) {

	case DRIVER_SALSA:
		driver_ = new SalsaDriver();

		break;
	default:
		driver_ = NULL;
		break;


	}

	return E_OK;
}

TErrors llaDeviceManager::refresh() {
	return driver_->detectDevices();

}

llaDeviceManager::~llaDeviceManager() __LLATHROW
{
	//delete DRIVER_;
	if(errorhandler_builtin_) delete errorHandler_;
	m_pInstance_ = NULL;
	if(driver_ != NULL) delete driver_;

	delete fstreamlist_;
}

void llaDeviceManager::destroy(void) __LLATHROW {
	try {
		delete  m_pInstance_;
		m_pInstance_ = NULL;
	} catch (llaErrorHandler::Exception& e) {
		throw e;
	}
}

void llaDeviceManager::setErrorHandler(llaErrorHandler& handler) {
	if(errorhandler_builtin_) {
		delete errorHandler_;
		errorhandler_builtin_ = false;
		errorHandler_ = &handler;
	}
}

llaErrorHandler& llaDeviceManager::getErrorHandler(void) {
	if(errorHandler_ == NULL ) errorHandler_ = new llaErrorHandler();
	return *errorHandler_;
}

llaDeviceIterator llaDeviceManager::getDeviceIterator(void) {
	if(driver_ == NULL) { \
		LOGGER().error(E_DRIVER, "No llaudio driver is available!");\
		return llaDeviceIterator(NULL); \
	}

	return driver_->getDeviceList().getIterator();
}

llaDevice& llaDeviceManager::getDevice(TDeviceId id) {

	if(driver_ == NULL ) {
		LOGGER().error(E_DRIVER, "No llaudio driver is available!");
		return LLA_NULL_DEVICE;
	}
	llaDevice* d = driver_->getDeviceList().find(id);
	if(d == NULL ) return LLA_NULL_DEVICE;

	return *d;
}

llaDevice& llaDeviceManager::getDefaultDevice(void) {
	if(driver_ == NULL ) {
		LOGGER().error(E_DRIVER, "No llaudio driver is available!");
		return LLA_NULL_DEVICE;
	}

	llaDevice* d = driver_->getDefaultDevice();
	if(d == NULL ) return LLA_NULL_DEVICE;

	return *d;
}

llaInputStream& llaDeviceManager::getInputStream(TDeviceId device, TStreamId id) {
	if(driver_ == NULL ) {
		LOGGER().error(E_DRIVER, "No llaudio driver is available!");
		return LLA_NULL_STREAM;
	}

	return driver_->getDeviceList().find(device)->getInputStream(id);
}

llaOutputStream& llaDeviceManager::getOutputStream(TDeviceId device, TStreamId id) {
	if(driver_ == NULL ) {
		LOGGER().error(E_DRIVER, "No llaudio driver is available!");
		return LLA_NULL_STREAM;
	}

	return driver_->getDeviceList().find(device)->getOutputStream(id);
}

llaDeviceManager::llaDeviceManager() {
	fstreamlist_ = new FileStreamContainer();
}

llaFileStream& llaDeviceManager::getFileStream(const char* file) {
	llaFileStream* s = new llaFileStream(file);
	fstreamlist_->add(file, s);

	return *s;
}



