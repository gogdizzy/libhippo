#pragma once

#include <vector>

#include <hippo/net/Poller.h>


struct epoll_event;

namespace hippo {

///
/// IO Multiplexing with epoll(4).
///
class EPollPoller : public Poller {
public:
	EPollPoller( EventLoop *loop );

	virtual ~EPollPoller();

	virtual Timestamp poll( int timeoutMs, ChannelList *activeChannels );

	virtual void updateChannel( Channel *channel );

	virtual void removeChannel( Channel *channel );

private:
	static const int kInitEventListSize = 16;

	static const char *operationToString( int op );

	void fillActiveChannels( int numEvents,
	                         ChannelList *activeChannels ) const;

	void update( int operation, Channel *channel );

	typedef std::vector< struct epoll_event > EventList;

	int epollfd_;
	EventList events_;
};

}
