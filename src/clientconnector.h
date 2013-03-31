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
public:

	virtual ~ClientConnector();

	virtual void send(OutboundMessage& message) = 0;

	TAlchemyError watch(void);

	// not really functional
	void stopWatching(void);

	bool isWatching();

protected:

	ClientConnector(DspServer& server);
	DspServer * getServer();

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
	bool watching_;

	friend class DspServer;
private:
	DspServer& server_;
};


} /* namespace soundalchemy */
#endif /* CLIENTCONNECTOR_H_ */
