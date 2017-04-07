
#pragma once

#include <time.h>
#include <functional>
#include <mutex>
#include <random>

#include <hippo/util/RandUtil.h>

namespace hippo {

// if need speed use std::minstd_rand or std::minstd_rand0
// instead of static std::mt19937

namespace RandUtil {

extern std::random_device g_rand;

template< int minv, int maxv >
int nextInt() {
	static std::mutex mutex;
	static std::uniform_int_distribution<> dis( minv, maxv );
	static std::mt19937 engine( static_cast<int>( g_rand() )
		^ static_cast<int>( reinterpret_cast<intptr_t>( &dis ) )
		^ static_cast<int>( time( NULL ) ) );
	static auto rnd = std::bind( dis, engine );
	std::lock_guard< std::mutex > guard( mutex );
	return rnd();
}

int nextInt( int minv, int maxv );

}

}
