
#pragma once

#include <assert.h>
#include <memory>

#include <hippo/codec/CommandRegistry.h>
#include <hippo/codec/NetMessage.h>

#include <hippo/util/Logging.h>

namespace hippo {

class ProtobufDecoder {

public:
	ProtobufDecoder( const std::shared_ptr< CommandRegistry > &registry ) : registry_( registry ) {}

	bool decode( hippo::Buffer* buf, NetMessage& netMsg ) {
		HIPPO_TRACE( "==== decode {} bytes", buf->readableBytes() );

		MessagePtr msg;
		if( buf->readableBytes() < kHeaderLen ) return false;

		int len = buf->peekInt32();
		HIPPO_TRACE( "==== decode len = {}", len );
		if( buf->readableBytes() < 4 + len ) return false;

		buf->readInt32();
		int cmd = buf->readInt32();
		int msgId = buf->readInt32();
		msg.reset( createMessage( cmd ) );
		if( !msg ) {
			HIPPO_ERROR( "can't create msg for cmd {}", cmd );
			return false;
		}
		if( !msg->ParseFromArray( buf->peek(), len - 8 ) ) {
			HIPPO_ERROR( "decode ParseFromArray fail" );
			return false;
		}
		netMsg.cmd_ = cmd;
		netMsg.msgId_ = msgId;
		netMsg.msg_ = msg;

		buf->retrieve( len - 8 );
		HIPPO_TRACE( "==== decode ok" );
		return true;
	}

protected:
	google::protobuf::Message* createMessage( int cmd ) {

		const CommandEntry* entry = registry_->get( cmd );
		if( entry ) return entry->msg_.New();
		return nullptr;

	}

private:
	std::shared_ptr< CommandRegistry > registry_;

	const static int kHeaderLen = sizeof( int32_t ) * 3;
	const static int kMinMessageLen = 0;
	const static int kMaxMessageLen = 64 * 1024 * 1024;
};

}
