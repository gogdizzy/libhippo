
#include "hippo/util/StrUtil.h"

namespace hippo {

namespace StrUtil {

std::string trim( const char* s ) {
	const char *p, *q;
	while( *s == ' ' || *s == '\t' ) ++s;
	if( *s == '\0' ) return "";
	p = q = s;
	while( *q != '\0' ) ++q;
	while( q[-1] == ' ' || q[-1] == '\t' ) --q;
	return std::string( p, q );
}

std::string trim( const std::string& s ) {
	auto beg = s.find_first_not_of( " \t" );
	if( beg == std::string::npos ) return "";
	auto end = s.find_last_not_of( " \t" );
	return s.substr( beg, end + 1 - beg );
}


}

}
