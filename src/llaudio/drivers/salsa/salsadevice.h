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
#ifndef SALSADEVICE_H_
#define SALSADEVICE_H_

#include "../../llaudioprivate.h"
#include "../../defaultcontainer.h"
#include <string>

namespace llaudio {

typedef int TAlsaCardId;

class SalsaDevice: public llaudio::llaDevice {
public:
	SalsaDevice(std::string name, int card, std::string fullname = "");
	~SalsaDevice();

	IStreamList * getInputList(void) { return input_stream_list_; }
	OStreamList * getOutputList(void) { return output_stream_list_; }


	/* Implementable methods from the interface: */
	const char* getName(bool full = false) {
		if(full) return fullname_.c_str();
		return name_.c_str();
	}

	TAlsaCardId getId(void) { return card_; }

private:


	friend class SalsaDriver;

	std::string name_;
	std::string fullname_;
	int card_;

};
}
#endif /* SALSADEVICE_H_ */
