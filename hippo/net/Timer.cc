
#include <hippo/net/Timer.h>

namespace hippo {

std::atomic< int64_t > Timer::s_numCreated_;

void Timer::restart( Timestamp now ) {
	if( repeat_ ) {
		expiration_ = addTime( now, interval_ );
	}
	else {
		expiration_ = Timestamp::invalid();
	}
}

}
