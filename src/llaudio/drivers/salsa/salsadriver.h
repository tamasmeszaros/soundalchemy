/*
 * salsadriver.h
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

#ifndef SALSADRIVER_H_
#define SALSADRIVER_H_

#include "../../llaudioprivate.h"
#include "salsastream.h"
#include "salsadevice.h"
#include <map>

namespace llaudio {
typedef int TAlsaDeviceId;

class SalsaDriver: public llaDriver {
public:
	SalsaDriver();
	virtual ~SalsaDriver();

	virtual TErrors detectDevices();
private:
	typedef std::multimap<TAlsaCardId, SalsaStream*> IStreamCache;
	typedef std::multimap<TAlsaCardId, SalsaStream*> OStreamCache;
	typedef OStreamCache::iterator TOsIt;
	typedef IStreamCache::iterator TIsIt;

	IStreamCache istreamcache_;
	OStreamCache ostreamcache_;

	TErrors findDevices(void);
	TErrors cacheStreams(void);

};

}
#endif /* SALSADRIVER_H_ */
