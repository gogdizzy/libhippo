//
// Created by gogdizzy on 16/1/24.
//

#pragma once

#include <hippo/base/ScopeGuard.h>
#include <hippo/concurrent/Task.h>

namespace hippo {

struct SyncTask : public Task {

	virtual void lock() = 0;
	virtual bool isLocked() = 0;
	virtual void unlock() = 0;
	virtual void operator() () {
		ScopeGuard guard( [this]() { unlock(); } );
		runTask();
	}
	virtual void runTask() = 0;
};

}
