/*
 * message.cpp
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


#include "message.h"
#include <cstdlib>

using namespace std;
using namespace Json;


namespace soundalchemy {





//InboundMessage* soundalchemy::InboundMessage::fromJSON(JSONNode json_msg) {
//	JSONNode::const_iterator i = json_msg.begin();
//
//	TMessageType msgtype = MSG_UNINITIALIZED;
//	InboundMessage* msg;
//
//	try {
//		msgtype = (TMessageType) json_msg.at("type").as_int();
//		switch(msgtype) {
//		case MSG_START:
//		  {
//			MsgStart*  m = new MsgStart(json_msg.at("channelID").as_int());
//			msg = m;
//		  }
//		  break;
//		case MSG_STOP:
//		  {
//			MsgStop*  m = new MsgStop(json_msg.at("channelID").as_int());
//			msg = m;
//		  }
//		  break;
//		case MSG_GET_DEVICE_LIST:
//		  {
//			MsgDeviceList* m = new MsgDeviceList(json_msg.at("channelID").as_int());
//			msg = m;
//		  }
//		  break;
//		default:
//			msg = NULL;
//			break;
//		}
//
//	} catch(out_of_range& e) {
//		log(LEVEL_ERROR, e.what());
//	}

//	return msg;

//}

std::ostream* soundalchemy::OutboundMessage::serialize(TProtocol protocol) {

	switch(protocol) {
	case PROTOCOL_JSON: {
		ostream *stream = new ostream;
		setOutputStream(stream);


	}
	break;
	default:
		log(LEVEL_WARNING, "No such protocol");

		break;
	}

	return NULL;
}

InboundMessage* soundalchemy::InboundMessage::unserialize(TProtocol protocol,
		std::istream& stream) {
}

OutboundMessage* soundalchemy::MsgStart::instruct(DspServer& server) {
	server.start();
	AckStart *reply = new AckStart();
	setReply(reply);
	return reply;
}

OutboundMessage* soundalchemy::MsgStop::instruct(DspServer& server) {
	server.stop();
	AckStop *reply = new AckStop();
	setReply(reply);
	return reply;
}

OutboundMessage* soundalchemy::MsgExit::instruct(DspServer& server) {
	server.stopListening();
	OutboundMessage *reply = new OutboundMessage(Message::MSG_EXIT);
	setReply(reply);
	return reply;
}

void soundalchemy::AckDeviceList::beginDevice(const char* name,
		const char* fullname) {
}

void soundalchemy::AckDeviceList::stream(int id, const char* name, bool input) {
}

void soundalchemy::AckDeviceList::endDevice() {
}

AudioInf::DeviceInf& soundalchemy::AckDeviceList::operator ->(void) {
}

AudioInf::DeviceInf& soundalchemy::AckDeviceList::operator *(void) {
}

AudioInf& soundalchemy::AckDeviceList::operator ++(int int1) {
}

AudioInf& soundalchemy::AckDeviceList::operator ++(void) {
}

bool soundalchemy::AckDeviceList::end(void) {
}

bool soundalchemy::AckDeviceList::empty(void) {
}

void soundalchemy::AckDeviceList::begin(void) {
}

OutboundMessage* soundalchemy::MsgDeviceList::instruct(DspServer& server) {
	AckDeviceList *reply = new AckDeviceList();
	server.getDeviceList(*reply);
	setReply(reply);
	return reply;
}

}

 /* namespace soundalchemy */

