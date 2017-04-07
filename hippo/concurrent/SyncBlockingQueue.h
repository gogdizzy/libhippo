//
// Created by gogdizzy on 16/1/24.
//

#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>

#include <hippo/concurrent/BlockingQueue.h>

namespace hippo {

template< typename T >
class SyncBlockingQueue : public BlockingQueue< T > {

public:
	typedef T type;

	virtual bool put( const T &element ) {
		{
			std::lock_guard< std::mutex > lock( mutex_ );
			elements_.push_back( element );
		}
		cond_.notify_one();
		return true;
	}

	virtual T take() {
		std::unique_lock< std::mutex > lock( mutex_ );
		T rv;
		cond_.wait( lock, [&]() { return ( !elements_.empty() ) && extract( rv ); } );
		return rv;
	}

	virtual bool poll( T& rv, int64_t timeout_nanos ) {
		std::unique_lock< std::mutex > lock( mutex_ );
		return cond_.wait_for( lock,
			std::chrono::nanoseconds( timeout_nanos ),
			[&]() { return ( !elements_.empty() ) && extract( rv ); } );
	}

	virtual size_t size() const {
		std::lock_guard< std::mutex > lock( mutex_ );
		return elements_.size();
	}

protected:

	bool extract( T& rv ) {
		for( auto it = elements_.begin(); it != elements_.end(); ++it ) {
			if( !it->isLocked() ) {
				rv = *it;
				elements_.erase( it );
				rv.lock();
				return true;
			}
		}
		return false;
	}

private:
	mutable std::mutex mutex_;
	std::condition_variable cond_;
	std::deque< T > elements_;
};

}
