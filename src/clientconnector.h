/*
 * clientconnector.h
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
#ifndef CLIENTCONNECTOR_H_
#define CLIENTCONNECTOR_H_

#include "logs.h"
#include "dspserver.h"
#include "message.h"
#include "thread.h"

namespace soundalchemy {
class DspServer;


class ClientConnector {
	std::string name_;
	Message::TChannelID channel_id_;

public:

	ClientConnector(std::string name);
	virtual ~ClientConnector();

	virtual void send(OutboundMessage& message) = 0;

	virtual TAlchemyError watch(void);

	virtual void stopWatching(void);

	virtual bool isWatching();

	std::string& getName() { return name_; }
	void setChannelId(Message::TChannelID chid) { channel_id_ = chid; }

protected:

	Message::TChannelID getChannelId(void) { return channel_id_; }

	void connect(DspServer& server) { server_ = &server; }

	DspServer* getServer() {  return server_; }

	void instructServer(InboundMessage& message);
	virtual InboundMessage* read(void) = 0;

	class WatchInput: public Runnable {
	public:
		WatchInput(ClientConnector& parent):parent_(parent) { }

		void* run(void);
	private:
		ClientConnector& parent_;
	} watcher_;


	Thread *thread_;
	Mutex * mutex_;

	bool watching_;

	friend class DspServer;

private:
	DspServer* server_;
};


} /* namespace soundalchemy */
#endif /* CLIENTCONNECTOR_H_ */
