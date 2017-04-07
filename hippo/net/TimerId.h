#pragma once

namespace hippo {

class Timer;

///
/// An opaque identifier, for canceling Timer.
///
class TimerId {
public:
	TimerId()
			: timer_( nullptr ),
			  sequence_( 0 ) {
	}

	TimerId( Timer *timer, int64_t seq )
			: timer_( timer ),
			  sequence_( seq ) {
	}

	void clear() {
		timer_ = nullptr;
		sequence_ = 0;
	}

	// default copy-ctor, dtor and assignment are okay

	friend class TimerQueue;

private:
	Timer *timer_;
	int64_t sequence_;
};

}
