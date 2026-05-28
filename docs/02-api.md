# API 接口文档

## Stacktrace 库

### 命名空间

```cpp
namespace StacktraceNS {}
```

---

## 核心类

### Stacktrace

堆栈捕获类，用于获取当前线程的调用堆栈。

#### 构造函数

```cpp
explicit Stacktrace(size_t maxFrames = DEFAULT_MAX_FRAMES);
```

**参数:**
- `maxFrames`: 最大捕获帧数，默认 64

**示例:**
```cpp
StacktraceNS::Stacktrace st;           // 捕获默认深度
StacktraceNS::Stacktrace st(128);    // 捕获更深栈
```

#### 析构函数

```cpp
~Stacktrace();
```

#### 静态方法

##### Capture

```cpp
static std::string Capture(size_t maxFrames = DEFAULT_MAX_FRAMES);
```

一次性捕获并返回堆栈字符串。

**参数:**
- `maxFrames`: 最大捕获帧数

**返回:** 格式化的堆栈字符串

**示例:**
```cpp
std::string trace = StacktraceNS::Stacktrace::Capture();

// 在异常处理中使用
try {
    riskyOperation();
} catch (...) {
    auto trace = StacktraceNS::Stacktrace::Capture();
    logToFile(trace);
    throw;
}
```

#### 实例方法

##### ToString

```cpp
std::string ToString() const;
```

获取格式化的堆栈字符串。

**返回:** 包含符号名、文件、行号的完整堆栈信息

**示例:**
```cpp
StacktraceNS::Stacktrace st;
std::cout << st.ToString();
```

##### ToStringSimple

```cpp
std::string ToStringSimple() const;
```

获取简化格式的堆栈字符串。

**返回:** 仅包含函数名和地址

**示例:**
```cpp
StacktraceNS::Stacktrace st;
std::string simple = st.ToStringSimple();
```

##### Frames

```cpp
const std::vector<Frame>& Frames() const;
```

获取原始帧数据。

**返回:** Frame 结构体向量

##### Size

```cpp
size_t Size() const;
```

获取捕获的帧数。

##### IsEmpty

```cpp
bool IsEmpty() const;
```

检查是否为空。

---

## 数据结构

### Frame

```cpp
struct Frame {
    void* address = nullptr;      // 指令地址
    std::string symbol;          // 函数符号名
    std::string filename;        // 源文件路径
    int lineNumber = 0;          // 行号
};
```

---

## ExceptionHandler 库

### 命名空间

```cpp
// 全局函数，无命名空间
```

---

## 全局函数

### ExceptionHandler::Init

初始化异常处理器，注册信号处理和终止处理。

```cpp
static void Init(const std::string& logFilename = "stacktrace.log");
```

**参数:**
- `logFilename`: 日志文件路径，默认 `stacktrace.log`

**示例:**
```cpp
int main() {
    ExceptionHandler::Init("/var/log/myapp/crash.log");

    // 或使用默认文件名
    ExceptionHandler::Init();
    // ...
}
```

---

### ExceptionHandler::LogStacktrace

手动记录堆栈到日志文件。

```cpp
static void LogStacktrace(const std::string& reason, int signalNum = 0);
```

**参数:**
- `reason`: 异常原因描述
- `signalNum`: 信号编号 (仅在信号处理中使用)

**示例:**
```cpp
try {
    operation();
} catch (const std::exception& e) {
    ExceptionHandler::LogStacktrace(
        std::string("Caught exception: ") + e.what()
    );
}
```

---

### ExceptionHandler::GetLogFilename

获取当前日志文件名。

```cpp
static std::string GetLogFilename();
```

**返回:** 日志文件路径

**示例:**
```cpp
std::string logFile = ExceptionHandler::GetLogFilename();
std::cout << "Log file: " << logFile << std::endl;
```

---

## 宏定义

### LOG_INFO

带时间戳的信息日志输出。

```cpp
#define LOG_INFO(msg) \
    std::cout << "[" << GetTimestamp() << "] [INFO] " << msg << std::endl
```

### LOG_ERROR

带时间戳的错误日志输出。

```cpp
#define LOG_ERROR(msg) \
    std::cerr << "[" << GetTimestamp() << "] [ERROR] " << msg << std::endl
```

---

## 使用示例

### 基础用法

```cpp
#include <Stacktrace/Stacktrace.h>
#include <iostream>

int main() {
    // 捕获当前堆栈
    auto trace = StacktraceNS::Stacktrace::Capture();
    std::cout << trace << std::endl;
    return 0;
}
```

### 异常处理集成

```cpp
#include <ExceptionHandler.h>
#include <stdexcept>

void riskyFunction() {
    throw std::runtime_error("Something went wrong");
}

int main() {
    ExceptionHandler::Init("error.log");

    try {
        riskyFunction();
    } catch (const std::exception& e) {
        // 记录异常堆栈
        ExceptionHandler::LogStacktrace(
            std::string("Exception: ") + e.what()
        );
    }

    return 0;
}
```

### 信号处理 (自动捕获)

```cpp
#include <ExceptionHandler.h>

int main() {
    ExceptionHandler::Init("crash.log");

    // 空指针解引用会触发 SIGSEGV
    // 堆栈会自动记录到 crash.log
    int* p = nullptr;
    *p = 42;

    return 0;
}
```

### 多线程环境

```cpp
#include <ExceptionHandler.h>
#include <thread>

void worker() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    throw std::runtime_error("error in worker");
}

int main() {
    ExceptionHandler::Init("multithread.log");

    std::thread t(worker);
    t.join();

    return 0;
}
```

---

## 错误处理

### 返回值

| 场景 | 返回值 |
|------|--------|
| 成功 | 堆栈字符串 (可能为空) |
| 捕获失败 | 空字符串或仅含地址的堆栈 |

### 线程安全

- `Stacktrace::Capture()` - 线程安全
- `ExceptionHandler::LogStacktrace()` - 线程安全

### 信号处理限制

在信号处理函数中:
- 仅调用 async-signal-safe 函数
- 使用 `std::_Exit()` 而非 `std::abort()`
- 启用防重入机制

---

## 平台差异

### Linux

```cpp
// 可用方法
StacktraceNS::Stacktrace::Capture();  // 使用 backtrace()
```

### macOS

```cpp
// 可用方法
StacktraceNS::Stacktrace::Capture();  // 使用 libunwind 或 execinfo
```

### 符号解析

| 平台 | 有 Boost | 无 Boost |
|------|---------|---------|
| Linux | ✓ 行号 | ✓ 函数名 |
| macOS | ✓ 行号 | ✓ 函数名 |

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
