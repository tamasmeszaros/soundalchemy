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
#include "logs.h"
#include "dspserver.h"
#include <cstdlib>
#include <json/json.h>

using namespace std;


namespace soundalchemy {

const Message::TChannelID Message::ALCHEMY_SERVER = 0;

void soundalchemy::OutboundMessage::serialize(TProtocol protocol,
		std::ostream& output, int delimiter) {

	switch(protocol) { // decide which protocol to use
	case PROTOCOL_JSON: {

		// send out the JSON data
		output << Json::FastWriter().write(dataroot_) << (char) delimiter;
		output.flush();
	}
	break;
	default:
		log(LEVEL_WARNING, "No such protocol");

		break;
	}

}

InboundMessage* soundalchemy::InboundMessage::unserialize(TProtocol protocol,
		std::istream& stream, int delimiter) {

	TMessageType msgtype = MSG_UNINITIALIZED;
	InboundMessage* msg;
	Json::Reader reader;
	Json::Value jsondoc;
	string buffer;


	// get the contents of the stream to the char '<delimiter>'
	std::getline(stream, buffer, (char) delimiter);

	// parse the content
	reader.parse(buffer, jsondoc, false);

	// get the Message type from the json data
	msgtype = (TMessageType) jsondoc["type"].asInt();

	switch(msgtype) { // create the message
	case MSG_START:
		msg = new MsgStart( );
		break;
	case MSG_STOP:
		msg = new MsgStop( );
		break;
	case MSG_GET_DEVICE_LIST:
		msg = new MsgDeviceList( );
		break;
	case MSG_EXIT:
		msg = new MsgExit( );
		break;
	default:
		msg = NULL;
		break;
	}

	return msg;
}

OutboundMessage* soundalchemy::MsgStart::instruct(DspServer& server) {
	const char * error = NULL;
	TAlchemyError err = server.start();
	if(err != E_OK) error = STR_ERRORS[err];
	OutboundMessage *reply =  OutboundMessage::AckStart( error );
	reply->setChannelId(getChannelId());
	setReply(reply);
	return reply;
}

OutboundMessage* soundalchemy::MsgStop::instruct(DspServer& server) {
	server.stop();
	OutboundMessage *reply = OutboundMessage::AckStop(NULL);
	reply->setChannelId(getChannelId());
	setReply(reply);
	return reply;
}

OutboundMessage* soundalchemy::MsgExit::instruct(DspServer& server) {
	server.stopListening();
	OutboundMessage *reply = OutboundMessage::AckExit();
	reply->setChannelId(getChannelId());
	setReply(reply);
	return reply;
}


OutboundMessage* MsgGetStream::instruct(DspServer& server) {
	OutboundMessage* reply;
	if(direction_ == INPUT_STREAM ) {
	reply = OutboundMessage::AckGetStream(
			server.getInputStream().getOwner().getName(),
			server.getInputStream().getId(),
			direction_
			);
	} else {
		reply = OutboundMessage::AckGetStream(
			server.getOutputStream().getOwner().getName(),
			server.getOutputStream().getId(),
			direction_
			);
	}
	reply->setChannelId(getChannelId());
	setReply(reply);
	return reply;
}

OutboundMessage* MsgSetStream::instruct(DspServer& server) {

	TAlchemyError err = E_OK;
	const char* error = NULL;
	if( direction_ ) {
		err = server.setInputStream(device_name_.c_str(), id_);
	} else {
		err = server.setOutputStream(device_name_.c_str(), id_);
	}

	if(err != E_OK ) error = STR_ERRORS[err];

	OutboundMessage * reply = OutboundMessage::AckSetStream(error);
	reply->setChannelId(getChannelId());
	return reply;
}

void soundalchemy::OutboundMsgDeviceList::beginDevice(const char* name,
		const char* fullname) {
	AudioInf::beginDevice(name, fullname);
	current_device_.clear();
	current_device_["input_streams"] = Json::Value(Json::arrayValue);
	current_device_["output_streams"] = Json::Value(Json::arrayValue);
}

void soundalchemy::OutboundMsgDeviceList::stream(int id, const char* name, bool input) {
	AudioInf::stream(id, name, input);
	Json::Value stream;
	stream[Json::valueToString(id)] = string(name);
	if(input)
		current_device_["input_streams"].append(stream);
	else current_device_["output_streams"].append(stream);

}

void soundalchemy::OutboundMsgDeviceList::endDevice() {
	dataroot_[getCurrent().fullname_] = current_device_;
}


OutboundMessage* soundalchemy::MsgDeviceList::instruct(DspServer& server) {
	OutboundMsgDeviceList *reply = new OutboundMsgDeviceList();
	reply->setChannelId(getChannelId());
	server.getDeviceList(*reply);
	setReply(reply);
	return reply;
}

// /////////////////////////////////////////////////////////////////////////////
// Acknowledge message initializers ////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////

OutboundMessage* OutboundMessage::MsgSendClientID( Message::TChannelID id ) {
	OutboundMessage *msg = new MsgClientID(MSG_SEND_CLIENT_ID);
	msg->dataroot_["channel_alloced"] = id;
	return msg;
}

OutboundMessage* OutboundMessage::AckStart( const char* error) {
	OutboundMessage *msg = new OutboundMessage(MSG_START);
	if( error != NULL ) msg->dataroot_["error"] = std::string(error);
	return msg;
}

OutboundMessage* OutboundMessage::AckStop( const char* error) {
	OutboundMessage *msg = new OutboundMessage(MSG_STOP);
	if( error != NULL ) msg->dataroot_["error"] = std::string(error);
	return msg;
}


OutboundMessage* OutboundMessage::AckExit( void ) {
	return new OutboundMessage(MSG_EXIT);
}

OutboundMessage* OutboundMessage::AckGetStream(const char* device_name, int id,
		TStreamDirection dir) {
	OutboundMessage *msg = new OutboundMessage(MSG_GET_STREAM);
	msg->dataroot_["device_name"] = string(device_name);
	msg->dataroot_["id"] = id;
	if(dir == INPUT_STREAM)
		msg->dataroot_["direction"] = string("input");
	else msg->dataroot_["direction"] = string("output");

	return msg;
}

OutboundMessage* OutboundMessage::AckSetStream(const char* error) {
	OutboundMessage *msg = new OutboundMessage(MSG_SET_STREAM);
	if(error != NULL) msg->dataroot_["error"] = string(error);
	return msg;
}

}





 /* namespace soundalchemy */

