
#pragma once

#include <hippo/concurrent/MutexLock.h>

namespace hippo {

class MutexLockGuard {

public:
	explicit MutexLockGuard( MutexLock& lock )
		: lock_( lock ) {
	}

	~MutexLockGuard() {
		lock_.unlock();
	}

private:
	MutexLock& lock_;

};

} // namespace hippo
