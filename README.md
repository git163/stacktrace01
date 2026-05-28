# Stacktrace

跨平台 C++ 堆栈捕获库，支持 Linux、macOS 环境下的调用堆栈获取与日志记录。

## 功能特性

- **跨平台支持**: Linux、macOS
- **多后端实现**: Boost.Stacktrace、glibc backtrace、libunwind
- **信号处理**: 捕获 SIGSEGV、SIGABRT、SIGFPE、SIGBUS、SIGILL、SIGTRAP、SIGPIPE、SIGSYS 等信号
- **线程安全**: 安全的日志写入与重入保护
- **C++17**: 使用现代 C++ 特性
- **日志钩子**: 支持自定义日志处理函数，方便线上部署集成

## 快速开始

```bash
# 方式一：使用构建脚本
./scripts/build.sh

# 方式二：手动构建
cmake -B build
cmake --build build

# 运行测试
./build/hello
```

## 日志钩子使用

```cpp
#include "ExceptionHandler.h"

// 自定义日志处理函数
void myLogHook(const std::string& logPath, const std::string& content) {
    // 可以发送到 syslog、远程日志服务等
    printf("Log hook called: %s\n", logPath.c_str());
}

int main() {
    // 设置日志钩子
    ExceptionHandler::SetLogHook(myLogHook);

    // 初始化
    ExceptionHandler::Init("/var/log/myapp.log");
    // ...
}
```

## 文档

- [设计文档](docs/01-design.md) - 架构设计与实现策略
- [API 文档](docs/02-api.md) - 接口说明与使用示例
- [用户指南](docs/03-user-guide.md) - 构建、集成、部署指南

## 项目结构

```
.
├── CMakeLists.txt              # 主构建配置
├── src/
│   ├── include/
│   │   └── ExceptionHandler.h  # 异常处理接口
│   ├── Main.cpp                # 测试入口
│   └── ExceptionHandler.cpp     # 异常处理实现
├── stacktrace/                 # Stacktrace 子库
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── Stacktrace/
│   │       └── Stacktrace.h    # 统一接口
│   └── src/
│       ├── Stacktrace.cpp
│       ├── StacktraceLinuxNative.cpp
│       ├── StacktraceMacNative.cpp
│       └── StacktraceFallback.cpp
└── docs/
    ├── 01-design.md
    ├── 02-api.md
    └── 03-user-guide.md
```

## 支持的测试场景

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

## 命名规范

| 类型 | 规范 | 示例 |
|------|------|------|
| 文件名 | 大写开头 | `ExceptionHandler.h`, `Stacktrace.cpp` |
| 类名 | 大写开头 | `class ExceptionHandler`, `class Stacktrace` |
| 函数名 | 大写开头 | `Init()`, `LogStacktrace()`, `Capture()` |
| 变量名 | 小驼峰 | `logFilename`, `maxFrames`, `signalNum` |
| 命名空间 | 大写开头 | `StacktraceNS` |
| 成员变量 | 小驼峰+下划线 | `logFilename_` |
