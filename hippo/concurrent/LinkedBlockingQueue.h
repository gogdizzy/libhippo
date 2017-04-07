//
// Created by gogdizzy on 16/1/24.
//

#pragma once

#include <memory>
#include <list>

#include <hippo/concurrent/BlockingQueue.h>

namespace hippo {

template< typename T >
class LinkedBlockingQueue : BlockingQueue< T > {

public:
	typedef T type;

	virtual bool put( const T &element ) {
		{
			std::lock_guard< std::mutex > lock( mutex_ );
			elements_.push_back( element );
		}
		cond_.notify_one();
	}

	virtual T take() {
		std::unique_lock< std::mutex > lock( mutex_ );
		cond_.wait( lock, [&]() { return !elements_.empty(); } );
		T rv( elements_.front() );
		elements_.pop_front();
		return rv;
	}

	virtual bool poll( T& rv, int64_t timeout_micros ) {
		std::unique_lock< std::mutex > lock( mutex_ );
		cond_.wait_for( lock, std::chrono::microseconds( timeout_micros ), [&]() { return !elements_.empty(); } );
		if( elements_.empty() ) return false;
		rv = elements_.front();
		elements_.pop_front();
		return true;
	}

	virtual size_t size() const {
		std::lock_guard< std::mutex > lock( mutex_ );
		return elements_.size();
	}

private:
	mutable std::mutex mutex_;
	std::condition_variable cond_;
	std::list< T > elements_;
};

}


