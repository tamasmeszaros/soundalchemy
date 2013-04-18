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

#include <errno.h>
#include <string.h>

namespace soundalchemy {

Thread::Thread() {
	returnval_ = NULL;
	realtime_ = false;
}

Thread::~Thread() {
}

bool Thread::isRunning(void) {
	bool r;
	running_cond_.lock();
	r = running_cond_.running_;
	running_cond_.unlock();
	return r;
}

TAlchemyError Thread::run(Runnable& r) {
	TAlchemyError ret;
	running_cond_.lock(); running_cond_.running_ = true; running_cond_.unlock();
	ret = _run(r);
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
		if(m == NULL ) return;

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

	virtual TAlchemyError _run(Runnable& r) {
		RunArg *arg = new RunArg();
		arg->runnable = &r;
		arg->thread = this;
		int ret = 0;
		TAlchemyError retval = E_OK;

		if( thread_ != pthread_self()) {
			ret = pthread_create(&thread_, &thread_attr_, proc, arg );
			if( ret == EPERM ){
				log(LEVEL_WARNING, "%s %s", STR_ERRORS[E_REALTIME], "based on EPERM");
				retval = E_REALTIME;
			}else if( ret) {
				log(LEVEL_ERROR, STR_ERRORS[E_THREAD]);
				retval = E_THREAD;
			}else if(realtime_) {

				int policy;
				sched_param p;
				p.sched_priority = 99;
				int schedret = pthread_setschedparam(thread_, SCHED_RR, &p);

					log(LEVEL_ERROR, strerror(schedret));

//				pthread_attr_getschedparam(&thread_attr_, &p);
//				pthread_attr_getschedpolicy(&thread_attr_, &policy);
				pthread_getschedparam(thread_, &policy, &p);
				if(policy != SCHED_RR || p.sched_priority != 99) {

					log(LEVEL_WARNING, "Cannot set real time priority!");
					retval = E_REALTIME;
				}
			}
		}
		else returnval_ = r.run();

		return retval;
	}

	int wakeUp(void) {
		return pthread_cond_signal(&wait_cond_);
	}

	int wakeUpAll(void) {
		return pthread_cond_broadcast(&wait_cond_);
	}

	void* join() {
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
		realtime_ = true;
		sched_param p;
		p.sched_priority = 99;
		int ret = pthread_attr_setschedpolicy(&thread_attr_, SCHED_RR);
		pthread_attr_setschedparam(&thread_attr_, &p);
		return ret;
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
		rarg->thread->running_cond_.lock();
		rarg->thread->running_cond_.running_ = false;
		rarg->thread->running_cond_.unlock();
		delete rarg;
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



ConditionVariable::ConditionVariable(): mutex_(Thread::getMutex()) {

	//locked_ = false;
}

ConditionVariable::~ConditionVariable()  {
	delete mutex_;
}

ConditionVariable COND_TRUE;

} /* namespace soundalchemy */
