/*
 * androidconnector.h
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
#ifndef ANDROIDCONNECTOR_H_
#define ANDROIDCONNECTOR_H_

#include "clientconnector.h"


using namespace std;

namespace soundalchemy {

class AndroidConnector: public ClientConnector {
public:

	AndroidConnector():ClientConnector("Android") {}
	virtual ~AndroidConnector();

	virtual void send(OutboundMessage& message);

protected:

	virtual InboundMessage* read(void);

};

} /* namespace soundalchemy */
#endif /* ANDROIDCONNECTOR_H_ */
