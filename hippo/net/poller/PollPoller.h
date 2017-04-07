#pragma once

#include <vector>

#include <hippo/net/Poller.h>


struct pollfd;

namespace hippo {

///
/// IO Multiplexing with poll(2).
///
class PollPoller : public Poller {
public:

	PollPoller( EventLoop *loop );

	virtual ~PollPoller();

	virtual Timestamp poll( int timeoutMillis, ChannelList *activeChannels );

	virtual void updateChannel( Channel *channel );

	virtual void removeChannel( Channel *channel );

private:
	void fillActiveChannels( int numEvents,
	                         ChannelList *activeChannels ) const;

	typedef std::vector< struct pollfd > PollFdList;
	PollFdList pollfds_;
};

}
