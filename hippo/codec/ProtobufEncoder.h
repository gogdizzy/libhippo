#pragma once

#include <hippo/codec/NetMessage.h>
#include <hippo/net/Buffer.h>
#include <hippo/util/Logging.h>

namespace hippo {

class ProtobufEncoder {

public:
	bool encode( const NetMessage &netMsg, hippo::Buffer *buf ) {
		assert( buf->readableBytes() == 0 );
		buf->appendInt32( netMsg.cmd_ );
		buf->appendInt32( netMsg.msgId_ );

		int byte_size = netMsg.msg_->ByteSize();
		buf->ensureWritableBytes( byte_size );

		uint8_t *start = reinterpret_cast<uint8_t *>(buf->beginWrite());
		uint8_t *end = netMsg.msg_->SerializeWithCachedSizesToArray( start );
		if( end - start != byte_size ) {
			HIPPO_ERROR( "ProtobufEncoder::encode serialize err" );
			return false;
		}
		buf->hasWritten( byte_size );

		int32_t len = buf->readableBytes();
		buf->prependInt32( len );

		HIPPO_TRACE( "==== encode {} bytes\n", buf->readableBytes() );
		return true;
	}

private:


};


}
