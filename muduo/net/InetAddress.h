#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdint>
#include <string>

namespace muduo {
namespace net {

// 封装socket地址类型
class InetAddress {
public:
  // chatserver 兼容: 参数顺序 (string ip, uint16_t port)
  explicit InetAddress(std::string ip = "127.0.0.1", uint16_t port = 0);
  explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {}

  std::string toIp() const;
  std::string toIpPort() const;
  uint16_t toPort() const;

  const sockaddr_in *getSockAddr() const { return &addr_; }
  void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

private:
  sockaddr_in addr_;
};

}  // namespace net
}  // namespace muduo