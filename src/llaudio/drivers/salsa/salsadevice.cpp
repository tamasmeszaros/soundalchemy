/*
 * Copyright (c) 2013 Mészáros Tamás.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v2.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * 
 * Contributors:
 *     Mészáros Tamás - initial API and implementation
 */
#include "salsadevice.h"
using namespace std;
namespace llaudio {

SalsaDevice::SalsaDevice(string name, int card, string fullname) {
	// TODO Auto-generated constructor stub
	name_ = name;
	card_ = card;
	fullname_ = fullname;
}

SalsaDevice::~SalsaDevice() {
	// TODO Auto-generated destructor stub
}
}
