/*
 * thread.h
 *
 *  Created on: Mar 13, 2013
 *      Author: Mészáros Tamás
 */

#ifndef THREAD_H_
#define THREAD_H_

#ifdef __linux__
class Pthread;
#endif

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
public:

	Thread();

	virtual ~Thread();

	virtual void waitOn(ConditionVariable& c = COND_TRUE, int timeout = -1 ) = 0;
	virtual int wakeUp(void) = 0;
	virtual int wakeUpAll(void) = 0;
	virtual void* join(Thread& thread) = 0;
	virtual int setRealtime(bool realtime = true) = 0;

	bool isRunning(void) { return running_; }

	int run(Runnable& r);

	void* getReturnValue(void) { return returnval_; }

	static Thread* getCurrent(void);
	static Thread * getNewThread(void);
	static Mutex * getMutex(int threads = 1);

protected:
	Mutex * getConditionMutex(ConditionVariable& cond);
	virtual int _run(Runnable& r) = 0;
	void* returnval_;
	bool running_;
};

} /* namespace llaudio */
#endif /* THREAD_H_ */
