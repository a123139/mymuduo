/**
 * mymuduo API 功能测试
 * 对照 chatserver .trae/documents/mymuduo-replacement-plan.md 中的 API 清单
 * 使用 muduo::net:: / muduo:: 命名空间，与 muduo 官方 API 完全一致
 */
#include "muduo/base/Timestamp.h"
#include "muduo/logging/Logging.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/TcpConnection.h"
#include "muduo/net/TcpServer.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// 使用 muduo 命名空间别名模拟 chatserver 的使用方式
using muduo::Timestamp;
using namespace muduo::net;

static int g_pass = 0;
static int g_fail = 0;

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS()                                                                 \
  do {                                                                         \
    std::cout << "PASS" << std::endl;                                          \
    ++g_pass;                                                                  \
  } while (0)
#define FAIL(msg)                                                              \
  do {                                                                         \
    std::cout << "FAIL (" << msg << ")" << std::endl;                          \
    ++g_fail;                                                                  \
  } while (0)

// ============================================================
// 1. EventLoop — chatserver 使用: 默认构造 + loop()
// ============================================================
void test_eventloop_basic() {
  TEST("EventLoop 默认构造");
  {
    EventLoop loop;
    PASS();
  }

  TEST("EventLoop loop() 非阻塞 + quit() 退出");
  {
    EventLoop loop;
    std::thread t([&loop]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      loop.quit();
    });
    loop.loop();
    t.join();
    PASS();
  }
}

// ============================================================
// 2. InetAddress — chatserver 使用: InetAddress(ip, port) + toIpPort()
// ============================================================
void test_inetaddress() {
  TEST("InetAddress(ip, port) 构造 + toIpPort()");
  {
    InetAddress addr("127.0.0.1", 6000);
    std::string ipport = addr.toIpPort();
    if (!ipport.empty()) {
      std::cout << "(" << ipport << ") ";
      PASS();
    } else {
      FAIL("toIpPort 返回空");
    }
  }

  TEST("InetAddress toIp() + toPort()");
  {
    InetAddress addr("192.168.1.1", 8080);
    std::string ip = addr.toIp();
    uint16_t port = addr.toPort();
    if (!ip.empty() && port == 8080) {
      std::cout << "(" << ip << ":" << port << ") ";
      PASS();
    } else {
      FAIL("toIp/toPort 异常");
    }
  }
}

// ============================================================
// 3. TcpServer — chatserver 使用: 构造 + setConnectionCallback +
//    setMessageCallback + setThreadNum + start()
// ============================================================
void test_tcpserver_basic() {
  TEST("TcpServer 构造");
  {
    EventLoop loop;
    InetAddress addr("127.0.0.1", 9999);
    TcpServer server(&loop, addr, "TestServer");
    PASS();
  }

  TEST("setConnectionCallback + setMessageCallback + setThreadNum + start");
  {
    EventLoop loop;
    InetAddress addr("127.0.0.1", 9998);
    TcpServer server(&loop, addr, "TestServer");

    server.setConnectionCallback(
        [](const TcpConnectionPtr &conn) { (void)conn; });
    server.setMessageCallback(
        [](const TcpConnectionPtr &conn, Buffer *buf, Timestamp time) {
          (void)conn;
          (void)buf;
          (void)time;
        });
    server.setThreadNum(4);
    server.start();
    PASS();
  }
}

// ============================================================
// 4. TcpConnectionPtr + Callbacks 类型别名
// ============================================================
void test_callbacks_types() {
  TEST("TcpConnectionPtr 类型存在");
  {
    TcpConnectionPtr ptr;
    (void)ptr;
    PASS();
  }

  TEST("ConnectionCallback 可赋值 lambda");
  {
    ConnectionCallback cb = [](const TcpConnectionPtr &) {};
    (void)cb;
    PASS();
  }

  TEST("MessageCallback 可赋值 lambda");
  {
    MessageCallback cb = [](const TcpConnectionPtr &, Buffer *, Timestamp) {};
    (void)cb;
    PASS();
  }
}

// ============================================================
// 5. Buffer — chatserver 使用: retrieveAllAsString() + append()
//    C++20: readableSpan()
// ============================================================
void test_buffer() {
  TEST("Buffer 默认构造 + retrieveAllAsString() 空缓冲区");
  {
    Buffer buf;
    std::string s = buf.retrieveAllAsString();
    if (s.empty()) {
      PASS();
    } else {
      FAIL("空 Buffer 应返回空: got '" + s + "'");
    }
  }

  TEST("Buffer append + retrieveAllAsString");
  {
    Buffer buf;
    const char *data = "hello muduo";
    buf.append(data, 11);
    std::string s = buf.retrieveAllAsString();
    if (s == "hello muduo") {
      PASS();
    } else {
      FAIL("数据不匹配: '" + s + "'");
    }
  }

  TEST("Buffer readableSpan() — C++20 span");
  {
    Buffer buf;
    buf.append("ABCDEFGH", 8);
    std::span<const char> span = buf.readableSpan();
    if (span.size() == 8 && span[0] == 'A') {
      PASS();
    } else {
      FAIL("span 数据异常");
    }
  }
}

// ============================================================
// 6. Timestamp — chatserver 使用: 作为 MsgHandler 参数类型
// ============================================================
void test_timestamp() {
  TEST("Timestamp 默认构造 + toString()");
  {
    Timestamp ts;
    std::string s = ts.toString();
    if (!s.empty()) {
      PASS();
    } else {
      FAIL("toString 返回空");
    }
  }

  TEST("Timestamp::now()");
  {
    Timestamp ts = Timestamp::now();
    std::string s = ts.toString();
    if (!s.empty()) {
      std::cout << "(" << s << ") ";
      PASS();
    } else {
      FAIL("now() toString 返回空");
    }
  }
}

// ============================================================
// 7. Logging — chatserver 使用: LOG_INFO << ...  / LOG_ERROR << ...
//    (muduo/base/Logging.h stream 风格)
// ============================================================
void test_logging() {
  TEST("LOG_INFO stream 风格");
  {
    LOG_INFO << "API test: LOG_INFO works";
    PASS();
  }

  TEST("LOG_ERROR stream 风格");
  {
    LOG_ERROR << "API test: LOG_ERROR works";
    PASS();
  }
}

// ============================================================
// main
// ============================================================
int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "  mymuduo API 功能测试" << std::endl;
  std::cout << "  对照 chatserver 所用 muduo API 清单" << std::endl;
  std::cout << "========================================" << std::endl
            << std::endl;

  std::cout << "--- 1. EventLoop ---" << std::endl;
  test_eventloop_basic();

  std::cout << std::endl << "--- 2. InetAddress ---" << std::endl;
  test_inetaddress();

  std::cout << std::endl << "--- 3. TcpServer ---" << std::endl;
  test_tcpserver_basic();

  std::cout << std::endl << "--- 4. Callbacks 类型 ---" << std::endl;
  test_callbacks_types();

  std::cout << std::endl << "--- 5. Buffer ---" << std::endl;
  test_buffer();

  std::cout << std::endl << "--- 6. Timestamp ---" << std::endl;
  test_timestamp();

  std::cout << std::endl << "--- 7. Logging ---" << std::endl;
  test_logging();

  std::cout << std::endl
            << "========================================" << std::endl;
  std::cout << "  结果: " << g_pass << " PASS, " << g_fail << " FAIL";
  if (g_fail > 0)
    std::cout << "  <= 存在失败!";
  std::cout << std::endl;
  std::cout << "========================================" << std::endl;

  return (g_fail > 0) ? 1 : 0;
}
