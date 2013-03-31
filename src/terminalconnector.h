/*
 * terminalconnector.h
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
#ifndef ANDROIDCONNECTOR_H_
#define ANDROIDCONNECTOR_H_

#include "clientconnector.h"

namespace soundalchemy {

class TerminalConnector: public ClientConnector {
public:
	TerminalConnector(DspServer *server);
	virtual ~TerminalConnector();

	virtual void send(Message* message);


private:
	virtual TAlchemyError read(Message* dest_msg);
	void waitForReply(void);
	void replyReceived(void);

	//pthread_mutex_t reply_mutex_;
	//pthread_cond_t reply_cond_;
};

} /* namespace soundalchemy */
#endif /* ANDROIDCONNECTOR_H_ */
