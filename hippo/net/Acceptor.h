#pragma once

#include <functional>

#include <hippo/base/Noncopyable.h>
#include <hippo/net/Channel.h>
#include <hippo/net/Socket.h>

namespace hippo {

class EventLoop;

class InetAddress;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor : Noncopyable {
public:
	typedef std::function< void( int sockfd, const InetAddress & ) > NewConnectionCallback;

	Acceptor( EventLoop *loop, const InetAddress &listenAddr, bool reuseport );

	~Acceptor();

	void setNewConnectionCallback( const NewConnectionCallback &cb ) { newConnectionCallback_ = cb; }

	bool listening() const { return listening_; }

	void listen();

private:
	void handleRead();

	EventLoop *loop_;
	Socket acceptSocket_;
	Channel acceptChannel_;
	NewConnectionCallback newConnectionCallback_;
	bool listening_;
	int idleFd_;
};

}

