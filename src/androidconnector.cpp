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
#include "androidconnector.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

namespace soundalchemy {

#define ENDCHAR '\\'

AndroidConnector::~AndroidConnector() {
}

InboundMessage* AndroidConnector::read(void) {

	// read the message
	InboundMessage* dest_msg =
			InboundMessage::unserialize(Message::PROTOCOL_JSON, cin, ENDCHAR);

	if(cin.fail()) {
		dest_msg = InboundMessage::newMsgClientOut();
	}

	if(dest_msg == NULL) {
		log(LEVEL_WARNING, STR_ERRORS[E_READ]);
	}
	else dest_msg->setChannelId(getChannelId());

	return dest_msg;
}

void AndroidConnector::send(OutboundMessage& message) {
	message.serialize(Message::PROTOCOL_JSON, cout, ENDCHAR);
}

} /* namespace soundalchemy */
