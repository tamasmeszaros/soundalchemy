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
#ifndef THREAD_H_
#define THREAD_H_

#ifdef __linux__
class Pthread;
#endif

#include "logs.h"

namespace soundalchemy {

#ifndef NULL
#define NULL ((void*) 0)
#endif

class Mutex {
public:
	Mutex(unsigned int threads = 1);
	virtual ~Mutex() {}

	virtual void lock(void) = 0;
	virtual void unlock(void) = 0;
	virtual bool isLockedByCurrent(void) = 0;

	bool isLocked(void) { return locked_; }

protected:
	unsigned int threads_;
	bool locked_;
};


class Runnable {
public:
	virtual ~Runnable() {}
	virtual void* run(void) = 0;
};

class ConditionVariable {
public:
	ConditionVariable();
	virtual ~ConditionVariable();

	void lock() {  mutex_->lock(); }
	void unlock() { mutex_->unlock(); }

	virtual bool isTrue(void) { return true; }
protected:
	Mutex * mutex_;
	friend class Thread;
};

extern ConditionVariable COND_TRUE;

class Thread {
protected:
	class RunCondition: public ConditionVariable {
	public:
		bool running_;
		RunCondition():running_(false) {}
		virtual bool isTrue(void) { return running_; }
	} running_cond_;
public:

	Thread();

	virtual ~Thread();

	virtual void waitOn(ConditionVariable& c, int timeout = -1 ) = 0;
	void waitOn(int timeout = -1) {
		this->waitOn(running_cond_);
	}
	virtual int wakeUp(void) = 0;
	virtual int wakeUpAll(void) = 0;
	virtual void* join() = 0;
	virtual int setRealtime(bool realtime = true) = 0;

	bool isRunning(void);

	TAlchemyError run(Runnable& r);

	void* getReturnValue(void) { return returnval_; }

	static Thread* getCurrent(void);
	static Thread * getNewThread(void);
	static Mutex * getMutex(int threads = 1);

protected:
	Mutex * getConditionMutex(ConditionVariable& cond);
	virtual TAlchemyError _run(Runnable& r) = 0;
	void* returnval_;
	bool realtime_;

};

} /* namespace llaudio */
#endif /* THREAD_H_ */
