//
// Created by gogdizzy on 16/1/11.
//

#pragma once

#include <stdint.h>
#include <sstream>
#include <memory>

#include <google/protobuf/message.h>

namespace hippo {

typedef std::shared_ptr< google::protobuf::Message > MessagePtr;

struct NetMessage {

	int32_t cmd_;
	int32_t msgId_;
	MessagePtr msg_;

	NetMessage()
			: cmd_( 0 ),
			  msgId_( 0 ),
			  msg_( nullptr ) {
	}

	NetMessage( int32_t cmd, int32_t msgId, google::protobuf::Message* msg )
			: cmd_( cmd ),
			  msgId_( msgId ),
			  msg_( msg ) {
	}

	NetMessage( int32_t cmd, int32_t msgId, const MessagePtr& msg )
			: cmd_( cmd ),
			  msgId_( msgId ),
			  msg_( msg ) {

	}

	NetMessage( const NetMessage& other )
			: cmd_( other.cmd_ ),
			  msgId_( other.msgId_ ),
			  msg_( other.msg_ ) {
	}

	std::string toString() const {
		std::ostringstream oss;
		oss << "cmdId=" << cmd_ << ",msgId=" << msgId_ << ",msg=" << msg_->DebugString();
		return oss.str();
	}
};

}
