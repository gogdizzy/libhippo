#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#include <hippo/base/Noncopyable.h>


namespace hippo {

class EventLoop;

class EventLoopThread : Noncopyable {
public:
	typedef std::function< void( EventLoop * ) > ThreadInitCallback;

	EventLoopThread( const ThreadInitCallback &cb = ThreadInitCallback(),
	                 const std::string &name = std::string() );

	~EventLoopThread();

	EventLoop *startLoop();

private:
	void threadFunc();

	EventLoop *loop_;
	bool exiting_;
	std::thread thread_;
	std::mutex mutex_;
	std::condition_variable cond_;
	ThreadInitCallback callback_;
};

}

