#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <hippo/base/Types.h>
#include <hippo/util/Logging.h>
#include <hippo/net/Channel.h>
#include <hippo/net/poller/EPollPoller.h>


namespace hippo {
// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
static_assert( EPOLLIN == POLLIN, "err EPOLLIN != POLLIN" );
static_assert( EPOLLPRI == POLLPRI, "err EPOLLPRI != POLLPRI" );
static_assert( EPOLLOUT == POLLOUT, "err EPOLLOUT != POLLOUT" );
static_assert( EPOLLRDHUP == POLLRDHUP, "err EPOLLRDHUP != POLLRDHUP" );
static_assert( EPOLLERR == POLLERR, "err EPOLLERR != POLLERR" );
static_assert( EPOLLHUP == POLLHUP, "err EPOLLHUP != POLLHUP" );

namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

EPollPoller::EPollPoller( EventLoop *loop )
		: Poller( loop ),
		  epollfd_( ::epoll_create1( EPOLL_CLOEXEC ) ),
		  events_( kInitEventListSize ) {
	if( epollfd_ < 0 ) {
		HIPPO_ERROR( "EPollPoller::EPollPoller epollfd_ < 0" );
	}
}

EPollPoller::~EPollPoller() {
	::close( epollfd_ );
}

Timestamp EPollPoller::poll( int timeoutMs, ChannelList *activeChannels ) {
	HIPPO_TRACE( "fd total count {}", channels_.size() );
	int numEvents = ::epoll_wait( epollfd_,
	                              &*events_.begin(),
	                              static_cast<int>(events_.size()),
	                              timeoutMs );
	int savedErrno = errno;
	Timestamp now( Timestamp::now() );
	if( numEvents > 0 ) {
		HIPPO_TRACE( "{} events happended", numEvents );
		fillActiveChannels( numEvents, activeChannels );
		if( implicit_cast< size_t >( numEvents ) == events_.size() ) {
			events_.resize( events_.size() * 2 );
		}
	}
	else if( numEvents == 0 ) {
		HIPPO_TRACE( "nothing happended" );
	}
	else {
		// error happens, log uncommon ones
		if( savedErrno != EINTR ) {
			errno = savedErrno;
			HIPPO_ERROR( "EPollPoller::poll() {}", savedErrno );
		}
	}
	return now;
}

void EPollPoller::fillActiveChannels( int numEvents,
                                      ChannelList *activeChannels ) const {
	assert( implicit_cast< size_t >( numEvents ) <= events_.size() );
	for( int i = 0; i < numEvents; ++i ) {
		Channel *channel = static_cast<Channel *>(events_[ i ].data.ptr);
#ifndef NDEBUG
		int fd = channel->fd();
		ChannelMap::const_iterator it = channels_.find( fd );
		assert( it != channels_.end() );
		assert( it->second == channel );
#endif
		channel->setRevents( events_[ i ].events );
		activeChannels->push_back( channel );
	}
}

void EPollPoller::updateChannel( Channel *channel ) {
	Poller::assertInLoopThread();
	const int index = channel->index();
	HIPPO_TRACE( "fd = {} events = {} index = {}", channel->fd(), channel->events(), index );
	if( index == kNew || index == kDeleted ) {
		// a new one, add with EPOLL_CTL_ADD
		int fd = channel->fd();
		if( index == kNew ) {
			assert( channels_.find( fd ) == channels_.end() );
			channels_[ fd ] = channel;
		}
		else // index == kDeleted
		{
			assert( channels_.find( fd ) != channels_.end() );
			assert( channels_[ fd ] == channel );
		}

		channel->setIndex( kAdded );
		update( EPOLL_CTL_ADD, channel );
	}
	else {
		// update existing one with EPOLL_CTL_MOD/DEL
		int fd = channel->fd();
		(void) fd;
		assert( channels_.find( fd ) != channels_.end() );
		assert( channels_[ fd ] == channel );
		assert( index == kAdded );
		if( channel->isNoneEvent() ) {
			update( EPOLL_CTL_DEL, channel );
			channel->setIndex( kDeleted );
		}
		else {
			update( EPOLL_CTL_MOD, channel );
		}
	}
}

void EPollPoller::removeChannel( Channel *channel ) {
	Poller::assertInLoopThread();
	int fd = channel->fd();
	HIPPO_TRACE( "fd = {}", fd );
	assert( channels_.find( fd ) != channels_.end() );
	assert( channels_[ fd ] == channel );
	assert( channel->isNoneEvent() );
	int index = channel->index();
	assert( index == kAdded || index == kDeleted );
	size_t n = channels_.erase( fd );
	(void) n;
	assert( n == 1 );

	if( index == kAdded ) {
		update( EPOLL_CTL_DEL, channel );
	}
	channel->setIndex( kNew );
}

void EPollPoller::update( int operation, Channel *channel ) {
	struct epoll_event event;
	memset( &event, 0, sizeof event );
	event.events = channel->events();
	event.data.ptr = channel;
	int fd = channel->fd();
	HIPPO_TRACE( "epoll_ctl op = {} fd = {} event = ({})",
	             operationToString( operation ),
	             fd,
	             channel->eventsToString() );
	if( ::epoll_ctl( epollfd_, operation, fd, &event ) < 0 ) {
		if( operation == EPOLL_CTL_DEL ) {
			HIPPO_ERROR( "epoll_ctl op = {} fd = {} err = {}", operationToString( operation ), fd, errno );
		}
		else {
			// LOG_SYSFATAL << "epoll_ctl op =" << operationToString( operation ) << " fd =" << fd;
			HIPPO_ERROR( "epoll_ctl op = {} fd = {} err = {}", operationToString( operation ), fd, errno );
		}
	}
}

const char *EPollPoller::operationToString( int op ) {
	switch( op ) {
		case EPOLL_CTL_ADD:
			return "ADD";
		case EPOLL_CTL_DEL:
			return "DEL";
		case EPOLL_CTL_MOD:
			return "MOD";
		default:
			assert( false && "ERROR op" );
			return "Unknown Operation";
	}
}

}
