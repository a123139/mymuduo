# mymuduo 自研心得

基于 [muduo](https://github.com/chenshuo/muduo)（C++11）自研的高性能 C++ 网络库，目标是对 [chatserver](../chatserver) 实现零改动替换，对外 API 与 muduo 完全兼容。

---

## 一、改进点

### 1. 工程结构模块化

将原始 muduo 扁平化的目录结构重组织为四大模块，头文件与实现文件一一对应：

```
muduo/                    # 对外头文件（按模块拆分）
├── base/                 # 基础层：线程、时间戳、互斥锁/条件变量、时区
├── logging/              # 日志模块：AsyncLogging、LogFile、LogStream、Logger
├── net/                  # 网络层：EventLoop、TcpServer、Buffer、Channel
└── timer/                # 定时器模块：Timer、TimerId、TimerQueue

src/                      # 对应实现文件（按模块拆分）
├── base/   (7 个源文件)
├── logging/ (6 个源文件)
├── net/    (13 个源文件)
└── timer/  (2 个源文件)
```

对比原始 muduo 中日志和定时器头文件与基础层/网络层混杂在一起的方式，模块化后职责清晰、便于维护。

### 2. C++ 标准现代化（C++11 → C++23）

| 改进项 | 原 muduo | mymuduo | 优势 |
|--------|----------|---------|------|
| 回调类型 | `std::function`（可拷贝） | `std::move_only_function`（C++23） | 不可拷贝，避免无意复制和悬垂引用，内存占用更小 |
| 缓冲区视图 | 手动指针 + 长度 | `Buffer::readableSpan()` 返回 `std::span`（C++20） | 零拷贝、边界安全、标准库生态兼容 |
| 错误处理 | `errno` + 日志 | `Acceptor` 使用 `std::expected`（C++23） | 显式错误传播，类型安全，避免忽略错误码 |
| 日志定位 | `__FILE__` / `__LINE__` 宏 | `std::source_location`（C++20） | 类型安全、不易被宏扩展破坏、支持默认参数 |
| 定时器排序 | `std::set`（节点型容器） | `std::flat_set` 条件编译（C++23） | 连续内存布局，减少缓存缺失和指针追逐 |
| 对象创建 | `new` / `delete` | `std::make_unique` / `std::make_shared`（C++14） | 异常安全，shared_ptr 控制块+对象合并分配 |
| 线程局部存储 | `__thread`（GCC 扩展） | `thread_local`（C++11） | 跨编译器兼容（GCC / Clang / MSVC） |
| 空指针 | `NULL`（整数 0） | `nullptr`（C++11） | 类型安全，避免重载决议歧义 |

### 3. 消除外部依赖

原 muduo 依赖 `boost::operators` 等 Boost 库，mymuduo 零外部依赖，仅需 C++ 标准库 + pthread，减少编译时间、简化部署。

### 4. 移除历史兼容代码

原 muduo 保留了大量 C++98 兼容代码（`__type_traits`、`BOOST_STATIC_ASSERT` 等），mymuduo 全部移除，代码更清晰。

### 5. 头文件双重定义统一

原 muduo 中存在 `Timestamp.h` / `MsTimestamp.h`、`Thread.h` / `MsThread.h`、`CurrentThread.h` / `MsCurrentThread.h` 双重定义，同名类但内存布局不同。mymuduo 统一为单一引用入口，详见下文"段错误根因"。

### 6. 对外 API 命名空间对齐

所有网络层类统一放入 `muduo::net::` 命名空间，基础层类放入 `muduo::`，与 muduo 官方 API 完全一致，实现 chatserver 零改动替换。

---

## 二、遇到的问题与解决方案

### 问题 1：Thread 双重定义导致段错误（最严重）

**现象：** `CountDownLatch` 成员未初始化便被访问，程序直接崩溃。

**根因：** 原 muduo 中 `MsThread.h`（基于 pthread + `CountDownLatch`）和 `Thread.h`（基于 `std::thread`）定义了同名类 `muduo::Thread` 但内存布局不同。`MsThread` 的构造函数会初始化 `CountDownLatch latch_`，而 `Thread` 的构造函数不走这个路径。链接器可能混用两个版本的构造函数和 `start()` 方法——构造函数用了 `Thread` 版本（未初始化 `latch_`），`start()` 用了 `MsThread` 版本（使用未初始化的 `latch_`），导致段错误。

**解决方案：**
- `Thread.h` 改为只引用 `MsThread.h`，不重复定义类
- 移除 `Thread.cc`，统一使用 `MsThread` 实现
- `CurrentThread.h` / `Timestamp.h` 同样处理

### 问题 2：`std::move_only_function` 拷贝错误

**现象：** 编译错误：`use of deleted function 'std::move_only_function::move_only_function(const std::move_only_function&)'`

**根因：** C++23 的 `std::move_only_function` 不可拷贝。多处代码尝试拷贝它：
- `MsThread.cc` 中 `ThreadData` 构造函数拷贝 `func_`
- `EventLoop.cc` 中 `pendingFunctors_.emplace_back(cb)` 尝试拷贝
- `EventLoop.cc` 中遍历回调队列使用 `const auto&` 导致无法调用

**解决方案：**
- 构造函数中使用 `std::move(func_)` 传递
- `emplace_back` 改为 `emplace_back(std::move(cb))`
- 遍历使用非 `const` 引用 `for (auto& cb : pendingFunctors_)`

### 问题 3：`Timer::run() const` 与 `move_only_function` 冲突

**现象：** `Timer::run()` 为 `const` 成员函数，无法调用 `std::move_only_function` 的非 `const operator()`。

**根因：** `std::move_only_function::operator()` 是非 `const` 的（因为调用可能修改内部状态）。

**解决方案：** 移除 `run()` 方法及 `callback_` 成员的 `const` 限定。

### 问题 4：`std::flat_set` 在 GCC 13 中不可用

**现象：** 编译错误 `fatal error: flat_set: No such file or directory`

**根因：** `std::flat_set` 是 C++23 特性，但 GCC 13 的标准库尚未提供 `<flat_set>` 头文件。

**解决方案：** 使用 `__has_include` 条件编译，GCC 13 回退到 `std::set`：

```cpp
#if __has_include(<flat_set>)
#include <flat_set>
#else
#include <set>
#endif
```

### 问题 5：命名空间冲突

**现象：** `Callbacks.h` 中 `namespace muduo::net { class Timestamp; }` 前置声明与实际 `muduo::Timestamp` 冲突。

**解决方案：** 使用 `using muduo::Timestamp;` 在 `muduo::net` 命名空间中引入正确类型。

### 问题 6：`InetAddress` 构造函数参数顺序不匹配

**现象：** API 文档中 `InetAddress(ip, port)` 的参数顺序与实际代码中 `InetAddress(port, ip)` 不一致。

**解决方案：** 调整为 `(const std::string& ip, uint16_t port)`，与原始 muduo API 对齐，同步修改实现文件和测试用例。

### 问题 7：`strerror` 未声明

**现象：** `Acceptor.cc` 中编译错误：`'strerror' was not declared in this scope`

**根因：** 标准库头文件未显式包含 `strerror` 的声明。

**解决方案：** 在 `Acceptor.cc` 中添加 `#include <string.h>`。

### 问题 8：头文件模块化后 `#include` 路径更新

**现象：** 将日志头文件移至 `muduo/logging/`、定时器头文件移至 `muduo/timer/` 后，所有引用旧路径的源文件编译失败。

**解决方案：** 全局搜索替换 28 处 `#include` 引用路径：
- `muduo/base/AsyncLogging.h` → `muduo/logging/AsyncLogging.h`
- `muduo/base/Logger.h` → `muduo/logging/Logger.h`
- `muduo/net/Timer.h` → `muduo/timer/Timer.h`
- 等等

覆盖 22 个文件（头文件交叉引用 + 源文件 + 测试文件 + 示例文件）。

### 问题 9：初始成员顺序警告

**现象：** GCC 报告 `EventLoop` 和 `TcpServer` 构造函数中成员初始化顺序与声明顺序不一致。

**根因：** 成员声明顺序与构造函数初始化列表顺序不一致，这是原始 muduo 遗留的问题。

**解决方案：** 这是警告而非错误，不影响运行正确性，暂保留原始代码结构以保持与 muduo 的 diff 最小化。

---

## 三、对接 chatserver 变更清单

| 步骤 | 文件 | 变更 |
|------|------|------|
| 1 | 复制 `mymuduo/` 到 `chatserver/mymuduo/` | 整个目录 |
| 2 | `chatserver/CMakeLists.txt` | 添加 `add_subdirectory(mymuduo)` |
| 3 | `chatserver/src/server/CMakeLists.txt` | `muduo_net muduo_base` → `mymuduo` |
| 4 | 业务代码 | 零改动 |

---

## 四、总结

mymuduo 自研过程的核心经验：

1. **先理解再修改。** 双重定义的类名问题是段错误的根因，必须彻底理解原始代码结构再动手。
2. **渐进式迁移。** 分 Phase 执行（Bug 修复 → 日志/定时器集成 → C++23 现代化 → 命名空间对齐 → 对接），每步编译验证，降低风险。
3. **新特性需审慎。** C++23 的 `std::flat_set`、`std::move_only_function` 等特性引入后需要充分测试兼容性（GCC 版本、标准库支持度）。
4. **工程化优先。** 模块化目录结构虽属于"体力活"，但极大提升了项目的可维护性和清晰度。
5. **API 兼容性是最高优先级。** 所有改动以"chatserver 零改动替换"为约束，确保命名空间、构造函数签名、头文件路径与原 muduo 完全一致。
