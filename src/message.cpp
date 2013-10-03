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
#include "message.h"
#include "logs.h"
#include "dspserver.h"
#include <cstdlib>
#include <json/json.h>

using namespace std;


namespace soundalchemy {

// /////////////////////////////////////////////////////////////////////////////
//  Message types:
//  Each message type defines an incoming (InboundMessage) and optionally an
//  outgoing (OutboundMessage) Message subclass.
// /////////////////////////////////////////////////////////////////////////////

// MSG_START ///////////////////////////////////////////////////////////////////
//

/**
 * @brief Incoming MSG_START message
 */
class MsgStart: public InboundMessage {
public:
	virtual OutboundMessage* instruct(DspServer& server) {
		const char * error = NULL;
		TAlchemyError err = server.start();
		if(err != E_OK) error = STR_ERRORS[err];
		OutboundMessage *reply =  OutboundMessage::AckStart( error );
		reply->setChannelId(getChannelId());
		setReply(reply);
		return reply;
	}
};

// MSG_STOP ////////////////////////////////////////////////////////////////////
//

/**
 * @brief Incoming MSG_STOP message
 */
class MsgStop: public InboundMessage {
public:
	OutboundMessage* instruct(DspServer& server) {
		server.stop();
		OutboundMessage *reply = OutboundMessage::AckStop(NULL);
		reply->setChannelId(getChannelId());
		setReply(reply);
		return reply;
	}

};

// MSG_EXIT ////////////////////////////////////////////////////////////////////
//

/**
 * @brief Incoming MSG_EXIT message
 */
class MsgExit: public InboundMessage {
public:
	OutboundMessage* instruct(DspServer& server) {
		server.stopListening();
		OutboundMessage *reply = OutboundMessage::AckExit();
		reply->setChannelId(getChannelId());
		setReply(reply);
		return reply;
	}

	bool isExitMessage(void) { return true; }
};


// MSG_GET_DEVICE_LIST /////////////////////////////////////////////////////////
//

/**
 * @brief Outgoing MSG_GET_DEVICE_LIST message
 */
class OutboundMsgDeviceList: public OutboundMessage, public AudioInf {
	MsgDataStore current_device_;
public:
	OutboundMsgDeviceList(): OutboundMessage(MSG_GET_DEVICE_LIST) {}

	virtual void beginDevice(const char* name, const char* fullname) {
		AudioInf::beginDevice(name, fullname);
		current_device_.clear();
		current_device_["fullname"] = getCurrent().fullname_;
		current_device_["input_streams"] = Json::Value(Json::arrayValue);
		current_device_["output_streams"] = Json::Value(Json::arrayValue);
	}

	virtual void stream(int id, const char* name, bool input)  {
		AudioInf::stream(id, name, input);
		Json::Value stream;
		stream["id"] = id;
		stream["name"] = name;
		if(input)
			current_device_["input_streams"].append(stream);
		else current_device_["output_streams"].append(stream);

	}

	virtual void endDevice() {
		dataroot_[getCurrent().name_] = current_device_;
	}

};

/**
 * @brief Incoming MSG_GET_DEVICE_LIST message
 */
class MsgGetDeviceList: public InboundMessage {
	bool do_redetect_;
public:
	MsgGetDeviceList(bool do_redetect = false): do_redetect_(do_redetect) {}
	OutboundMessage* instruct(DspServer& server) {
		OutboundMsgDeviceList *reply = new OutboundMsgDeviceList();
		reply->setChannelId(getChannelId());
		server.getDeviceList(*reply, do_redetect_);
		setReply(reply);
		return reply;
	}
};

// MSG_SEND_CLIENT_ID //////////////////////////////////////////////////////////

class MsgClientID: public OutboundMessage {
	TChannelID new_client_;
public:
	MsgClientID(TChannelID new_client):
		OutboundMessage(MSG_UNINITIALIZED),
		new_client_(new_client) {
		dataroot_["type"] = MSG_SEND_CLIENT_ID;
		dataroot_["new_client"] = (TChannelID) new_client_;
	}
};

// MSG_GET_STREAM //////////////////////////////////////////////////////////////
//
class MsgGetStream: public InboundMessage {
	TStreamDirection direction_;
public:
	MsgGetStream(TStreamDirection direction) : direction_(direction) {}

	OutboundMessage* instruct(DspServer& server) {
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
};

// MSG_SET_STREAM //////////////////////////////////////////////////////////////
//
class MsgSetStream: public InboundMessage {
	std::string device_id_;
	int stream_id_;
	TStreamDirection direction_;
public:
	MsgSetStream(const char* device_name, int id, TStreamDirection dir):
		device_id_(device_name), stream_id_(id), direction_(dir) {}

	OutboundMessage* instruct(DspServer& server) {

		TAlchemyError err = E_OK;
		const char* error = NULL;
		if( direction_ == INPUT_STREAM) {
			err = server.setInputStream(device_id_.c_str(), stream_id_);
		} else {
			err = server.setOutputStream(device_id_.c_str(), stream_id_);
		}

		if(err != E_OK ) error = STR_ERRORS[err];

		OutboundMessage * reply = OutboundMessage::AckSetStream(error);
		reply->setChannelId(getChannelId());
		return reply;
	}
};

// MSG_GET_STATE ///////////////////////////////////////////////////////////////
//
class MsgGetState: public InboundMessage {
public:
	OutboundMessage* instruct(DspServer& server) {
		OutboundMessage *reply = OutboundMessage::AckGetState(server.getState());
		reply->setChannelId(getChannelId());
		return reply;
	}
};

// MSG_CLIENT_OUT //////////////////////////////////////////////////////////////
//
class MsgClientOut: public InboundMessage {
public:
	OutboundMessage* instruct(DspServer& server) {
		server.clientOut(getChannelId());
		OutboundMessage *reply = OutboundMessage::AckClientOut();
		reply->setChannelId(getChannelId());
		return reply;
	}
};

// MSG_SET_BUFFER_SIZE /////////////////////////////////////////////////////////
//

class MsgSetBufferSize: public InboundMessage {
	unsigned int buffer_size_;
public:
	MsgSetBufferSize(unsigned int buffer_size): buffer_size_(buffer_size) {}

	OutboundMessage* instruct(DspServer& server) {
		server.setBufferSize(buffer_size_);
		OutboundMessage *reply = OutboundMessage::AckSetBufferSize();
		reply->setChannelId(getChannelId());
		return reply;
	}
};

//
// End of Message definitions //////////////////////////////////////////////////


// Alchemy Servers has channel id of 0
const Message::TChannelID Message::ALCHEMY_SERVER = 0;

// Create a serialized form of the Message driven by a specified protocol
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

// Create an input message from a byte stream
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
		msg = new MsgStart();
		break;
	case MSG_STOP:
		msg = new MsgStop();
		break;
	case MSG_GET_DEVICE_LIST:
		{
			bool redetect = jsondoc["do_redetect"].asBool();
			msg = new MsgGetDeviceList(redetect);
		}
		break;
	case MSG_GET_STATE:
		msg = new MsgGetState();
		break;
	case MSG_SET_STREAM:
		{
			TStreamDirection dir = (TStreamDirection) jsondoc["direction"].asInt();
			std::string devid = jsondoc["device_id"].asString();
			TStreamId streamid = jsondoc["stream_id"].asInt();
			msg = new MsgSetStream(devid.c_str(), streamid, dir);
		}
		break;
	case MSG_GET_STREAM:
		{
			TStreamDirection dir = (TStreamDirection) jsondoc["direction"].asInt();
			msg = new MsgGetStream(dir);
		}
		break;
	case MSG_SET_BUFFER_SIZE:
		{
			unsigned int frames = (unsigned int) jsondoc["buffer_size"].asUInt();
			msg = new MsgSetBufferSize(frames);
		}
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

// /////////////////////////////////////////////////////////////////////////////
// Input Message Initializers
// /////////////////////////////////////////////////////////////////////////////

InboundMessage* InboundMessage::newMsgClientOut() {
	return new MsgClientOut();
}

// /////////////////////////////////////////////////////////////////////////////
// Acknowledge message initializers ////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////

OutboundMessage* OutboundMessage::MsgSendClientID( Message::TChannelID id ) {
	OutboundMessage *msg = new MsgClientID(id);
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
	msg->dataroot_["direction"] = dir;

	return msg;
}

OutboundMessage* OutboundMessage::AckSetStream(const char* error) {
	OutboundMessage *msg = new OutboundMessage(MSG_SET_STREAM);
	if(error != NULL) msg->dataroot_["error"] = string(error);
	return msg;
}

OutboundMessage* OutboundMessage::AckGetState(TProcessingState state) {
	OutboundMessage *msg = new OutboundMessage(MSG_GET_STATE);
	msg->dataroot_["state"] = state;
	return msg;
}

OutboundMessage* OutboundMessage::AckClientOut( void ) {
	return new OutboundMessage(MSG_CLIENT_OUT);
}

OutboundMessage* OutboundMessage::AckSetBufferSize( void ) {
	return new OutboundMessage(MSG_SET_BUFFER_SIZE);
}

}





 /* namespace soundalchemy */

