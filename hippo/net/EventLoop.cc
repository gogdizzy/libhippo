
#include <signal.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/eventfd.h>
#endif

#include <algorithm>
#include <functional>
#include <mutex>

#include <hippo/net/EventLoop.h>
#include <hippo/util/Logging.h>
#include <hippo/net/Channel.h>
#include <hippo/net/Poller.h>
#include <hippo/net/SocketsOps.h>
#include <hippo/net/TimerQueue.h>


namespace hippo {

namespace {
__thread EventLoop *t_loopInThisThread = 0;

const int kPollTimeMs = 10000;

int createEventfd() {
#ifdef __linux__
	int evtfd = ::eventfd( 0, EFD_NONBLOCK | EFD_CLOEXEC );
#else
	int evtfd = -1;
#endif

	if( evtfd < 0 ) {
		HIPPO_ERROR( "Failed in eventfd" );
		abort();
	}
	return evtfd;
}

class IgnoreSigPipe {
public:
	IgnoreSigPipe() {
		::signal( SIGPIPE, SIG_IGN );
	}
};

IgnoreSigPipe initObj;
}

EventLoop *EventLoop::getEventLoopOfCurrentThread() {
	return t_loopInThisThread;
}

EventLoop::EventLoop()
		: looping_( false ),
		  quit_( false ),
		  eventHandling_( false ),
		  callingPendingFunctors_( false ),
		  iteration_( 0 ),
		  threadId_( std::this_thread::get_id() ),
		  poller_( Poller::newDefaultPoller( this ) ),
		  timerQueue_( new TimerQueue( this ) ),
		  wakeupFd_( createEventfd() ),
		  wakeupChannel_( new Channel( this, wakeupFd_ ) ),
		  currentActiveChannel_( NULL ) {
	HIPPO_DEBUG( "EventLoop created {} in thread {}", (void*)this, threadId_ );
	if( t_loopInThisThread ) {
		HIPPO_ERROR( "Another EventLoop {} exists in this thread {}", (void*)t_loopInThisThread, threadId_ );
	}
	else {
		t_loopInThisThread = this;
	}
	wakeupChannel_->setReadCallback(
			std::bind( &EventLoop::handleRead, this ) );
	// we are always reading the wakeupfd
	wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
	HIPPO_DEBUG( "EventLoop {} of thread {} destructs in thread {}",
	             (void*)this, threadId_, std::this_thread::get_id() );
	wakeupChannel_->disableAll();
	wakeupChannel_->remove();
	::close( wakeupFd_ );
	t_loopInThisThread = NULL;
}

void EventLoop::loop() {
	assert( !looping_ );
	assertInLoopThread();
	looping_ = true;
	quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
	HIPPO_INFO( "EventLoop {} start looping", (void*)this );

	while( !quit_ ) {
		activeChannels_.clear();
		pollReturnTime_ = poller_->poll( kPollTimeMs, &activeChannels_ );
		++iteration_;
		// if( Logger::logLevel() <= Logger::TRACE ) {
		//	printActiveChannels();
		// }
		// TODO sort channel by priority
		eventHandling_ = true;
		for( ChannelList::iterator it = activeChannels_.begin();
		     it != activeChannels_.end(); ++it ) {
			currentActiveChannel_ = *it;
			currentActiveChannel_->handleEvent( pollReturnTime_ );
		}
		currentActiveChannel_ = NULL;
		eventHandling_ = false;
		doPendingFunctors();
	}

	HIPPO_INFO( "EventLoop {} stop looping", (void*)this );
	looping_ = false;
}

void EventLoop::quit() {
	quit_ = true;
	// There is a chance that loop() just executes while(!quit_) and exists,
	// then EventLoop destructs, then we are accessing an invalid object.
	// Can be fixed using mutex_ in both places.
	if( !isInLoopThread() ) {
		wakeup();
	}
}

void EventLoop::runInLoop( const Functor &cb ) {
	if( isInLoopThread() ) {
		cb();
	}
	else {
		queueInLoop( cb );
	}
}

void EventLoop::queueInLoop( const Functor &cb ) {
	{
		std::lock_guard< std::mutex > lock( mutex_ );
		pendingFunctors_.push_back( cb );
	}

	if( !isInLoopThread() || callingPendingFunctors_ ) {
		wakeup();
	}
}

TimerId EventLoop::runAt( const Timestamp &time, const TimerCallback &cb ) {
	return timerQueue_->addTimer( cb, time, 0.0 );
}

TimerId EventLoop::runAfter( double delay, const TimerCallback &cb ) {
	Timestamp time( addTime( Timestamp::now(), delay ) );
	return runAt( time, cb );
}

TimerId EventLoop::runEvery( double interval, const TimerCallback &cb ) {
	Timestamp time( addTime( Timestamp::now(), interval ) );
	return timerQueue_->addTimer( cb, time, interval );
}

// FIXME: remove duplication
void EventLoop::runInLoop( Functor &&cb ) {
	if( isInLoopThread() ) {
		cb();
	}
	else {
		queueInLoop( std::move( cb ) );
	}
}

void EventLoop::queueInLoop( Functor &&cb ) {
	{
		std::lock_guard< std::mutex > lock( mutex_ );
		pendingFunctors_.push_back( std::move( cb ) );  // emplace_back
	}

	if( !isInLoopThread() || callingPendingFunctors_ ) {
		wakeup();
	}
}

TimerId EventLoop::runAt( const Timestamp &time, TimerCallback &&cb ) {
	return timerQueue_->addTimer( std::move( cb ), time, 0.0 );
}

TimerId EventLoop::runAfter( double delay, TimerCallback &&cb ) {
	Timestamp time( addTime( Timestamp::now(), delay ) );
	return runAt( time, std::move( cb ) );
}

TimerId EventLoop::runEvery( double interval, TimerCallback &&cb ) {
	Timestamp time( addTime( Timestamp::now(), interval ) );
	return timerQueue_->addTimer( std::move( cb ), time, interval );
}

void EventLoop::cancel( TimerId timerId ) {
	return timerQueue_->cancel( timerId );
}

void EventLoop::updateChannel( Channel *channel ) {
	assert( channel->ownerLoop() == this );
	assertInLoopThread();
	poller_->updateChannel( channel );
}

void EventLoop::removeChannel( Channel *channel ) {
	assert( channel->ownerLoop() == this );
	assertInLoopThread();
	if( eventHandling_ ) {
		assert( currentActiveChannel_ == channel ||
		        std::find( activeChannels_.begin(), activeChannels_.end(), channel ) == activeChannels_.end() );
	}
	poller_->removeChannel( channel );
}

bool EventLoop::hasChannel( Channel *channel ) {
	assert( channel->ownerLoop() == this );
	assertInLoopThread();
	return poller_->hasChannel( channel );
}

void EventLoop::abortNotInLoopThread() {
	HIPPO_ERROR( "EventLoop::abortNotInLoopThread - EventLoop {} "
			" was created in threadId_ = "
			", current thread id = ",
	        (void*)this, threadId_, std::this_thread::get_id() );
}

void EventLoop::wakeup() {
	uint64_t one = 1;
	ssize_t n = sockets::write( wakeupFd_, &one, sizeof one );
	if( n != sizeof one ) {
		HIPPO_ERROR( "EventLoop::wakeup() writes {} bytes instead of 8", n );
	}
}

void EventLoop::handleRead() {
	uint64_t one = 1;
	ssize_t n = sockets::read( wakeupFd_, &one, sizeof one );
	if( n != sizeof one ) {
		HIPPO_ERROR( "EventLoop::handleRead() reads {} bytes instead of 8", n );
	}
}

void EventLoop::doPendingFunctors() {
	std::vector< Functor > functors;
	callingPendingFunctors_ = true;

	{
		std::lock_guard< std::mutex > lock( mutex_ );
		functors.swap( pendingFunctors_ );
	}

	for( size_t i = 0; i < functors.size(); ++i ) {
		functors[ i ]();
	}
	callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const {
	for( ChannelList::const_iterator it = activeChannels_.begin();
	     it != activeChannels_.end(); ++it ) {
		const Channel *ch = *it;
		HIPPO_DEBUG( "{}{}{} ", "{", ch->reventsToString(), "} " );
	}
}

}
