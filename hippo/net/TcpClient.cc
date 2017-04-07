#include <stdio.h>  // snprintf

#include <functional>

#include <hippo/net/Connector.h>
#include <hippo/net/EventLoop.h>
#include <hippo/net/SocketsOps.h>
#include <hippo/net/TcpClient.h>
#include <hippo/util/Logging.h>



// TcpClient::TcpClient(EventLoop* loop)
//   : loop_(loop)
// {
// }

// TcpClient::TcpClient(EventLoop* loop, const string& host, uint16_t port)
//   : loop_(CHECK_NOTNULL(loop)),
//     serverAddr_(host, port)
// {
// }

namespace hippo {

namespace detail {

void removeConnection( EventLoop *loop, const TcpConnectionPtr &conn ) {
	loop->queueInLoop( std::bind( &TcpConnection::connectDestroyed, conn ) );
}

void removeConnector( const ConnectorPtr &connector ) {
	//connector->
}

}

TcpClient::TcpClient( EventLoop *loop,
                      const InetAddress &serverAddr,
                      const std::string &nameArg )
		: loop_( loop ),
		  connector_( new Connector( loop, serverAddr ) ),
		  name_( nameArg ),
		  connectionCallback_( defaultConnectionCallback ),
		  messageCallback_( defaultMessageCallback ),
		  retry_( false ),
		  connect_( true ),
		  nextConnId_( 1 ) {
	connector_->setNewConnectionCallback(
			std::bind( &TcpClient::newConnection, this, std::placeholders::_1 ) );
	// FIXME setConnectFailedCallback
	HIPPO_INFO( "TcpClient::TcpClient[{}] - connector {}", name_, (void*)connector_.get() );
}

TcpClient::~TcpClient() {
	HIPPO_INFO( "TcpClient::~TcpClient[{}] - connector {}", name_, (void*)connector_.get() );
	TcpConnectionPtr conn;
	bool unique = false;
	{
		std::lock_guard< std::mutex > lock( mutex_ );
		unique = connection_.unique();
		conn = connection_;
	}
	if( conn ) {
		assert( loop_ == conn->getLoop() );
		// FIXME: not 100% safe, if we are in different thread
		CloseCallback cb = std::bind( &detail::removeConnection, loop_, std::placeholders::_1 );
		loop_->runInLoop(
				std::bind( &TcpConnection::setCloseCallback, conn, cb ) );
		if( unique ) {
			conn->forceClose();
		}
	}
	else {
		connector_->stop();
		// FIXME: HACK
		loop_->runAfter( 1, std::bind( &detail::removeConnector, connector_ ) );
	}
}

void TcpClient::connect() {
	// FIXME: check busy
	HIPPO_INFO( "TcpClient::connect[{}] - connecting to {}", name_, connector_->serverAddress().toIpPort() );
	connect_ = true;
	connector_->start();
}

void TcpClient::disconnect() {
	connect_ = false;

	{
		std::lock_guard< std::mutex > lock( mutex_ );
		if( connection_ ) {
			connection_->shutdown();
		}
	}
}

void TcpClient::stop() {
	connect_ = false;
	connector_->stop();
}

void TcpClient::newConnection( int sockfd ) {
	loop_->assertInLoopThread();
	InetAddress peerAddr( sockets::getPeerAddr( sockfd ) );
	char buf[32];
	snprintf( buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_ );
	++nextConnId_;
	string connName = name_ + buf;

	InetAddress localAddr( sockets::getLocalAddr( sockfd ) );
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	TcpConnectionPtr conn( new TcpConnection( loop_,
	                                          connName,
	                                          sockfd,
	                                          localAddr,
	                                          peerAddr ) );

	conn->setConnectionCallback( connectionCallback_ );
	conn->setMessageCallback( messageCallback_ );
	conn->setWriteCompleteCallback( writeCompleteCallback_ );
	conn->setCloseCallback(
			std::bind( &TcpClient::removeConnection, this, std::placeholders::_1 ) ); // FIXME: unsafe
	{
		std::lock_guard< std::mutex > lock( mutex_ );
		connection_ = conn;
	}
	conn->connectEstablished();
}

void TcpClient::removeConnection( const TcpConnectionPtr &conn ) {
	loop_->assertInLoopThread();
	assert( loop_ == conn->getLoop() );

	{
		std::lock_guard< std::mutex > lock( mutex_ );
		assert( connection_ == conn );
		connection_.reset();
	}

	loop_->queueInLoop( std::bind( &TcpConnection::connectDestroyed, conn ) );
	if( retry_ && connect_ ) {
		HIPPO_INFO( "TcpClient::connect[{}] - Reconnecting to {}", name_, connector_->serverAddress().toIpPort() );
		connector_->restart();
	}
}

}
