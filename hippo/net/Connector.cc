#include <errno.h>
#include <assert.h>
#include <hippo/util/Logging.h>
#include <hippo/net/Channel.h>
#include <hippo/net/EventLoop.h>
#include <hippo/net/SocketsOps.h>

#include <hippo/net/Connector.h>

namespace hippo {

const int Connector::kMaxRetryDelayMs;

Connector::Connector( EventLoop *loop, const InetAddress &serverAddr )
		: loop_( loop ),
		  serverAddr_( serverAddr ),
		  connect_( false ),
		  state_( kDisconnected ),
		  retryDelayMs_( kInitRetryDelayMs ) {
	HIPPO_DEBUG( "ctor[{}]", (void*)this );
}

Connector::~Connector() {
	HIPPO_DEBUG( "dtor[{}]", (void*)this );
	assert( !channel_ );
}

void Connector::start() {
	connect_ = true;
	loop_->runInLoop( std::bind( &Connector::startInLoop, this ) ); // FIXME: unsafe
}

void Connector::startInLoop() {
	loop_->assertInLoopThread();
	assert( state_ == kDisconnected );
	if( connect_ ) {
		connect();
	}
	else {
		HIPPO_DEBUG( "do not connect" );
	}
}

void Connector::stop() {
	connect_ = false;
	loop_->queueInLoop( std::bind( &Connector::stopInLoop, this ) ); // FIXME: unsafe
	// FIXME: cancel timer
}

void Connector::stopInLoop() {
	loop_->assertInLoopThread();
	if( state_ == kConnecting ) {
		setState( kDisconnected );
		int sockfd = removeAndResetChannel();
		retry( sockfd );
	}
}

void Connector::connect() {
	int sockfd = sockets::createNonblockingOrDie();
	int ret = sockets::connect( sockfd, serverAddr_.getSockAddrInet() );
	int savedErrno = ( ret == 0 ) ? 0 : errno;
	switch( savedErrno ) {
		case 0:
		case EINPROGRESS:
		case EINTR:
		case EISCONN:
			connecting( sockfd );
			break;

		case EAGAIN:
		case EADDRINUSE:
		case EADDRNOTAVAIL:
		case ECONNREFUSED:
		case ENETUNREACH:
			retry( sockfd );
			break;

		case EACCES:
		case EPERM:
		case EAFNOSUPPORT:
		case EALREADY:
		case EBADF:
		case EFAULT:
		case ENOTSOCK:
			HIPPO_ERROR( "connect error in Connector::startInLoop {}", savedErrno );
			sockets::close( sockfd );
			break;

		default:
			HIPPO_ERROR( "Unexpected error in Connector::startInLoop {}", savedErrno );
			sockets::close( sockfd );
			// connectErrorCallback_();
			break;
	}
}

void Connector::restart() {
	loop_->assertInLoopThread();
	setState( kDisconnected );
	retryDelayMs_ = kInitRetryDelayMs;
	connect_ = true;
	startInLoop();
}

void Connector::connecting( int sockfd ) {
	setState( kConnecting );
	assert( !channel_ );
	channel_.reset( new Channel( loop_, sockfd ) );
	channel_->setWriteCallback(
			std::bind( &Connector::handleWrite, this ) ); // FIXME: unsafe
	channel_->setErrorCallback(
			std::bind( &Connector::handleError, this ) ); // FIXME: unsafe

	// channel_->tie(shared_from_this()); is not working,
	// as channel_ is not managed by shared_ptr
	channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
	channel_->disableAll();
	channel_->remove();
	int sockfd = channel_->fd();
	// Can't reset channel_ here, because we are inside Channel::handleEvent
	loop_->queueInLoop( std::bind( &Connector::resetChannel, this ) ); // FIXME: unsafe
	return sockfd;
}

void Connector::resetChannel() {
	channel_.reset();
}

void Connector::handleWrite() {
	HIPPO_DEBUG( "Connector::handleWrite {}", state_ );

	if( state_ == kConnecting ) {
		int sockfd = removeAndResetChannel();
		int err = sockets::getSocketError( sockfd );
		if( err ) {
			HIPPO_WARN( "Connector::handleWrite - SO_ERROR = {} {}", err, strerror( err ) );
			retry( sockfd );
		}
		else if( sockets::isSelfConnect( sockfd ) ) {
			HIPPO_WARN( "Connector::handleWrite - Self connect" );
			retry( sockfd );
		}
		else {
			setState( kConnected );
			if( connect_ ) {
				newConnectionCallback_( sockfd );
			}
			else {
				sockets::close( sockfd );
			}
		}
	}
	else {
		// what happened?
		assert( state_ == kDisconnected );
	}
}

void Connector::handleError() {
	HIPPO_ERROR( "Connector::handleError busy = {}", state_ );
	if( state_ == kConnecting ) {
		int sockfd = removeAndResetChannel();
		int err = sockets::getSocketError( sockfd );
		HIPPO_DEBUG( "SO_ERROR = {} {}", err, strerror( err ) );
		retry( sockfd );
	}
}

void Connector::retry( int sockfd ) {
	sockets::close( sockfd );
	setState( kDisconnected );
	if( connect_ ) {
		HIPPO_INFO( "Connector::retry - Retry connecting to {} in {} milliseconds.",
		            serverAddr_.toIpPort(), retryDelayMs_ );
		loop_->runAfter( retryDelayMs_ / 1000.0,
		                 std::bind( &Connector::startInLoop, shared_from_this() ) );
		retryDelayMs_ = std::min( retryDelayMs_ * 2, kMaxRetryDelayMs );
	}
	else {
		HIPPO_DEBUG( "do not connect" );
	}
}

}
