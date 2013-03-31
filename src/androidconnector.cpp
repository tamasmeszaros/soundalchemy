/*
 * androidconnector.cpp
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


#include "androidconnector.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

namespace soundalchemy {

#define ENDCHAR '\x04'

AndroidConnector::~AndroidConnector() {
}


//void _parseJSON(const JSONNode & n, Message& msg){
//    JSONNode::const_iterator i = n.begin();
//    while (i != n.end()){
//
//        if (i -> type() == JSON_ARRAY ) {
//        	msg.beginArray(i->name().c_str());
//        	_parseJSON(*i, msg);
//        	msg.endArray();
//        }
//        else if(i -> type() == JSON_NODE) {
//        	msg.beginObject(i->name().c_str());
//        	_parseJSON(*i, msg);
//        	msg.endObject();
//        } else {
//
//			// get the node name and value as a string
//			std::string node_name = i -> name();
//
//			// find out where to store the values
//			if (node_name == "type"){
//				msg.changeType((TMessageType) i -> as_int() );
//			}
//			else if (node_name == "channelID")
//				msg.setChannelId((TChannelID)i -> as_int());
//			else {
//				msg.addElement(node_name.c_str(), Message::DATA(i->as_string()));
//			}
//
//			//increment the iterator
//			++i;
//        }
//    }
//}

InboundMessage* AndroidConnector::read(void) {
	string buffer;
	InboundMessage* dest_msg;
	int c;
	while ( (c=cin.get()) != ENDCHAR && c!='\\' && !cin.fail() && !cin.eof() ) {
		buffer.push_back(c);
	}

	//JSONNode node = libjson::parse(buffer);

	//dest_msg = InboundMessage::fromJSON(node);
	return dest_msg;
}
//void _serialize(JSONNode& n, Message::Iterator& it);
//void _serializeArray(JSONNode& n, Message::ArrayIterator& it) {
//
//	JSONNode a(JSON_ARRAY);
//	a.set_name(it.getKey());
//
//	for( ; !it.end(); it++ ) {
//		TAlchemyString as;
//		TAlchemyFloat af;
//		TAlchemyInt ai;
//		switch(it.getDataType()) {
//		case MDT_FLOAT:
//			it.get( af );
//			a.push_back( JSONNode("", af) );
//			break;
//		case MDT_INTEGER:
//			it.get( ai);
//			a.push_back(JSONNode("", ai));
//			break;
//		case MDT_STRING:
//			it.get( as);
//			a.push_back(JSONNode("", as));
//			break;
//		case MDT_ARRAY:
//			{
//			Message::ArrayIterator ai = it.getArrayIterator();
//			_serializeArray(a, ai);
//			}
//			break;
//
//		case MDT_OBJECT:
//			{Message::Iterator oi = it.getObjectIterator();
//			JSONNode object;
//			_serialize(object, oi);
//			a.push_back(object);}
//			break;
//		}
//
//	}
//	n.push_back( a );
//}
//
//void _serialize(JSONNode& n, Message::Iterator& it) {
//
//	for(; !it.end(); it++) {
//		TAlchemyString as;
//		TAlchemyFloat af;
//		TAlchemyInt ai;
//		switch(it->getDataType()) {
//		case MDT_FLOAT:
//			it.get( af );
//			n.push_back( JSONNode(it.getkey(), af) );
//			break;
//		case MDT_INTEGER:
//			it.get( ai);
//			n.push_back(JSONNode(it.getkey(), ai));
//			break;
//		case MDT_STRING:
//			it.get( as);
//			n.push_back(JSONNode(it.getkey(), as));
//			break;
//		case MDT_ARRAY:
//		{
//			Message::ArrayIterator ai = it.getArrayIterator();
//			_serializeArray(n, ai);
//		}
//			break;
//
//		case MDT_OBJECT:
//			{Message::Iterator oi = it.getObjectIterator();
//			JSONNode newnode;
//			newnode.set_name(it.getkey());
//			_serialize(newnode, oi);
//			n.push_back(newnode);}
//			break;
//		}
//
//	}
//}
//string AndroidConnector::serialize(Message& msg) {
//
//
//	JSONNode n(JSON_NODE);
//	n.push_back(JSONNode("type", msg.getType()));
//	n.push_back(JSONNode("channelID", msg.getChannelId()));
//
//	Message::Iterator it = msg.getIterator();
//	_serialize(n, it);
//
//	string jc = n.write_formatted();
//	return jc;
//}


void AndroidConnector::send(OutboundMessage& message) {
	//cout << message.toJSON().write_formatted()  << ENDCHAR;
	//cout.flush();
}

} /* namespace soundalchemy */
