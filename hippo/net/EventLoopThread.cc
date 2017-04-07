#include <assert.h>
#include <hippo/net/EventLoopThread.h>
#include <hippo/net/EventLoop.h>

namespace hippo {

EventLoopThread::EventLoopThread( const ThreadInitCallback &cb,
                                  const std::string &name )
		: loop_( NULL ),
		  exiting_( false ),
		  thread_(),
		  mutex_(),
		  cond_(),
		  callback_( cb ) {
}

EventLoopThread::~EventLoopThread() {
	exiting_ = true;
	if( loop_ != NULL ) // not 100% race-free, eg. threadFunc could be running callback_.
	{
		// still a tiny chance to call destructed object, if threadFunc exits just now.
		// but when EventLoopThread destructs, usually programming is exiting anyway.
		loop_->quit();
		thread_.join();
	}
}

EventLoop *EventLoopThread::startLoop() {
	assert( !thread_.joinable() );
	thread_ = std::thread( std::bind( &EventLoopThread::threadFunc, this ) );

	{
		std::unique_lock< std::mutex > lock( mutex_ );
		cond_.wait( lock, [&]() { return loop_ != nullptr; } );
	}

	return loop_;
}

void EventLoopThread::threadFunc() {
	EventLoop loop;

	if( callback_ ) {
		callback_( &loop );
	}

	{
		std::lock_guard< std::mutex > lock( mutex_ );
		loop_ = &loop;
		cond_.notify_one();
	}

	loop.loop();
	//assert(exiting_);
	loop_ = NULL;
}

}
