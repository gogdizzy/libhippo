#pragma once

#include <string>
#include <vector>
#include <functional>

#include <hippo/base/Types.h>
#include <hippo/base/Noncopyable.h>

namespace hippo {


class EventLoop;

class EventLoopThread;

class EventLoopThreadPool : Noncopyable {
public:
	typedef std::function< void( EventLoop * ) > ThreadInitCallback;

	EventLoopThreadPool( EventLoop *baseLoop, const std::string &nameArg );

	~EventLoopThreadPool();

	void setThreadNum( int numThreads ) { numThreads_ = numThreads; }

	void start( const ThreadInitCallback &cb = ThreadInitCallback() );

	// valid after calling start()
	/// round-robin
	EventLoop *getNextLoop();

	/// with the same hash code, it will always return the same EventLoop
	EventLoop *getLoopForHash( size_t hashCode );

	std::vector< EventLoop * > getAllLoops();

	bool started() const { return started_; }

	const std::string &name() const { return name_; }

private:

	EventLoop *baseLoop_;
	std::string name_;
	bool started_;
	int numThreads_;
	int next_;
	std::vector< EventLoopThread * > threads_;
	std::vector< EventLoop * > loops_;
};

}
