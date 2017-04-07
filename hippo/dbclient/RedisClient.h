
#pragma once

#include <string>
#include <set>
#include <mutex>
#include <memory>
#include "hiredis/hiredis.h"

namespace hippo {

struct RedisConf {

	std::string host_;
	int port_;
	std::string auth_;
	int database_;
	int pool_capacity_;

	bool load( const std::string& filePath );
};

typedef std::shared_ptr< redisReply > RedisReply;

class RedisClient {

public:
	typedef std::shared_ptr< RedisClient >  ptr_t;
	RedisClient();

	bool connect( const std::string& addr, int port );
	void close();

	template< typename... Args >
	RedisReply send( const char* fmt, Args... args );

private:
	redisContext* ctx_;
};

template< typename... Args >
RedisReply RedisClient::send( const char* fmt, Args... args ) {
	redisReply *rv;
	rv = (redisReply*) redisCommand( ctx_, fmt, args... );
	return RedisReply( rv, []( redisReply* r ) { freeReplyObject( r ); } );
}

class RedisClientPool {

public:
	typedef std::shared_ptr< RedisClientPool > ptr_t;
	static ptr_t create( const std::string& filePath );

	RedisClient::ptr_t get();
	void recycle( RedisClient::ptr_t c );

	~RedisClientPool();
private:
	RedisClientPool( const std::string& filePath );

	RedisConf conf_;
	unsigned int curconn_;
	std::set< RedisClient::ptr_t > connections_;
	std::mutex mutex_;
};


} // namespace hippo
