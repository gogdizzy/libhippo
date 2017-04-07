
#include <hippo/util/RandUtil.h>

namespace hippo {

namespace RandUtil {

#ifdef __linux__
std::random_device g_rand( "/dev/random" );
#else
std::random_device g_rand;
#endif

int nextInt( int minv, int maxv ) {
	static std::mutex mutex;
	static std::uniform_real_distribution<> dis( 0.0, 1.0 );
	static std::mt19937 engine( static_cast<int>( g_rand() )
		^ static_cast<int>( reinterpret_cast<intptr_t>( &dis ) )
		^ static_cast<int>( time( NULL ) ) );
	static auto rnd = std::bind( dis, engine );
	std::lock_guard< std::mutex > guard( mutex );
	return ( maxv - minv + 1 ) * rnd() + minv;
}

}

}

