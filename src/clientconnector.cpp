/*
 * clientconnector.cpp
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


#include "clientconnector.h"
#include <signal.h>
#include <cstring>

namespace soundalchemy {

/*
 * ClientConnector abstract class implementation *******************************
 */

ClientConnector::ClientConnector(DspServer& server):watcher_(*this),
		server_(server) {
	thread_ = Thread::getNewThread();
}

void ClientConnector::instructServer(InboundMessage& message) {
	server_.processMessage(message);
}

ClientConnector::~ClientConnector() {
	// TODO Auto-generated destructor stub
	watching_ = false;
	delete thread_;
}

bool ClientConnector::isWatching() {
	bool ret;
	ret = watching_;
	return ret;
}

TAlchemyError ClientConnector::watch(void) {
	watching_ = true;


	int ret = thread_->run(watcher_);
	log(LEVEL_INFO, "Listening on cli");

	if (ret != 0)
		return E_START_LISTENER;

	return E_OK;
}

void ClientConnector::stopWatching(void) {

	watching_ = false;

	//pthread_join(thread_id_, NULL);

}
void* ClientConnector::WatchInput::run(void)  {
	ClientConnector& self = parent_;

	while (self.watching_) {
		InboundMessage* message = self.read();
		self.instructServer(*message);
		// TODO check the message for validity
	}

	log(LEVEL_INFO, "Listener out");
	return NULL;
}

}

/* namespace soundalchemy */
