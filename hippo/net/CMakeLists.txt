set( net_SRCS
    SocketsOps.cc
    InetAddress.cc
    Socket.cc
    Timer.cc
    TimerQueue.cc
    Poller.cc
    EventLoop.cc
    Channel.cc
    Acceptor.cc
    Connector.cc
    Buffer.cc
    EventLoopThread.cc
    EventLoopThreadPool.cc
    TcpConnection.cc
    TcpClient.cc
    TcpServer.cc

    poller/DefaultPoller.cc
    poller/PollPoller.cc
   # poller/EPollPoller.cc
)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set( net_SRCS ${net_SRCS} poller/EPollPoller.cc )
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

add_library( hipponet ${net_SRCS} )
target_link_libraries( hipponet hippobase hippoutil )
