# Stacktrace 库功能设计文档

## 1. 概述

`Stacktrace` 是一个跨平台的 C++ 堆栈捕获库，支持在 Linux、macOS 等主流操作系统上获取调用堆栈信息。

### 1.1 设计目标

- **跨平台兼容**: 支持 Linux、macOS
- **零依赖可选**: 可选择使用 Boost 或原生 API
- **统一接口**: 提供一致的 C++ API，屏蔽平台差异
- **线程安全**: 堆栈捕获和日志写入均为线程安全

### 1.2 支持的平台实现

| 平台 | 有 Boost | 无 Boost |
|------|---------|---------|
| Linux | Boost.Stacktrace | glibc `backtrace()` + `dladdr()` |
| macOS | Boost.Stacktrace | libunwind 或 `execinfo.h` |
| 其他 | 降级方案 | 仅输出地址 |

---

## 2. 架构设计

### 2.1 目录结构

```
.
├── CMakeLists.txt                    # 主项目构建配置
├── src/
│   ├── include/
│   │   └── ExceptionHandler.h      # 异常处理接口
│   ├── Main.cpp                     # 测试入口
│   └── ExceptionHandler.cpp        # 异常处理实现
└── Stacktrace/                      # Stacktrace 子库
    ├── CMakeLists.txt
    ├── include/
    │   └── Stacktrace/
    │       └── Stacktrace.h         # 统一接口
    └── src/
        ├── Stacktrace.cpp           # 主实现 (Boost 路径)
        ├── StacktraceDemangle.cpp   # 符号解混淆
        ├── StacktraceLinuxNative.cpp
        ├── StacktraceMacNative.cpp
        └── StacktraceFallback.cpp
```

### 2.2 核心组件

#### Stacktrace 类

```cpp
namespace StacktraceNS {

class Stacktrace {
public:
    static const size_t DEFAULT_MAX_FRAMES = 64;

    explicit Stacktrace(size_t maxFrames = DEFAULT_MAX_FRAMES);
    ~Stacktrace();

    std::string ToString() const;
    std::string ToStringSimple() const;

    const std::vector<Frame>& Frames() const;
    size_t Size() const;
    bool IsEmpty() const;

    static std::string Capture(size_t maxFrames = DEFAULT_MAX_FRAMES);

private:
    void CaptureImpl();
    std::vector<Frame> frames_;
};

}
```

#### Frame 结构

```cpp
struct Frame {
    void* address = nullptr;      // 指令地址
    std::string symbol;           // 函数符号名 (未解混淆)
    std::string filename;        // 源文件路径
    int lineNumber = 0;          // 行号
};
```

### 2.3 实现策略

#### Boost 路径 (优先)

当 Boost 可用时，使用 `boost::stacktrace::stacktrace`，提供：
- 自动符号解析
- 行号信息
- 跨平台一致性

#### Linux 原生路径

```
backtrace()           → 获取原始地址数组
dladdr()              → 地址 → 符号/文件
abi::__cxa_demangle() → C++ 符号解混淆
```

#### macOS 原生路径

```
libunwind:
    unw_getcontext()   → 获取上下文
    unw_init_local()   → 初始化游标
    unw_step()         → 逐帧遍历
    unw_get_proc_name()→ 获取函数名
    dladdr()           → 补充文件信息
```

---

## 3. ExceptionHandler 设计

### 3.1 功能概述

`ExceptionHandler` 封装了信号处理和异常捕获机制，提供：
- 多信号拦截 (SIGSEGV, SIGABRT, SIGFPE, etc.)
- 堆栈日志写入
- 线程安全保护
- 防重入设计

### 3.2 处理的信号

| 信号 | 含义 | 触发场景 |
|------|------|---------|
| SIGSEGV | 段错误 | 空指针、内存越界访问 |
| SIGABRT | 中止 | assert 失败、abort() 调用 |
| SIGFPE | 浮点异常 | 除零、溢出 |
| SIGBUS | 总线错误 | 对齐错误 |
| SIGILL | 非法指令 | 无效机器码 |
| SIGTRAP | 追踪陷阱 | 调试断点 |
| SIGPIPE | 管道断开 | 写入已关闭的 pipe/socket |
| SIGSYS | 系统调用错误 | 非法系统调用号 |

### 3.3 核心机制

#### 信号处理

```cpp
std::signal(SIGSEGV, [](int sig) {
    LogStacktrace("SIGSEGV", sig);
    std::_Exit(1);  // 使用 _Exit 而非 abort() 避免信号循环
});
```

#### 防重入保护

```cpp
std::atomic<bool> loggingInProgress(false);

void LogStacktrace(const std::string& reason, int signalNum) {
    // 防止信号处理函数重入
    if (loggingInProgress.exchange(true)) return;
    // ... 写入日志
    loggingInProgress = false;
}
```

#### 互斥锁保护

```cpp
std::mutex logMutex;

void LogStacktrace(...) {
    std::lock_guard<std::mutex> lock(logMutex);
    // 写入文件
}
```

---

## 4. 日志格式设计

### 4.1 日志条目结构

```
╔════════════════════════════════════════════════════════════════════════╗
║ STACK TRACE LOG ENTRY                                                    ║
╠════════════════════════════════════════════════════════════════════════╣
║ Timestamp: 2026-04-29 22:49:52.091
║ Thread ID:  0x2001f2140
║ Reason:     std::exception: deep exception in level3
║ Signal:     SIGSEGV (Invalid memory access) (11)
╠════════════════════════════════════════════════════════════════════════╣
║ Stack Trace:                                                             ║
╚════════════════════════════════════════════════════════════════════════╝
 0# StacktraceNS::Stacktrace::ToString() const in ./build/hello
 1# StacktraceNS::Stacktrace::Capture() in ./build/hello
 2# WriteLogHeader(...) in ./build/hello
 ...
╔════════════════════════════════════════════════════════════════════════╗
║ END OF STACK TRACE                                                      ║
╚════════════════════════════════════════════════════════════════════════╝
```

### 4.2 字段说明

| 字段 | 说明 |
|------|------|
| Timestamp | ISO 8601 格式，精确到毫秒 |
| Thread ID | 十六进制线程标识符 |
| Reason | 异常描述或信号名 |
| Signal | 信号名称和编号 (仅信号处理时) |
| Stack Trace | 堆栈帧列表 |

---

## 5. 构建配置

### 5.1 CMake 选项

| 选项 | 默认值 | 说明 |
|------|-------|------|
| `STACKTRACE_USE_BOOST` | ON | 优先使用 Boost |
| `STACKTRACE_USE_LIBUNWIND` | ON | macOS 上使用 libunwind |

### 5.2 链接要求

#### Linux (无 Boost)

```cmake
target_link_libraries(myapp PRIVATE Stacktrace)
# 自动链接: dl, pthread
```

#### macOS (无 Boost)

```cmake
# 需要安装 libunwind
brew install libunwind

target_link_libraries(myapp PRIVATE Stacktrace)
# 自动链接: dl, pthread, unwind
```

---

## 6. 线程安全

### 6.1 设计原则

1. **原子操作**: 使用 `std::atomic<bool>` 防止信号处理重入
2. **互斥锁**: 使用 `std::mutex` 保护文件写入
3. **异常安全**: `std::lock_guard` 保证锁的释放

### 6.2 限制

- 信号处理函数中不可调用非 async-signal-safe 函数
- Boost.Stacktrace 在信号处理中的安全性取决于平台

---

## 7. 性能考虑

| 操作 | 耗时 (典型) |
|------|------------|
| 堆栈捕获 | 0.5-2 ms |
| 符号解析 | 1-10 ms/帧 |
| 文件写入 | 0.1-1 ms |

建议仅在异常/崩溃路径中捕获堆栈，避免影响正常业务性能。

---

## 8. 命名规范

### 8.1 命名约定

| 类型 | 规范 | 示例 |
|------|------|------|
| 文件名 | 大写开头 | `ExceptionHandler.h`, `Stacktrace.cpp` |
| 类名 | 大写开头 | `class ExceptionHandler`, `class Stacktrace` |
| 函数名 | 大写开头 | `Init()`, `LogStacktrace()`, `Capture()` |
| 变量名 | 小驼峰 | `logFilename`, `maxFrames`, `signalNum` |
| 命名空间 | 大写开头 | `StacktraceNS` |
| 成员变量 | 小驼峰+下划线 | `logFilename_` |
