/*
 * lladriver.cpp
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


#include "lladriver.h"
#include "defaultcontainer.h"

using namespace llaudio;

llaDriver::llaDriver() {
	devlist_ = new DefaultDeviceContainer();
	//fstreamlist_ = new FileStreamContainer();
}

llaDriver::llaDriver(llaDeviceList *device_container) {
	devlist_ = device_container;
}

llaDriver::~llaDriver() {
	delete devlist_;
}


//llaFileStream * llaDriver::getFileStream(const char* file) {
//	return NULL;
//}


