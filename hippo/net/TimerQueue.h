#pragma once

#include <set>
#include <vector>
#include <mutex>

#include <hippo/base/Noncopyable.h>
#include <hippo/base/Timestamp.h>
#include <hippo/net/Callbacks.h>
#include <hippo/net/Channel.h>
#include <hippo/net/TimerId.h>

namespace hippo {

class EventLoop;

class Timer;

class TimerId;

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
class TimerQueue : Noncopyable {
public:
	TimerQueue( EventLoop *loop );

	~TimerQueue();

	///
	/// Schedules the callback to be run at given time,
	/// repeats if @c interval > 0.0.
	///
	/// Must be thread safe. Usually be called from other threads.
	TimerId addTimer( const TimerCallback &cb,
	                  Timestamp when,
	                  double interval );

	TimerId addTimer( TimerCallback &&cb,
	                  Timestamp when,
	                  double interval );

	void cancel( TimerId timerId );

private:

	// FIXME: use unique_ptr<Timer> instead of raw pointers.
	typedef std::pair< Timestamp, Timer * > Entry;
	typedef std::set< Entry > TimerList;
	typedef std::pair< Timer *, int64_t > ActiveTimer;
	typedef std::set< ActiveTimer > ActiveTimerSet;

	void addTimerInLoop( Timer *timer );

	void cancelInLoop( TimerId timerId );

	// called when timerfd alarms
	void handleRead();

	// move out all expired timers
	std::vector< Entry > getExpired( Timestamp now );

	void reset( const std::vector< Entry > &expired, Timestamp now );

	bool insert( Timer *timer );

	EventLoop *loop_;
	const int timerfd_;
	Channel timerfdChannel_;
	// Timer list sorted by expiration
	TimerList timers_;

	// for cancel()
	ActiveTimerSet activeTimers_;
	bool callingExpiredTimers_;
	/* atomic */
	ActiveTimerSet cancelingTimers_;
};

}
