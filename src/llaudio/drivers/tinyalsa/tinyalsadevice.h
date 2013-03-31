/*
 * tinyalsadevice.h
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
 * tinyalsadevice.h
 *
 *  Created on: Nov 30, 2012
 *      Author: quarky
 */

#ifndef TINYALSADEVICE_H_
#define TINYALSADEVICE_H_

#include <llaudio.h>
#include "tinyalsadriver.h"
#include <cstdlib>
#include <string>
#include <map>

using namespace std;

namespace llaudio {

class TinyalsaDevice: public llaDevice
{
public:

	TinyalsaDevice(string name, int card, string fullname = "");

	const char* getName(bool full=false);

	~TinyalsaDevice();

protected:
	llaInputStream * _getInputStream(int id );

	llaOutputStream * _getOutputStream(int id );


private:

	string name_;
	string fullname_;
	int card_;

	typedef map<int, llaOutputStream* > TOstreamList;
	typedef TOstreamList::iterator TOstreamListIt;

	TOstreamList ostreams_;

	typedef map<int, llaInputStream* > TIstreamList;
	typedef TIstreamList::iterator TIstreamListIt;

	TIstreamList istreams_;

	friend class TinyalsaDriver;
};

}

#endif /* TINYALSADEVICE_H_ */
