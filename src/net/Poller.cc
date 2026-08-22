#include "muduo/net/Poller.h"
#include "muduo/net/Channel.h"

namespace muduo {
namespace net {

Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop)
{
}

bool Poller::hasChannel(Channel *channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

}  // namespace net
}  // namespace muduo