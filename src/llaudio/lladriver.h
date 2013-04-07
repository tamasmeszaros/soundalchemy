/*
 * lladriver.h
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
#ifndef LLADRIVER_H_
#define LLADRIVER_H_
#include "predef.h"
#include "defaultcontainer.h"

using namespace std;

namespace llaudio {

class llaDriver
{
public:

	llaDriver();
	llaDriver(llaDeviceList *device_container);
	virtual ~llaDriver();

	virtual TErrors detectDevices() = 0;

	virtual llaDevice* getDefaultDevice(void) {
		return *(devlist_->getIterator());
	}

	llaDeviceList& getDeviceList(void) { return *devlist_; }


protected:

	llaDeviceList* devlist_;


};

}
#endif /* LLADRIVER_H_ */
