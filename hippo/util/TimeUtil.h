//
// Created by gogdizzy on 16/2/18.
//

#pragma once


#include <time.h>

namespace hippo {

namespace TimeUtil {

constexpr int HOUR_TO_MILLIS = 60 * 60 * 1000;
constexpr int MINUTE_TO_MILLIS = 60 * 1000;
constexpr int SECOND_TO_MILLIS = 1000;

int64_t parseHMStoMillis( const char *time ) {
	int64_t hour = ( time[0] - '0' ) * 10 + time[1] - '0';
	int64_t minute = ( time[3] - '0' ) * 10 + time[4] - '0';
	int64_t second = ( time[6] - '0' ) * 10 + time[7] - '0';
	return hour * HOUR_TO_MILLIS + minute * MINUTE_TO_MILLIS + second * SECOND_TO_MILLIS;
}

int64_t parseHMStoMillis( const std::string &time ) {
	return parseHMStoMillis( time.c_str() );
}

int64_t parseYMDHMStoMillis( const char *time ) {
	struct tm tmp;
	auto rv = strptime( time, "%Y-%m-%d %H:%M:%S", &tmp );
	if( rv == nullptr ) return -1LL;
	return mktime( &tmp ) * (int64_t)SECOND_TO_MILLIS;
}

int64_t parseYMDHMStoMillis( const std::string &time ) {
	return parseYMDHMStoMillis( time.c_str() );
}

}

}
