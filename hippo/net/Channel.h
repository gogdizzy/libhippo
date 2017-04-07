#pragma once

#include <functional>
#include <memory>

#include <hippo/base/Noncopyable.h>
#include <hippo/base/Timestamp.h>

namespace hippo {

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Channel : Noncopyable {
public:
	typedef std::function< void() > EventCallback;
	typedef std::function< void( Timestamp ) > ReadEventCallback;

	Channel( EventLoop *loop, int fd );

	~Channel();

	void handleEvent( Timestamp receiveTime );

	void setReadCallback( const ReadEventCallback &cb ) { readCallback_ = cb; }

	void setWriteCallback( const EventCallback &cb ) { writeCallback_ = cb; }

	void setCloseCallback( const EventCallback &cb ) { closeCallback_ = cb; }

	void setErrorCallback( const EventCallback &cb ) { errorCallback_ = cb; }

	void setReadCallback( ReadEventCallback &&cb ) { readCallback_ = std::move( cb ); }

	void setWriteCallback( EventCallback &&cb ) { writeCallback_ = std::move( cb ); }

	void setCloseCallback( EventCallback &&cb ) { closeCallback_ = std::move( cb ); }

	void setErrorCallback( EventCallback &&cb ) { errorCallback_ = std::move( cb ); }

	/// Tie this channel to the owner object managed by shared_ptr,
	/// prevent the owner object being destroyed in handleEvent.
	void tie( const std::shared_ptr< void > & );

	int fd() const { return fd_; }

	int events() const { return events_; }

	void setRevents( int revt ) { revents_ = revt; } // used by pollers
	// int revents() const { return revents_; }
	bool isNoneEvent() const { return events_ == kNoneEvent; }

	void enableReading() {
		events_ |= kReadEvent;
		update();
	}

	void disableReading() {
		events_ &= ~kReadEvent;
		update();
	}

	void enableWriting() {
		events_ |= kWriteEvent;
		update();
	}

	void disableWriting() {
		events_ &= ~kWriteEvent;
		update();
	}

	void disableAll() {
		events_ = kNoneEvent;
		update();
	}

	bool isWriting() const { return events_ & kWriteEvent; }

	// for Poller
	int index() { return index_; }

	void setIndex( int idx ) { index_ = idx; }

	// for debug
	std::string reventsToString() const;

	std::string eventsToString() const;

	void logHupEvent( bool on ) { logHup_ = on; }

	EventLoop *ownerLoop() { return loop_; }

	void remove();

private:
	static std::string eventsToString( int fd, int ev );

	void update();

	void handleEventWithGuard( Timestamp receiveTime );

	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;

	EventLoop *loop_;
	const int fd_;
	int events_;
	int revents_; // it's the received event types of epoll or poll
	int index_; // used by Poller.
	bool logHup_;

	std::weak_ptr< void > tie_;
	bool tied_;
	bool eventHandling_;
	bool addedToLoop_;
	ReadEventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;
};

}
