#pragma once

#include <mutex>
#include <condition_variable>
#include <hippo/base/Noncopyable.h>

namespace hippo {

class CountDownLatch : Noncopyable {
public:

	explicit CountDownLatch( int count );

	void wait();

	void countDown();

	int getCount() const;

private:
	std::mutex mutex_;
	std::condition_variable condition_;
	int count_;
};

}
