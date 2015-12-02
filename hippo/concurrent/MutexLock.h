
#pragma once

#include <assert.h>
#include <pthread.h>
#include <hippo/base/noncopyable.h>

namespace hippo {

class MutexLock : noncopyable {

public:
	MutexLock()
		: holder_( 0 ) {
		pthread_mutex_init( &mutex_, NULL );
	}
	~MutexLock() {
		
	}

	bool isLockedByThisThread() const {
		return holder_ == CurrentThread::tid();
	}

	void lock() {
		pthread_mutex_lock( &mutex_ );
		assignHolder();
	}

	void unlock() {
		unassignHolder();
		pthread_mutex_unlock( &mutex_ );
	}

	pthread_mutex_t* getPthreadMutex() {
		return &mutex_;
	}

protected:
	void assignHolder() {
		holder_ = CurrentThread::tid();
	}
	void unassignHolder() {
		holder_ = 0;
	}

private:
	pthread_mutex_t mutex_;
	pid_t holder_;
};

} // namespace hippo
