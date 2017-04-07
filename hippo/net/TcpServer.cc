#include <stdio.h>  // snprintf
#include <functional>

#include <hippo/net/TcpServer.h>

#include <hippo/util/Logging.h>
#include <hippo/net/Acceptor.h>
#include <hippo/net/EventLoop.h>
#include <hippo/net/EventLoopThreadPool.h>
#include <hippo/net/SocketsOps.h>


namespace hippo {

TcpServer::TcpServer( EventLoop *loop,
                      const InetAddress &listenAddr,
                      const std::string &nameArg,
                      Option option )
		: loop_( loop ),
		  hostport_( listenAddr.toIpPort() ),
		  name_( nameArg ),
		  acceptor_( new Acceptor( loop, listenAddr, option == kReusePort ) ),
		  threadPool_( new EventLoopThreadPool( loop, name_ ) ),
		  connectionCallback_( defaultConnectionCallback ),
		  messageCallback_( defaultMessageCallback ),
		  started_( 0 ),
		  nextConnId_( 1 ) {
	acceptor_->setNewConnectionCallback(
			std::bind( &TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2 ) );
}

TcpServer::~TcpServer() {
	loop_->assertInLoopThread();
	HIPPO_DEBUG( "TcpServer::~TcpServer [{}] destructing", name_ );

	for( ConnectionMap::iterator it( connections_.begin() );
	     it != connections_.end(); ++it ) {
		TcpConnectionPtr conn = it->second;
		it->second.reset();
		conn->getLoop()->runInLoop(
				std::bind( &TcpConnection::connectDestroyed, conn ) );
		conn.reset();
	}
}

void TcpServer::setThreadNum( int numThreads ) {
	assert( 0 <= numThreads );
	threadPool_->setThreadNum( numThreads );
}

void TcpServer::start() {
	if( started_.exchange( 1 ) == 0 ) {
		threadPool_->start( threadInitCallback_ );

		assert( !acceptor_->listening() );
		loop_->runInLoop(
				std::bind( &Acceptor::listen, acceptor_.get() ) );
	}
}

void TcpServer::newConnection( int sockfd, const InetAddress &peerAddr ) {
	loop_->assertInLoopThread();
	EventLoop *ioLoop = threadPool_->getNextLoop();
	char buf[32];
	snprintf( buf, sizeof buf, ":%s#%d", hostport_.c_str(), nextConnId_ );
	++nextConnId_;
	std::string connName = name_ + buf;

	HIPPO_INFO( "TcpServer::newConnection [{}] - new connection [{}] from {}",
	            name_, connName, peerAddr.toIpPort() );
	InetAddress localAddr( sockets::getLocalAddr( sockfd ) );
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	TcpConnectionPtr conn( new TcpConnection( ioLoop,
	                                          connName,
	                                          sockfd,
	                                          localAddr,
	                                          peerAddr ) );
	connections_[ connName ] = conn;
	conn->setConnectionCallback( connectionCallback_ );
	conn->setMessageCallback( messageCallback_ );
	conn->setWriteCompleteCallback( writeCompleteCallback_ );
	conn->setCloseCallback(
			std::bind( &TcpServer::removeConnection, this, std::placeholders::_1 ) ); // FIXME: unsafe
	ioLoop->runInLoop( std::bind( &TcpConnection::connectEstablished, conn ) );
}

void TcpServer::removeConnection( const TcpConnectionPtr &conn ) {
	// FIXME: unsafe
	loop_->runInLoop( std::bind( &TcpServer::removeConnectionInLoop, this, conn ) );
}

void TcpServer::removeConnectionInLoop( const TcpConnectionPtr &conn ) {
	loop_->assertInLoopThread();
	HIPPO_INFO( "TcpServer::removeConnectionInLoop [{}] - connection {}", name_, conn->name() );
	size_t n = connections_.erase( conn->name() );
	(void) n;
	assert( n == 1 );
	EventLoop *ioLoop = conn->getLoop();
	ioLoop->queueInLoop(
			std::bind( &TcpConnection::connectDestroyed, conn ) );
}

}
