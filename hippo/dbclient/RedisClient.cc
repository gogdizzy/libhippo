
#include <unistd.h>
#include <iostream>

#include "hippo/dbclient/RedisClient.h"
#include "hippo/util/PropReader.h"
#include "hippo/util/Logging.h"


namespace hippo {

bool RedisConf::load( const std::string& filePath ) {
	PropReader pr;
	if( !pr.readFile( filePath ) ) return false;

	try {
		host_ = pr.getProp( "host" );
		port_ = std::stoi( pr.getProp( "port" ) );
		auth_ = pr.getProp( "auth" );
		database_ = std::stoi( pr.getProp( "database" ) );
		pool_capacity_ = std::stoi( pr.getProp( "pool_capacity" ) );
	}
	catch( std::exception& e ) {
		std::cerr << "Error: [RedisConf::load] " << e.what() << std::endl;
		return false;
	}
	
	return true;
}

RedisClient::RedisClient()
	: ctx_( nullptr ) {
}

bool RedisClient::connect( const std::string& addr, int port ) {
	ctx_ = redisConnect( addr.c_str(), port );
	if( ctx_ != nullptr && ctx_->err ) {
		HIPPO_ERROR( "Connect to {}:{} fail, errmsg: {}", addr.c_str(), port, ctx_->errstr );
		redisFree( ctx_ );
		ctx_ = nullptr;
		return false;
	}
	return ctx_ != nullptr;
}

void RedisClient::close() {
	redisFree( ctx_ );
	ctx_ = nullptr;
}


RedisClientPool::RedisClientPool( const std::string& filePath )
	: curconn_( 0 ) {
	conf_.load( filePath );
}

RedisClientPool::~RedisClientPool() {
	for( auto c : connections_ ) {
		if( c ) c->close();
	}
}

RedisClientPool::ptr_t RedisClientPool::create( const std::string& filePath ) {
	return ptr_t( new RedisClientPool( filePath ) );
}

RedisClient::ptr_t RedisClientPool::get() {
	RedisClient::ptr_t rv;
	{
		std::lock_guard< std::mutex > lock( mutex_ );
		auto it = connections_.begin();
		if( it != connections_.end() ) {
			rv = *it;
			connections_.erase(it);
		}
	}

	if( !rv ) {
		rv.reset( new RedisClient() );
		while( !rv->connect( conf_.host_, conf_.port_ ) ) {
			// FIXME
			usleep( 1000000 );
		}
		rv->send( "auth %s", conf_.auth_.c_str() );
		rv->send( "select %d", conf_.database_ );
	}
	return rv;
}

void RedisClientPool::recycle( RedisClient::ptr_t c ) {
	std::lock_guard< std::mutex > lock( mutex_ );
	connections_.insert( c );
}

}
