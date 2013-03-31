/*
 * terminalconnector.cpp
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


#include "terminalconnector.h"
#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;

namespace soundalchemy {

class Parser;

TerminalConnector::TerminalConnector(DspServer* server) :
		ClientConnector(*server) {
	//pthread_mutex_init(&reply_mutex_, NULL);
	//pthread_cond_init(&reply_cond_, NULL);
}

TerminalConnector::~TerminalConnector() {
	//pthread_mutex_destroy(&reply_mutex_);
	//pthread_cond_destroy(&reply_cond_);
}

void TerminalConnector::waitForReply(void) {
	usleep(15000);
}

void TerminalConnector::replyReceived(void) {
	//pthread_cond_signal(&reply_cond_);
}


TAlchemyError TerminalConnector::read(Message* dest_msg) {
	string buffer;

	waitForReply();
	cout << "soundalchemy> ";
	cout.flush();

	getline(std::cin, buffer);

	if( buffer == "start") {
		*dest_msg = MSG_START;
	}
	else if( buffer == "pause" ) {
		*dest_msg = MSG_PAUSE;
	}
	else if( buffer == "stop") {
		*dest_msg = MSG_STOP;
	}
	else if( buffer == "exit") {
		*dest_msg = MSG_EXIT;
	}
	else if( buffer == "devicelist") {
		*dest_msg = MSG_GET_DEVICE_LIST;
	}
	else {
		log(LEVEL_INFO, buffer.c_str());
	}

	// it blocks and waits for the reply or for 5 seconds
	// exceptionally for this clientconnector
	waitForReply();


	return E_OK;
}

void TerminalConnector::send(Message* message) {

	string rawmsg;
	string temp;
	DspServer::TState st;

	switch( message->getType()) {
	case MSG_ACK:
		message->getElement("state", (TAlchemyInt&) st);
		switch(st) {
		case DspServer::ST_RUNNING:
			rawmsg = "Alchemy server running."; break;
		case DspServer::ST_STOPPED:
			rawmsg = "Alchemy server stopped."; break;
//		case DspServer::ST_PAUSED:
//			rawmsg = "Alchemy server paused."; break;
		default: break;
		}

		break;
	case MSG_GET_DEVICE_LIST:

		for(Message::Iterator it = message->getIterator(); !it.end(); it++) {
			message->getElement(it.getkey(), temp);
			rawmsg += string(it.getkey()) + " -> " + temp + "\n";
		}
		break;

	default:
		break;
	}

	log(LEVEL_INFO, rawmsg.c_str());

	//replyReceived();
}


} /* namespace soundalchemy */
