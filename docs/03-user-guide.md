# User Guide 用户指南

## 目录

1. [快速开始](#1-快速开始)
2. [构建指南](#2-构建指南)
3. [集成到项目](#3-集成到项目)
4. [使用示例](#4-使用示例)
5. [部署说明](#5-部署说明)
6. [故障排除](#6-故障排除)

---

## 1. 快速开始

### 1.1 安装依赖

#### Linux

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential cmake

# 有 Boost (推荐)
sudo apt install libboost-stacktrace-dev

# 无 Boost - 使用原生实现
# 无需额外安装
```

#### macOS

```bash
# 安装 Xcode Command Line Tools
xcode-select --install

# 有 Boost (推荐)
brew install boost

# 无 Boost - 使用原生实现
brew install libunwind
```

### 1.2 下载源码

```bash
cd /your/project/path
git clone https://github.com/your/stacktrace.git
# 或直接复制 Stacktrace 目录到你的项目
```

### 1.3 快速测试

```bash
# 配置
cmake -B build

# 编译
cmake --build build

# 运行测试
./build/hello

# 查看日志
cat stacktrace.log
```

---

## 2. 构建指南

### 2.1 CMake 选项

| 选项 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `STACKTRACE_USE_BOOST` | BOOL | ON | 启用 Boost 支持 |
| `STACKTRACE_USE_LIBUNWIND` | BOOL | ON | macOS 使用 libunwind |

### 2.2 构建命令

#### 有 Boost 环境

```bash
cmake -B build -DSTACKTRACE_USE_BOOST=ON
cmake --build build
```

#### 无 Boost 环境 (Linux)

```bash
cmake -B build -DSTACKTRACE_USE_BOOST=OFF
cmake --build build
```

#### 无 Boost 环境 (macOS)

```bash
cmake -B build \
    -DSTACKTRACE_USE_BOOST=OFF \
    -DSTACKTRACE_USE_LIBUNWIND=ON
cmake --build build
```

### 2.3 静态库构建

```bash
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF

cmake --build build --config Release
```

---

## 3. 集成到项目

### 3.1 添加到 CMake 项目

#### 方式一: 子目录

```cmake
# 你的项目 CMakeLists.txt

# 添加 Stacktrace 子目录
add_subdirectory(Stacktrace)

# 链接到你的可执行文件
add_executable(myapp src/Main.cpp)
target_link_libraries(myapp PRIVATE Stacktrace)
```

#### 方式二: 外部项目

```cmake
# 从系统路径使用
find_package(Stacktrace REQUIRED)
target_link_libraries(myapp PRIVATE Stacktrace::Stacktrace)
```

### 3.2 头文件包含

```cpp
// 使用 Stacktrace 库
#include <Stacktrace/Stacktrace.h>

// 使用 ExceptionHandler (在 src/include/ 下)
#include <ExceptionHandler.h>
```

### 3.3 完整示例

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(MyApp)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(Stacktrace)

add_executable(myapp src/Main.cpp)
target_link_libraries(myapp PRIVATE Stacktrace)

# 如果需要 Boost
find_package(Boost REQUIRED COMPONENTS stacktrace)
target_link_libraries(myapp PRIVATE Boost::boost)
```

---

## 4. 使用示例

### 4.1 基础堆栈捕获

```cpp
#include <Stacktrace/Stacktrace.h>
#include <iostream>

int main() {
    std::cout << "=== Stack Trace ===" << std::endl;
    std::cout << StacktraceNS::Stacktrace::Capture() << std::endl;
    return 0;
}
```

### 4.2 异常处理集成

```cpp
#include <ExceptionHandler.h>
#include <stdexcept>
#include <fstream>

void riskyOperation() {
    throw std::runtime_error("operation failed");
}

int main() {
    // 初始化异常处理
    ExceptionHandler::Init("crash.log");

    try {
        riskyOperation();
    } catch (const std::exception& e) {
        // 记录异常堆栈
        ExceptionHandler::LogStacktrace(
            std::string("Exception: ") + e.what()
        );
    }

    return 0;
}
```

### 4.3 自动信号捕获

```cpp
#include <ExceptionHandler.h>

int main() {
    ExceptionHandler::Init("crash.log");

    // 下面的代码如果崩溃，堆栈会自动记录
    int* p = nullptr;
    *p = 42;  // SIGSEGV

    return 0;
}
```

### 4.4 多线程应用

```cpp
#include <ExceptionHandler.h>
#include <thread>
#include <vector>

void worker(int id) {
    try {
        if (id == 3) {
            throw std::runtime_error("error in thread");
        }
    } catch (...) {
        ExceptionHandler::LogStacktrace("Worker exception");
    }
}

int main() {
    ExceptionHandler::Init("crash.log");

    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
```

### 4.5 自定义日志格式

```cpp
#include <ExceptionHandler.h>
#include <chrono>
#include <iomanip>

std::string getCustomTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

int main() {
    ExceptionHandler::Init("custom_crash.log");

    try {
        throw std::runtime_error("test");
    } catch (...) {
        ExceptionHandler::LogStacktrace(
            "[" + getCustomTimestamp() + "] Caught exception"
        );
    }

    return 0;
}
```

---

## 5. 部署说明

### 5.1 Linux 部署

#### 系统要求

- GCC 7+ 或 Clang 5+
- CMake 3.10+
- 可选: Boost 1.67+

#### 构建发布版本

```bash
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -g"

cmake --build build --config Release
```

#### 设置 Core Dump

```bash
# 无限制 core 文件大小
ulimit -c unlimited

# 设置 core 文件路径模式
echo "/tmp/core.%e.%p.%t" | sudo tee /proc/sys/kernel/core_pattern

# 或使用 systemd
sudo mkdir -p /var/lib/systemd/coredump
sudo chmod 777 /var/lib/systemd/coredump
```

### 5.2 macOS 部署

#### 系统要求

- macOS 10.14+
- Xcode 10+
- 可选: Boost 1.70+

#### 构建发布版本

```bash
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14

cmake --build build --config Release
```

#### 设置 Core Dump

```bash
# 查看当前配置
launchctl limit core

# 设置无限制 (需要 sudo)
sudo launchctl limit core unlimited
```

### 5.3 日志管理

#### 日志轮转

使用 `logrotate` 配置:

```bash
# /etc/logrotate.d/myapp
/var/log/myapp/*.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 0644 root root
}
```

#### 权限配置

```bash
# 创建日志目录
sudo mkdir -p /var/log/myapp
sudo chown myapp:myapp /var/log/myapp

# 在代码中初始化
ExceptionHandler::Init("/var/log/myapp/crash.log");
```

---

## 6. 故障排除

### 6.1 编译错误

#### "Stacktrace.h not found"

```cmake
# 确保正确添加子目录
add_subdirectory(Stacktrace)
target_include_directories(myapp PRIVATE ${CMAKE_SOURCE_DIR}/Stacktrace/include)
```

#### "undefined reference to backtrace"

```bash
# Linux: 确保链接了 dl
target_link_libraries(myapp PRIVATE dl)

# 或使用 CMake 自动检测
find_library(DL_LIBRARY dl)
target_link_libraries(myapp PRIVATE ${DL_LIBRARY})
```

#### "libunwind not found" on macOS

```bash
# 安装 libunwind
brew install libunwind

# 或设置路径
cmake -B build -DLIBUNWIND_ROOT=/opt/homebrew
```

### 6.2 运行时问题

#### 日志文件未生成

1. 检查写入权限
2. 确保目录存在
3. 验证 `Init()` 被调用

```cpp
// 调试
int main() {
    try {
        ExceptionHandler::Init("/tmp/test.log");
        // ...
    } catch (const std::exception& e) {
        std::cerr << "Init failed: " << e.what() << std::endl;
    }
}
```

#### 堆栈信息不完整 (无符号)

**Linux 解决方案:**

```bash
# 编译时添加 -rdynamic
set(CMAKE_EXE_LINKER_FLAGS "-rdynamic")
# 或
target_link_libraries(myapp PRIVATE -rdynamic)
```

**macOS 解决方案:**

```bash
# 禁用符号剥离
set(CMAKE_EXE_LINKER_FLAGS "-Wl,-no_warn_unused_dummies")
```

#### 信号处理后程序异常退出

确保使用 `std::_Exit()` 而非 `std::abort()`:

```cpp
std::signal(SIGSEGV, [](int) {
    LogStacktrace("SIGSEGV");
    std::_Exit(1);  // 正确
    // std::abort();  // 可能导致信号循环
});
```

### 6.3 性能问题

#### 堆栈捕获慢

- 减少捕获帧数: `Stacktrace(32)` 而非默认 64
- 延迟初始化: 仅在需要时调用 `Init()`

#### 文件写入阻塞

```cpp
// 使用缓冲
class BufferedLogger {
    std::ostringstream buffer;
    std::mutex mutex;
public:
    void log(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex);
        buffer << msg << std::endl;
        if (buffer.str().size() > 4096) {
            flush();
        }
    }
};
```

---

## 附录: 测试用例

### 运行测试

```bash
# 全部安全测试
./build/hello

# 单个测试
./build/hello 1    # 深调用栈
./build/hello 5     # SIGSEGV
./build/hello 20    # SIGABRT

# 测试列表
./build/hello
```

### 测试用例清单

| # | 场景 | 类型 |
|---|------|------|
| 1 | 深调用栈 | Safe |
| 2 | 嵌套异常 | Safe |
| 3 | 子线程异常 | Crash |
| 4 | 自定义异常 | Safe |
| 5 | SIGSEGV | Crash |
| 6 | SIGFPE | Crash |
| 7 | std::logic_error | Safe |
| 8 | std::runtime_error | Safe |
| 9 | std::out_of_range | Safe |
| 10 | std::invalid_argument | Safe |
| 11 | std::length_error | Safe |
| 12 | std::overflow_error | Safe |
| 13 | std::underflow_error | Safe |
| 14 | Re-throw Exception | Safe |
| 15 | 多次连续异常 | Safe |
| 16 | 内嵌异常 | Safe |
| 17 | Async/Future | Safe |
| 18 | Void Pointer | Crash |
| 19 | 栈溢出 | Crash |
| 20 | SIGABRT | Crash |
| 21 | SIGILL | Crash |
| 22 | SIGTRAP | Crash |
| 23 | std::bad_alloc | Safe |
| 24 | std::bad_cast | Safe |
| 25 | std::bad_typeid | Safe |
| 26 | std::bad_exception | Safe |
| 27 | 空函数指针 | Crash |
| 28 | 断言失败 | Safe |
| 29 | 构造函数异常 | Safe |
| 30 | 析构函数异常 | Crash |
| 31 | SIGPIPE | Crash |
| 32 | SIGSYS | Crash |

---

## 命名规范

| 类型 | 规范 | 示例 |
|------|------|------|
| 文件名 | 大写开头 | `ExceptionHandler.h`, `Stacktrace.cpp` |
| 类名 | 大写开头 | `class ExceptionHandler`, `class Stacktrace` |
| 函数名 | 大写开头 | `Init()`, `LogStacktrace()`, `Capture()` |
| 变量名 | 小驼峰 | `logFilename`, `maxFrames`, `signalNum` |
| 命名空间 | 大写开头 | `StacktraceNS` |
| 成员变量 | 小驼峰+下划线 | `logFilename_` |
