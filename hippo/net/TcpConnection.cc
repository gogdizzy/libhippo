#include <errno.h>

#include <functional>

#include <hippo/base/WeakCallback.h>
#include <hippo/net/Channel.h>
#include <hippo/net/EventLoop.h>
#include <hippo/net/Socket.h>
#include <hippo/net/SocketsOps.h>
#include <hippo/net/TcpConnection.h>
#include <hippo/util/Logging.h>

namespace hippo {

void defaultConnectionCallback( const TcpConnectionPtr &conn ) {
	HIPPO_TRACE( "{} -> {} is {}",
	             conn->localAddress().toIpPort().c_str(),
	             conn->peerAddress().toIpPort().c_str(),
	             ( conn->connected() ? "UP" : "DOWN" ) );
	// do not call conn->forceClose(), because some users want to register message callback only.
}

void defaultMessageCallback( const TcpConnectionPtr &,
                             Buffer *buf,
                             Timestamp ) {
	buf->retrieveAll();
}

TcpConnection::TcpConnection( EventLoop *loop,
                              const std::string &nameArg,
                              int sockfd,
                              const InetAddress &localAddr,
                              const InetAddress &peerAddr )
		: loop_( loop ),
		  name_( nameArg ),
		  state_( kConnecting ),
		  socket_( new Socket( sockfd ) ),
		  channel_( new Channel( loop, sockfd ) ),
		  localAddr_( localAddr ),
		  peerAddr_( peerAddr ),
		  highWaterMark_( 64 * 1024 * 1024 ) {
	channel_->setReadCallback(
			std::bind( &TcpConnection::handleRead, this, std::placeholders::_1 ) );
	channel_->setWriteCallback(
			std::bind( &TcpConnection::handleWrite, this ) );
	channel_->setCloseCallback(
			std::bind( &TcpConnection::handleClose, this ) );
	channel_->setErrorCallback(
			std::bind( &TcpConnection::handleError, this ) );
	HIPPO_TRACE( "TcpConnection::ctor[{}] at {} fd={}", name_.c_str(), (void*)this, sockfd );
	socket_->setKeepAlive( true );
}

TcpConnection::~TcpConnection() {
	HIPPO_TRACE( "TcpConnection::dtor[{}] at {} fd={} busy={}",
	             name_.c_str(), (void*)this, channel_->fd(), stateToString() );
	assert( state_ == kDisconnected );
}

bool TcpConnection::getTcpInfo( struct tcp_info *tcpi ) const {
	return socket_->getTcpInfo( tcpi );
}

std::string TcpConnection::getTcpInfoString() const {
	char buf[1024];
	buf[ 0 ] = '\0';
	socket_->getTcpInfoString( buf, sizeof buf );
	return buf;
}

void TcpConnection::send( const void *data, int len ) {
	send( StringPiece( static_cast<const char *>(data), len ) );
}

void TcpConnection::send( const StringPiece &message ) {
	if( state_ == kConnected ) {
		if( loop_->isInLoopThread() ) {
			sendInLoop( message );
		}
		else {
			auto fn = std::mem_fn< void(const StringPiece &) >( &TcpConnection::sendInLoop );
			loop_->runInLoop(
					std::bind( fn,
					             this,     // FIXME
					             message.as_string() ) );
			//std::forward<std::string>(message)));
		}
	}
}

// FIXME efficiency!!!
void TcpConnection::send( Buffer *buf ) {
	HIPPO_TRACE( "==== enter tcpconn send\n" );
	if( state_ == kConnected ) {
		HIPPO_TRACE( "==== enter tcpconn connected\n" );
		if( loop_->isInLoopThread() ) {
			sendInLoop( buf->peek(), buf->readableBytes() );
			buf->retrieveAll();
		}
		else {
			auto fn = std::mem_fn< void(const StringPiece &) >( &TcpConnection::sendInLoop );
			loop_->runInLoop(
					std::bind( fn,
					             this,     // FIXME
					             buf->retrieveAllAsString() ) );
			//std::forward<std::string>(message)));
		}
	}
	else {
		
		HIPPO_TRACE( "==== enter tcpconn not connected\n" );
	}
}


void TcpConnection::sendInLoop( const StringPiece &message ) {
	sendInLoop( message.data(), message.size() );
}

void TcpConnection::sendInLoop( const void *data, size_t len ) {
	loop_->assertInLoopThread();
	ssize_t nwrote = 0;
	size_t remaining = len;
	bool faultError = false;
	if( state_ == kDisconnected ) {
		HIPPO_WARN( "disconnected, give up writing" );
		return;
	}
	// if no thing in output queue, try writing directly
	if( !channel_->isWriting() && outputBuffer_.readableBytes() == 0 ) {
		nwrote = sockets::write( channel_->fd(), data, len );
		if( nwrote >= 0 ) {
			remaining = len - nwrote;
			if( remaining == 0 && writeCompleteCallback_ ) {
				loop_->queueInLoop( std::bind( writeCompleteCallback_, shared_from_this() ) );
			}
		}
		else // nwrote < 0
		{
			nwrote = 0;
			if( errno != EWOULDBLOCK ) {
				HIPPO_ERROR( "TcpConnection::sendInLoop" );
				if( errno == EPIPE || errno == ECONNRESET ) // FIXME: any others?
				{
					faultError = true;
				}
			}
		}
	}

	assert( remaining <= len );
	if( !faultError && remaining > 0 ) {
		size_t oldLen = outputBuffer_.readableBytes();
		if( oldLen + remaining >= highWaterMark_
		    && oldLen < highWaterMark_
		    && highWaterMarkCallback_ ) {
			loop_->queueInLoop( std::bind( highWaterMarkCallback_, shared_from_this(), oldLen + remaining ) );
		}
		outputBuffer_.append( static_cast<const char *>(data) + nwrote, remaining );
		if( !channel_->isWriting() ) {
			channel_->enableWriting();
		}
	}
}

void TcpConnection::shutdown() {
	// FIXME: use compare and swap
	if( state_ == kConnected ) {
		setState( kDisconnecting );
		// FIXME: shared_from_this()?
		loop_->runInLoop( std::bind( &TcpConnection::shutdownInLoop, this ) );
	}
}

void TcpConnection::shutdownInLoop() {
	loop_->assertInLoopThread();
	if( !channel_->isWriting() ) {
		// we are not writing
		socket_->shutdownWrite();
	}
}

// void TcpConnection::shutdownAndForceCloseAfter(double seconds)
// {
//   // FIXME: use compare and swap
//   if (state_ == kConnected)
//   {
//     setState(kDisconnecting);
//     loop_->runInLoop(boost::bind(&TcpConnection::shutdownAndForceCloseInLoop, this, seconds));
//   }
// }

// void TcpConnection::shutdownAndForceCloseInLoop(double seconds)
// {
//   loop_->assertInLoopThread();
//   if (!channel_->isWriting())
//   {
//     // we are not writing
//     socket_->shutdownWrite();
//   }
//   loop_->runAfter(
//       seconds,
//       makeWeakCallback(shared_from_this(),
//                        &TcpConnection::forceCloseInLoop));
// }

void TcpConnection::forceClose() {
	// FIXME: use compare and swap
	if( state_ == kConnected || state_ == kDisconnecting ) {
		setState( kDisconnecting );
		loop_->queueInLoop( std::bind( &TcpConnection::forceCloseInLoop, shared_from_this() ) );
	}
}

void TcpConnection::forceCloseWithDelay( double seconds ) {
	if( state_ == kConnected || state_ == kDisconnecting ) {
		setState( kDisconnecting );
		loop_->runAfter(
				seconds,
				makeWeakCallback( shared_from_this(),
				                  &TcpConnection::forceClose ) );  // not forceCloseInLoop to avoid race condition
	}
}

void TcpConnection::forceCloseInLoop() {
	loop_->assertInLoopThread();
	if( state_ == kConnected || state_ == kDisconnecting ) {
		// as if we received 0 byte in handleRead();
		handleClose();
	}
}

const char *TcpConnection::stateToString() const {
	switch( state_ ) {
		case kDisconnected:
			return "kDisconnected";
		case kConnecting:
			return "kConnecting";
		case kConnected:
			return "kConnected";
		case kDisconnecting:
			return "kDisconnecting";
		default:
			return "unknown busy";
	}
}

void TcpConnection::setTcpNoDelay( bool on ) {
	socket_->setTcpNoDelay( on );
}

void TcpConnection::connectEstablished() {
	loop_->assertInLoopThread();
	assert( state_ == kConnecting );
	setState( kConnected );
	channel_->tie( shared_from_this() );
	channel_->enableReading();

	connectionCallback_( shared_from_this() );
}

void TcpConnection::connectDestroyed() {
	loop_->assertInLoopThread();
	if( state_ == kConnected ) {
		setState( kDisconnected );
		channel_->disableAll();

		connectionCallback_( shared_from_this() );
	}
	channel_->remove();
}

void TcpConnection::handleRead( Timestamp receiveTime ) {
	loop_->assertInLoopThread();
	int savedErrno = 0;
	ssize_t n = inputBuffer_.readFd( channel_->fd(), &savedErrno );
	if( n > 0 ) {
		messageCallback_( shared_from_this(), &inputBuffer_, receiveTime );
	}
	else if( n == 0 ) {
		handleClose();
	}
	else {
		errno = savedErrno;
		HIPPO_ERROR( "TcpConnection::handleRead" );
		handleError();
	}
}

void TcpConnection::handleWrite() {
	loop_->assertInLoopThread();
	if( channel_->isWriting() ) {
		ssize_t n = sockets::write( channel_->fd(),
		                            outputBuffer_.peek(),
		                            outputBuffer_.readableBytes() );
		if( n > 0 ) {
			outputBuffer_.retrieve( n );
			if( outputBuffer_.readableBytes() == 0 ) {
				channel_->disableWriting();
				if( writeCompleteCallback_ ) {
					loop_->queueInLoop( std::bind( writeCompleteCallback_, shared_from_this() ) );
				}
				if( state_ == kDisconnecting ) {
					shutdownInLoop();
				}
			}
		}
		else {
			HIPPO_ERROR( "TcpConnection::handleWrite" );
			// if (state_ == kDisconnecting)
			// {
			//   shutdownInLoop();
			// }
		}
	}
	else {
		HIPPO_TRACE( "Connection fd = {} is down, no more writing", channel_->fd() );
	}
}

void TcpConnection::handleClose() {
	loop_->assertInLoopThread();
	HIPPO_TRACE( "fd = {} busy = {}", channel_->fd(), stateToString() );
	assert( state_ == kConnected || state_ == kDisconnecting );
	// we don't close fd, leave it to dtor, so we can find leaks easily.
	setState( kDisconnected );
	channel_->disableAll();

	TcpConnectionPtr guardThis( shared_from_this() );
	connectionCallback_( guardThis );
	// must be the last line
	closeCallback_( guardThis );
}

void TcpConnection::handleError() {
	int err = sockets::getSocketError( channel_->fd() );
	HIPPO_ERROR( "TcpConnection::handleError [{}] - SO_ERROR = {} {}", name_.c_str(), err, strerror( err ) );
}

}
