cmake_minimum_required(VERSION 3.14)
project(mymuduo)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fPIC -Wall -Wextra")

set(MYMUDUO_SRCS
    src/base/Condition.cc
    src/base/CountDownLatch.cc
    src/base/Date.cc
    src/base/MsCurrentThread.cc
    src/base/MsThread.cc
    src/base/MsTimestamp.cc
    src/base/TimeZone.cc
    src/logging/AsyncLogging.cc
    src/logging/FileUtil.cc
    src/logging/LogFile.cc
    src/logging/LogStream.cc
    src/logging/Logger.cc
    src/logging/Logging.cc
    src/net/Acceptor.cc
    src/net/Buffer.cc
    src/net/Channel.cc
    src/net/DefaultPoller.cc
    src/net/EPollPoller.cc
    src/net/EventLoop.cc
    src/net/EventLoopThread.cc
    src/net/EventLoopThreadPool.cc
    src/net/InetAddress.cc
    src/net/Poller.cc
    src/net/Socket.cc
    src/net/TcpConnection.cc
    src/net/TcpServer.cc
    src/timer/Timer.cc
    src/timer/TimerQueue.cc
)

add_library(mymuduo STATIC ${MYMUDUO_SRCS})
target_include_directories(mymuduo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mymuduo PUBLIC pthread)

# API 功能测试（对照 chatserver 使用的 API）
option(BUILD_API_TEST "Build mymuduo API test" ON)
if(BUILD_API_TEST)
    add_executable(api_test test/api_test.cpp)
    target_link_libraries(api_test mymuduo pthread)
endif()