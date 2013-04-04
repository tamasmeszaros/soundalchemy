/*
 * message.h
 * 
 * Copyright (c) 2013 Mészáros Tamás.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 * 
 * Contributors:
 *     Mészáros Tamás - initial API and implementation
 *
 */
#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <map>
#include <list>
#include <string>
#include <cstdio>
#include <json/json.h>

namespace soundalchemy {

class DspServer;

/**
 * @brief Base class for a Message
 *
 * Sound Alchemy can be built as a stand-alone service and the communication
 * mechanism with various front-ends is based on Message objects. Two types
 * of Message are defined, the InboundMessage for incoming messages and the
 * OutboundMessage for outgoing messages.
 */
class Message {
protected:

	// TODO replace with an abstract class which inherits Json::Value.
	// This is a  general purpose data store for message data.
	typedef Json::Value MsgDataStore;

	/**
	 * Each message type has a named constant identifier. It stands for a
	 * specific command to the server. The parameters of each operation are
	 * defined in the appropriate subclasses of InboundMessage and
	 * OutboundMessage base classes.
	 */
	typedef enum e_messages {
		MSG_UNINITIALIZED,      //!< A generic uninitialized message
		MSG_ACK,                //!< The ID of outgoing messages
		MSG_START,              //!< Start the sound processing.
		MSG_STOP,               //!< Stop the sound processing
		MSG_EXIT,               //!< Kill the alchemy service
		MSG_GET_DEVICE_LIST,    //!< Obtain information about sound devices
		MSG_SET_INPUT_STREAM,   //!< Set the audio input for processing
		MSG_SET_OUTPUT_STREAM,  //!< Set the audio output for processing
		MSG_GET_INPUT_STREAM,
		MSG_GET_OUTPUT_STREAM,
		MSG_GET_EFFECT_DATABASE,//!< Get the database of available effects
		MSG_ADD_EFFECT,		    //!< Add an effect to the signal chain
		MSG_REMOVE_EFFECT,      //!< Remove an effect from the signal chain
		MSG_SET_EFFECT_PARAM,   //!< Set a parameter of a specific effect
		MSG_GET_EFFECT_PARAM,	//!< Get a parameter of a specific effect
		MSG_GET_STATE,          //!< Obtain the processing state
		MSG_SEND_CLIENT_ID,     //!< Sending a channelID to a new client
	} TMessageType;

public:

	/**
	 * ID of the front-end sending the message
	 */
	typedef unsigned int TChannelID;
	static const TChannelID ALCHEMY_SERVER;

private:
	TChannelID channel_;

public:

	/**
	 * The concrete protocol for serialization. This can be extended in the
	 * future
	 */
	typedef enum {
		PROTOCOL_JSON,//!< JSON protocol
		PROTOCOL_MIDI //!< MIDI control protocol
	} TProtocol;

	/**
	 * @brief Default constructor
	 * @param channel An optional ID of the sender front-end. If points to
	 * Alchemy Server if omitted.
	 */
	Message():channel_(ALCHEMY_SERVER) {}
	virtual ~Message() {}

	/**
	 * @brief Get the ID of the sender front-end.
	 * @return Returns a TChannel value.
	 */
	TChannelID getChannelId(void) { return channel_; }

	/**
	 * @brief Get the ID of the sender front-end.
	 * @param channel
	 */
	virtual void setChannelId(TChannelID channel) { channel_ = channel; }

};

/**
 * @brief Base class for an outgoing message.
 */
class OutboundMessage: public Message {
protected:
	TMessageType ack_for_;
	MsgDataStore dataroot_;

public:

	/// initializers
	static OutboundMessage* MsgSendClientID( TChannelID id );
	static OutboundMessage* AckStart( const char* error);
	static OutboundMessage* AckStop( const char* error );
	static OutboundMessage* AckExit( void );
	static OutboundMessage* AckGetInputStream(const char* device_name, int id);
	static OutboundMessage* AckSetInputStream(const char* error);


	virtual ~OutboundMessage() {}

	/**
	 * @brief Convert the message to a serialized byte stream.
	 *
	 * The message is converted to a byte stream conforming to the parameter
	 * protocol and the stream is written to the given output stream.
	 *
	 * @param protocol Specifies the protocol of the byte stream
	 * @param output An output stream where the raw message will be sent
	 * @param delimiter Specifies the delimiter character after the byte stream
	 */
	virtual void serialize(TProtocol protocol, std::ostream& output,
			int delimiter = EOF);

	virtual void setChannelId(TChannelID chid) {
		Message::setChannelId(chid);
		dataroot_["channelID"] = getChannelId();
	}

protected:

	OutboundMessage(TMessageType reply_for):
			ack_for_(reply_for) {
		dataroot_["type"] = MSG_ACK;
		dataroot_["channelID"] = getChannelId();
		dataroot_["ack_for"] = reply_for;
	}

};

/**
 * @brief Base class for incoming messages
 */
class InboundMessage: public Message {
	OutboundMessage* reply_;
protected:
	void setReply(OutboundMessage* reply) { delete reply_; reply_ = reply; };

public:
	InboundMessage() { reply_ = NULL; }

	virtual ~InboundMessage() { delete reply_; }

	/**
	 * @brief Runs the defined command on he specified DspServer object.
	 *
	 * This method has a key role in the communication mechanism. An incoming
	 * message represents a command to a DspServer. The method has to be invoked
	 * to run this this command on a specific DspServer object.
	 *
	 * Subclasses have to implement this method by returning their appropriate
	 * outgoing message which has to be a subclass of OutboundMessage
	 *
	 * The setReply() method has to be called on the reply object before
	 * returning it.
	 *
	 * @param
	 * @return Returns an OutboundMessage.
	 */
	virtual OutboundMessage* instruct(DspServer& server) = 0;

	/**
	 * @brief Creates an InboundMessage from the specified input stream
	 * @param protocol Specifies the format of the input stream data
	 * @param stream An input stream object from the standard C++ library
	 * @return Returns a subclass of InboundMessage
	 */
	static InboundMessage* unserialize(TProtocol protocol,
			std::istream& stream, int delimiter = EOF);


	virtual bool isExitMessage(void) { return false; }
};

////////////////////////////////////////////////////////////////////////////////
/// Message types:
/// Each message type defines an incoming (InboundMessage) and an outgoing
/// (OutboundMessage) message subclass.
////////////////////////////////////////////////////////////////////////////////

// MSG_START ///////////////////////////////////////////////////////////////////
//

/**
 * @brief Incoming MSG_START message
 */
class MsgStart: public InboundMessage {
public:
	virtual OutboundMessage* instruct(DspServer& server);
};

// MSG_STOP ////////////////////////////////////////////////////////////////////
//

/**
 * @brief Incoming MSG_STOP message
 */
class MsgStop: public InboundMessage {
public:
	OutboundMessage* instruct(DspServer& server);

};

// MSG_EXIT ////////////////////////////////////////////////////////////////////
//

/**
 * @brief Incoming MSG_EXIT message
 */
class MsgExit: public InboundMessage {
public:
	OutboundMessage* instruct(DspServer& server);

	bool isExitMessage(void) { return true; }
};

// MSG_GET_DEVICE_LIST /////////////////////////////////////////////////////////
//

/**
 * @brief Definition of a data structure interface for the audio devices
 *
 * It represents a list of device ( Identifier, full name ) pairs. Additionally,
 * all device has a list of input and output streams. A stream is an (id, name)
 * pair.
 *
 * The data structures are provided for reference. Subclasses can define their
 * own.
 */
class AudioInf {

	struct StreamInf {
		std::string name_;
		int id_;
		StreamInf(const char* name, int id):name_(name), id_(id) {}
	};

	class DeviceInf {
	public:
		std::list<StreamInf> istreaminf_;
		std::list<StreamInf> ostreaminf_;
		std::string name_;
		std::string fullname_;

		DeviceInf(const char* name, const char* fullname):name_(name),
			fullname_(fullname) {}

		void addStream(StreamInf inf, bool input) {
			if(input) istreaminf_.push_back(inf);
			else ostreaminf_.push_back(inf);
		}
	};

	std::list<DeviceInf*> devinf_;
	DeviceInf* current_;
	std::list<DeviceInf*>::iterator it_;

public:
	AudioInf() { it_ = devinf_.begin(); }

	virtual ~AudioInf() {
		for(std::list<DeviceInf*>::iterator it = devinf_.begin();
				it != devinf_.end(); it++)
			delete *it;
	}

	/**
	 *
	 * @param name
	 * @param fullname
	 */
	virtual void beginDevice(const char* name, const char* fullname) {
		current_ = new DeviceInf(name, fullname);
	}

	/**
	 *
	 * @param id
	 * @param name
	 * @param input
	 */
	virtual void stream(int id, const char* name, bool input) {
		current_->addStream(StreamInf( name, id), input);
	}

	DeviceInf& getCurrent(void) { return *current_; }

	/**
	 *
	 */
	virtual void endDevice() { devinf_.push_back(current_); }

	/**
	 *
	 * @return
	 */
	virtual DeviceInf& operator->(void) {
		return *(*it_);
	}

	/**
	 *
	 * @return
	 */
	virtual DeviceInf& operator*(void) {
		return *(*it_);
	}

	/**
	 *
	 * @param
	 * @return
	 */
	virtual AudioInf& operator++(int) {
		it_++;
		return *this;
	}

	/// operators
	virtual AudioInf& operator++(void) { return (*this)++; }
	virtual bool end(void) { return it_ == devinf_.end(); }
	virtual bool empty(void) { return devinf_.empty(); }
	virtual void begin(void) { it_ = devinf_.begin(); }

};

/**
 * @brief Outgoing MSG_GET_DEVICE_LIST message
 */
class OutboundMsgDeviceList: public OutboundMessage, public AudioInf {
	MsgDataStore current_device_;
public:
	OutboundMsgDeviceList(): OutboundMessage(MSG_GET_DEVICE_LIST) {}

	virtual void beginDevice(const char* name, const char* fullname);

	virtual void stream(int id, const char* name, bool input);

	virtual void endDevice();

};

/**
 * @brief Incoming MSG_GET_DEVICE_LIST message
 */
class MsgDeviceList: public InboundMessage {
public:
	OutboundMessage* instruct(DspServer& server);
};

// MSG_SEND_CLIENT_ID //////////////////////////////////////////////////////////

class MsgClientID: public OutboundMessage {
	TChannelID new_client_;
public:
	MsgClientID(TChannelID new_client):
		OutboundMessage(MSG_UNINITIALIZED),
		new_client_(new_client) {
		dataroot_["clientID"] = (TChannelID) new_client_;
	}
};

// MSG_SET_INPUT_STREAM ////////////////////////////////////////////////////////
//

class MsgGetInputStream: public InboundMessage {
public:
	OutboundMessage* instruct(DspServer& server);
};

// MSG_SET_INPUT_STREAM ////////////////////////////////////////////////////////
//

class MsgSetInputStream: public InboundMessage {
	std::string device_name_;
	int id_;
public:
	MsgSetInputStream(const char* device_name, int id):
		device_name_(device_name), id_(id) {}

	OutboundMessage* instruct(DspServer& server);
};



} /* namespace soundalchemy */
#endif /* MESSAGE_H_ */
