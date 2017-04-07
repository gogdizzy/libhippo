#pragma once

#include <stdint.h>
#include <arpa/inet.h>

#ifdef __linux__
#include <endian.h>
#endif


namespace hippo {

namespace sockets {

inline uint64_t hostToNetwork64( uint64_t host64 ) {
#ifdef __APPLE__
	return htonll( host64 );
#else
	return htobe64( host64 );
#endif
}

inline uint32_t hostToNetwork32( uint32_t host32 ) {
	return htonl( host32 );
}

inline uint16_t hostToNetwork16( uint16_t host16 ) {
	return htons( host16 );
}

inline uint64_t networkToHost64( uint64_t net64 ) {
#ifdef __APPLE__
	return ntohll( net64 );
#else
	return be64toh( net64 );
#endif
}

inline uint32_t networkToHost32( uint32_t net32 ) {
	return ntohl( net32 );
}

inline uint16_t networkToHost16( uint16_t net16 ) {
	return ntohs( net16 );
}

}

}

