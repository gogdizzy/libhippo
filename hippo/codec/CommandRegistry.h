//
// Created by gogdizzy on 16/1/23.
//

#pragma once

#include <map>
#include <memory>

#include <hippo/base/Noncopyable.h>
#include <hippo/codec/NetMessage.h>
#include <hippo/net/TcpConnection.h>


namespace hippo {

class Callback : hippo::Noncopyable {
public:
	typedef std::shared_ptr< Callback > ptr_t;

	virtual ~Callback() { }

	virtual void onMessage( int msgId,
	                        const MessagePtr &message,
	                        const TcpConnectionPtr &conn ) const = 0;
};

struct CommandEntry {

	int cmd_;
	const google::protobuf::Message &msg_;
	Callback::ptr_t callback_;
	std::string pool_;

	CommandEntry( int cmd, const google::protobuf::Message &msg, Callback::ptr_t callback,
	              const std::string &pool = "" )
			: cmd_( cmd ),
			  msg_( msg ),
			  callback_( callback ),
			  pool_( pool ) {
	}

	CommandEntry( const CommandEntry &o )
			: cmd_( o.cmd_ ),
			  msg_( o.msg_ ),
			  callback_( o.callback_ ),
			  pool_( o.pool_ ) {
	}
};

struct CommandRegistry {

	std::map< int, CommandEntry > cmds_;

	void add( CommandEntry ce ) {
		cmds_.insert( std::make_pair( ce.cmd_, ce ) );
	}

	const CommandEntry *get( int cmd ) const {
		auto it = cmds_.find( cmd );
		return it == cmds_.end() ? nullptr : &( it->second );
	}

};


}
