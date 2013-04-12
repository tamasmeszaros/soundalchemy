/*
 * thread.cpp
 *
 *  Created on: Mar 13, 2013
 *      Author: Mészáros Tamás
 */

#include "thread.h"
#ifdef __linux__
#include <pthread.h>
#endif

namespace soundalchemy {

Thread::Thread() {
	// TODO Auto-generated constructor stub
	running_ = false;
	returnval_ = NULL;
}

Thread::~Thread() {
	// TODO Auto-generated destructor stub
}

int Thread::run(Runnable& r) {
	int ret;
	running_ = true;
	ret = _run(r);
	running_ = false;
	return ret;
}

Mutex::Mutex(unsigned int threads) {
	threads_ = threads;
}


Mutex * Thread::getConditionMutex(ConditionVariable& cond) {
	return cond.mutex_;
}


// /////////////////////////////////////////////////////////////////////////////
// Pthread implementation of the Thread interface:
// /////////////////////////////////////////////////////////////////////////////

#ifdef __linux__

class PthreadMutex: public Mutex {
public:
	PthreadMutex(int threads = 1):Mutex(threads) {

		//pthread_mutexattr_init(&mattr_);
		//pthread_mutexattr_settype(&mattr_, PTHREAD_MUTEX_FAST_NP);
		pthread_mutex_init(&mutex_, NULL);
		locked_ = false;
	}

	virtual void lock(void) {
		pthread_mutex_lock(&mutex_);
		locked_ = true;
		owner_ = pthread_self();
	}

	virtual void unlock(void) {
		pthread_mutex_unlock(&mutex_);
		locked_ = false;
	}

	bool isLockedByCurrent(void) {
		return (owner_ == pthread_self() && locked_);
	}

	~PthreadMutex() {
		pthread_mutex_destroy(&mutex_);
		//pthread_mutexattr_destroy(&mattr_);
	}

private:
	pthread_mutex_t mutex_;
	//pthread_mutexattr_t mattr_;
	friend class Pthread;
	pthread_t owner_;
};

class Pthread: public Thread {

	struct RunArg {
		Runnable *runnable;
		Pthread * thread;
	};

public:

	Pthread(): Thread() {
		thread_ = -1;
		pthread_cond_init(&wait_cond_, NULL);
		pthread_attr_init(&thread_attr_);
		pthread_attr_setdetachstate(&thread_attr_,
				PTHREAD_CREATE_JOINABLE);
	}

	~Pthread() {
		wakeUpAll();
		pthread_cond_destroy(&wait_cond_);
		pthread_attr_destroy(&thread_attr_);
	}

	virtual void waitOn(ConditionVariable& c = COND_TRUE, int timeout = -1 ) {
		pthread_mutex_t *m = getCondMutex(c);

		bool l = !getConditionMutex(c)->isLockedByCurrent();
		if(l) c.lock();
		if(c.isTrue()) {
			if(timeout > 0) {
				// TODO pthread_cond_timedwait(&wait_cond_, &wait_mutex_, NULL);
			}
			else {
				pthread_cond_wait( &wait_cond_, m);
			}
		}
		if(l) c.unlock();

	}

	virtual int _run(Runnable& r) {
		RunArg *arg = new RunArg();
		arg->runnable = &r;
		arg->thread = this;
		int ret = 0;

		if( thread_ != pthread_self())
			ret = pthread_create(&thread_, &thread_attr_, proc, arg );
		else returnval_ = r.run();

		return ret;
	}

	int wakeUp(void) {
		return pthread_cond_signal(&wait_cond_);
	}

	int wakeUpAll(void) {
		return pthread_cond_broadcast(&wait_cond_);
	}

	void* join(Thread& thread) {
		void* ret;
		pthread_join(thread_, &ret);
		return ret;
	}

	virtual int setRealtime(bool realtime = true) {
//		struct sched_param schp;
//		schp.sched_priority = 1;
//
//		if( sched_setscheduler(0, SCHED_RR, &schp) != 0 )
//		{
//			log(LEVEL_WARNING, "Cannot set real time policy!");
//		}
//		else if( sched_getscheduler(0) != SCHED_RR )
//		{
//			log(LEVEL_WARNING,"Cannot set real time policy!");
//		}

		return pthread_attr_setschedpolicy(&thread_attr_, SCHED_RR);
	}


private:

	pthread_mutex_t* getCondMutex(ConditionVariable& c) {
		PthreadMutex* m = (PthreadMutex*) getConditionMutex(c);
		return &(m->mutex_);
	}

	static void* proc(void* arg) {
		RunArg *rarg = (RunArg*) arg;
		rarg->thread->returnval_ = rarg->runnable->run();
		void* ret = rarg->thread->returnval_;
		delete rarg;
		//std::cout<< "proc end" << std::endl;
		return ret;
	}

	pthread_t thread_;
	pthread_cond_t wait_cond_;
	pthread_attr_t thread_attr_;

	friend class Thread;
};


#endif


Thread * Thread::getNewThread(void) {
#ifdef __linux__
	return new Pthread();
#endif
}

Thread * Thread::getCurrent(void) {
#ifdef __linux__
	Pthread * current = new Pthread();
	current->thread_ = pthread_self();

#endif

	return current;
}

Mutex * Thread::getMutex(int threads)  {
#ifdef __linux__
	return new PthreadMutex(threads);
#endif
}



ConditionVariable::ConditionVariable() {
	mutex_ = new PthreadMutex();
	//locked_ = false;
}

ConditionVariable::~ConditionVariable()  {
	delete mutex_;
}

ConditionVariable COND_TRUE;

} /* namespace soundalchemy */
