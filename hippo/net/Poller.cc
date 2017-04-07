#include <hippo/net/Poller.h>
#include <hippo/net/Channel.h>

namespace hippo {

Poller::Poller( EventLoop *loop )
		: ownerLoop_( loop ) {
}

Poller::~Poller() {
}

bool Poller::hasChannel( Channel *channel ) const {
	assertInLoopThread();
	ChannelMap::const_iterator it = channels_.find( channel->fd() );
	return it != channels_.end() && it->second == channel;
}

}

