#pragma once

#include "muduo/base/noncopyable.h"
#include "muduo/base/Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

namespace muduo {
namespace net {

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::move_only_function<void(EventLoop*)>;

    EventLoopThread(ThreadInitCallback cb = ThreadInitCallback(), 
        const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();
private:
    void threadFunc();

    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};

}  // namespace net
}  // namespace muduo