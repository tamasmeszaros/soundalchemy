/*
 * tinyalsadriver.h
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
 * llaalsadriver.h
 *
 *  Created on: Nov 20, 2012
 *      Author: quarky
 */

#ifndef LLTINYAALSADRIVER_H_
#define LLTINYAALSADRIVER_H_

#include "../../lladriver.h"

#define HW_BUFFER_COUNT 2

namespace llaudio {

class IStreamCache;
class OStreamCache;

class TinyalsaDriver: public llaDriver
{
public:
	TinyalsaDriver();
	virtual ~TinyalsaDriver();

private:
	enum e_errors findDevices(void);
	enum e_errors cacheStreams(void);
	static IStreamCache istreamcache_;
	static OStreamCache ostreamcache_;

};

}
#endif /* LLAALSADRIVER_H_ */
