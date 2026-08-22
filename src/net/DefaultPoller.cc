#include "muduo/net/Poller.h"
#include "muduo/net/EPollPoller.h"

#include <stdlib.h>

namespace muduo {
namespace net {

Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        return nullptr; // 生成poll的实例
    }
    else
    {
        return new EPollPoller(loop); // 生成epoll的实例
    }
}

}  // namespace net
}  // namespace muduo