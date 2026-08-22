#pragma once
#include "muduo/base/noncopyable.h"
#include "muduo/net/Socket.h"
#include "muduo/net/Channel.h"

#include <functional>

namespace muduo {
namespace net {

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::move_only_function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(NewConnectionCallback cb) 
    {
        newConnectionCallback_ = std::move(cb);
    }

    bool listenning() const { return listenning_; }
    void listen();
private:
    void handleRead();
    
    EventLoop *loop_; // Acceptor用的就是用户定义的那个baseLoop，也称作mainLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};

}  // namespace net
}  // namespace muduo