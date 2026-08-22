#pragma once

#include "muduo/base/Timestamp.h"

#include <functional>
#include <memory>

namespace muduo {
namespace net {

class Buffer;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
// 以下回调在多连接间共享，需要可拷贝，保留 std::function
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using MessageCallback =
    std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;
using HighWaterMarkCallback =
    std::function<void(const TcpConnectionPtr &, size_t)>;
// C++23: TimerCallback 为唯一所有权场景，使用 move_only_function 避免堆分配
using TimerCallback = std::move_only_function<void()>;

} // namespace net
} // namespace muduo