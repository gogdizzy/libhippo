#include <stdlib.h>
#include <hippo/net/Poller.h>
#include <hippo/net/poller/PollPoller.h>
#include <hippo/net/poller/EPollPoller.h>

namespace hippo {

Poller *Poller::newDefaultPoller( EventLoop *loop ) {
#ifdef __linux__
	return new EPollPoller( loop );
#else
	return new PollPoller( loop );
#endif
}

}
