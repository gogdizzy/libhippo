//
// Created by gogdizzy on 16/1/24.
//

#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <hippo/concurrent/Task.h>
#include <hippo/concurrent/BlockingQueue.h>

namespace hippo {

struct Worker {

	Worker() {}
	Worker( const std::string & name ) : name_( name ) {}
	Worker( const std::string & name, std::thread &&thread ) :
			name_( name ),
	        thread_( std::move( thread ) ) {
	}
	Worker( Worker&& w )
			: name_( w.name_ ),
			  thread_( std::move( w.thread_ ) ) {
	}

	std::string name_;
	std::thread thread_;
};

template < typename T >
class ThreadPool : public std::enable_shared_from_this< ThreadPool< T > > {

public:
	typedef std::shared_ptr< ThreadPool< T > > ptr_t;

	ThreadPool( const std::string &name, int nthreads, BlockingQueue< T > *queue )
			: name_( name ),
			  maxThreads_( nthreads ),
			  queue_( queue ),
	          running_( false ) {
		workers_.reserve( nthreads );
	}

	~ThreadPool() {
		stop();
	}

	void start() {
		running_ = true;
		for( int i = 0; i < maxThreads_; ++i ) {
			char buf[32];
			snprintf( buf, 32, "%d", i );
			workers_.push_back( Worker( this->name() + buf, std::thread( std::bind( &ThreadPool::run, this ) ) ) );
		}
	}

	void stop() {
		running_ = false;
	}

	const std::string& name() {
		return name_;
	}



	void addTask( const T& task ) {
		queue_->put( task );
	}

private:
	void run() {
		while( running_ ) {
			try {
				T task = queue_->take();
				task();
			}
			catch( ... ) {
				// TODO log error
			}
		}
	}

	std::string name_;
	int maxThreads_;
	std::unique_ptr< BlockingQueue< T > > queue_;
	std::vector< Worker > workers_;
	bool running_;
};

}
