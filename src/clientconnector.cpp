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

////////////////////////////////////////////////////////////////////////////////
/// ClientConnector abstract class implementation
////////////////////////////////////////////////////////////////////////////////

ClientConnector::ClientConnector(std::string name): name_(name), watcher_(*this),
		watching_(false), server_(NULL) {
	thread_ = Thread::getNewThread();
	mutex_ = Thread::getMutex();
}

void ClientConnector::instructServer(InboundMessage& message) {
	server_->processMessage(message);
}

ClientConnector::~ClientConnector() {
	stopWatching();
	delete mutex_;
	delete thread_;
}

bool ClientConnector::isWatching() {
	mutex_->lock();
	bool ret = watching_;
	mutex_->unlock();
	return ret;

}

TAlchemyError ClientConnector::watch(void) {
	mutex_->lock();

	if(!watching_) {
		watching_ = true;
		int ret = thread_->run(watcher_);
		if (ret != 0) {
			mutex_->unlock();
			return E_START_LISTENER;
		}
		log(LEVEL_INFO, (std::string("Listening on ")+getName()).c_str());
	}
	mutex_->unlock();

	return E_OK;
}

void ClientConnector::stopWatching(void) {
	mutex_->lock(); watching_ = false; mutex_->unlock();
	Thread* current = Thread::getCurrent();
	thread_->join(*current);
	delete current;
}


void* ClientConnector::WatchInput::run(void)  {
	ClientConnector& self = parent_;

	while (self.isWatching()) {
		InboundMessage* message = self.read();
		if(message != NULL ) {
			self.instructServer(*message);
			if(message->isExitMessage()) {
				self.mutex_->lock();
				self.watching_ = false;
				self.mutex_->unlock();
			}
		}
	}

	log(LEVEL_INFO, (self.getName() + " Listener out").c_str());
	return NULL;
}

}

/* namespace soundalchemy */
