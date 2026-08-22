# mymuduo

基于 muduo 网络库的自研高性能 C++ 网络库，API 与 muduo 完全兼容，目标是对 [chatserver](..\chatserver) 实现零改动替换。

## 项目结构

```
mymuduo/
├── muduo/
│   ├── base/         # 基础层头文件（线程、时间、同步原语等）
│   ├── logging/      # 日志模块头文件（AsyncLogging、LogFile 等）
│   ├── net/          # 网络层头文件（EventLoop、TcpServer 等）
│   └── timer/        # 定时器模块头文件（Timer、TimerQueue 等）
├── src/
│   ├── base/         # 基础层实现（7 个源文件）
│   ├── logging/      # 日志模块实现（6 个源文件）
│   ├── net/          # 网络层实现（13 个源文件）
│   └── timer/        # 定时器模块实现（2 个源文件）
├── test/
│   └── api_test.cpp  # API 功能测试（对照 chatserver 所用接口）
├── CMakeLists.txt
└── README.md
```

## 核心模块

| 模块 | 路径 | 功能 |
|------|------|------|
| **基础层** | `muduo/base/` + `src/base/` | 线程封装、时间戳、互斥锁/条件变量、倒计时门闩、时区处理 |
| **网络层** | `muduo/net/` + `src/net/` | Reactor 事件循环（EventLoop）、TCP 服务器/连接、非阻塞 I/O、多线程事件循环池 |
| **日志模块** | `muduo/logging/` + `src/logging/` | 异步日志（AsyncLogging）、日志文件滚动（LogFile）、流式输出（LogStream） |
| **定时器模块** | `muduo/timer/` + `src/timer/` | 基于 `timerfd` 的定时器管理（TimerQueue） |

## 技术栈

- C++23（`std::move_only_function`、`std::expected` 等）
- CMake 3.14+
- Linux epoll / GCC 13+（WSL 编译环境）
- 零外部依赖（纯 C++ 标准库 + pthread）

## 构建与测试

```bash
cd mymuduo
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# 运行 API 测试
./api_test
```

产物：`libmymuduo.a` 静态库，链接时只需 `-lmymuduo -lpthread`。

## 命名空间

对外 API 与 muduo 完全一致，所有类位于对应命名空间：

- 网络层：`muduo::net::EventLoop`、`muduo::net::TcpServer`、`muduo::net::InetAddress` 等
- 基础层：`muduo::Timestamp`、`muduo::Thread` 等

## 现代化改造

相比 [原始 muduo](https://github.com/chenshuo/muduo)（C++11），进行了以下升级：

| 改进项 | 原 muduo | mymuduo | 优势 |
|--------|----------|---------|------|
| **C++ 标准** | C++11 | C++23 | 使用最新语言特性，更简洁高效 |
| **线程局部存储** | `__thread`（GCC 扩展） | `thread_local`（C++11） | 跨编译器兼容（GCC / Clang / MSVC） |
| **回调类型** | `std::function`（可拷贝） | `std::move_only_function`（C++23） | 不可拷贝，避免无意的回调复制和悬垂引用；内存占用更小 |
| **缓冲区视图** | 手动指针 + 长度 | `Buffer::readableSpan()` 返回 `std::span`（C++20） | 零拷贝、边界安全、标准库生态兼容 |
| **错误处理** | `errno` + 日志 `LOG_SYSERR` | `Acceptor` 使用 `std::expected`（C++23） | 显式错误传播，类型安全，避免忽略错误码 |
| **日志定位** | `__FILE__` / `__LINE__` 宏 | `std::source_location`（C++20） | 类型安全、不易被宏扩展破坏、支持默认参数 |
| **定时器排序** | `std::set`（节点型容器） | `std::flat_set` 条件编译（C++23） | 连续内存布局，减少缓存缺失和指针追逐，提升定时器查找性能 |
| **对象创建** | `new` / `delete` | `std::make_unique` / `std::make_shared`（C++14） | 异常安全，一次分配（shared_ptr 控制块+对象合并） |
| **空指针** | `NULL`（整数 0） | `nullptr`（C++11） | 类型安全，避免重载决议歧义 |
| **外部依赖** | 依赖 `boost::operators` 等 | 零外部依赖（纯 C++ 标准库 + pthread） | 减少编译时间，简化部署 |
| **兼容残留** | `__type_traits` 等 C++98 兼容代码 | 全部移除 | 代码更清晰，减少无效分支 |
| **头文件组织** | `Timestamp.h` / `MsTimestamp.h` 双重定义；<br>`Thread.h` / `MsThread.h` 同名不同布局 | 统一为单一引用入口 | 消除链接器混用不同实现的隐患，避免段错误 |

### 关键缺陷修复

原 muduo 中 `MsThread.h`（基于 pthread + `CountDownLatch`）和简化版 `Thread.h`（基于 `std::thread`）定义了同名类 `muduo::Thread` 但内存布局不同。链接器可能混用两个版本的构造函数和 `start()` 方法，导致 `CountDownLatch` 成员未初始化便被访问，引发段错误。mymuduo 将所有头文件统一为单一入口，彻底消除该类问题。
