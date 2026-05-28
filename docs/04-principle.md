# ExceptionHandler 工作原理

## 目录

1. [概述](#1-概述)
2. [Init 注册的三道防线](#2-init-注册的三道防线)
3. [崩溃时的信号处理流程](#3-崩溃时的信号处理流程)
4. [正常 C++ 异常的堆栈捕获](#4-正常-c-异常的堆栈捕获)
5. [符号解析与行号还原](#5-符号解析与行号还原)
6. [两种场景对比总结](#6-两种场景对比总结)

---

## 1. 概述

`ExceptionHandler` 同时覆盖了**两条异常路径**：

- **主动路径**：代码中 `throw` 异常或在 `try/catch` 里手动调用 `LogStacktrace()`，属于正常的 C++ 控制流。
- **崩溃路径**：发生空指针解引用、除零、栈溢出等硬件/内核异常，进程收到 `SIGSEGV`、`SIGABRT`、`SIGFPE` 等致命信号。

两条路径的底层机制完全不同。崩溃路径的限制极为严苛——信号处理函数运行在内核强制切入的上下文中，几乎不能调用任何标准库函数，否则会引发死锁或二次崩溃。

---

## 2. Init 注册的三道防线

`ExceptionHandler::Init()` 在进程启动阶段预埋了三层防护：

### 2.1 未捕获异常兜底（std::set_terminate）

```cpp
std::set_terminate([]() {
    LogStacktrace("std::terminate called (uncaught exception)");
    std::_Exit(1);
});
```

当异常一直抛到 `main` 之外、没有任何 `catch` 能接住时，C++ 运行时调用 `std::terminate()`。这里把它重定向为记录堆栈后干净退出。

### 2.2 备用信号栈（sigaltstack）

```cpp
static constexpr size_t ALT_STACK_SIZE = 64 * 1024;
alignas(16 * 1024) static char altStack[ALT_STACK_SIZE];
stack_t ss;
ss.ss_sp = altStack;
ss.ss_size = ALT_STACK_SIZE;
ss.ss_flags = 0;
sigaltstack(&ss, nullptr);
```

**为什么需要备用栈？**

如果进程已经**栈溢出**（Stack Overflow），主栈空间已耗尽。此时再触发 `SIGSEGV`，若信号处理函数仍运行在主栈上，会立即二次崩溃（连信号 handler 都跑不起来）。`sigaltstack` 提前分配一块独立的内存区域，确保 handler 有栈可用。

### 2.3 信号处理器注册（sigaction）

```cpp
struct sigaction sa;
sa.sa_handler = [](int sig) { SignalSafeLogStacktrace(sig); _Exit(1); };
sigemptyset(&sa.sa_mask);
sa.sa_flags = SA_ONSTACK;  // 关键：在备用栈上执行 handler

sigaction(SIGSEGV, &sa, nullptr);
sigaction(SIGBUS,  &sa, nullptr);
sigaction(SIGABRT, &sa, nullptr);
sigaction(SIGFPE,  &sa, nullptr);
sigaction(SIGILL,  &sa, nullptr);
sigaction(SIGTRAP, &sa, nullptr);
sigaction(SIGPIPE, &sa, nullptr);
sigaction(SIGSYS,  &sa, nullptr);
```

覆盖的信号说明：

| 信号 | 典型触发场景 |
|------|-------------|
| `SIGSEGV` | 空指针解引用、越界访问 |
| `SIGBUS` | 内存对齐错误、不可访问的物理地址 |
| `SIGABRT` | `abort()`、`assert` 失败、检测到堆损坏 |
| `SIGFPE` | 整数除零、浮点异常 |
| `SIGILL` | 非法指令、二进制损坏 |
| `SIGTRAP` | 断点、调试器陷阱 |
| `SIGPIPE` | 向已关闭的管道/socket 写数据 |
| `SIGSYS` | 调用了被 seccomp 禁止的系统调用 |

**`SA_ONSTACK` 标志至关重要**：它告诉内核，该信号的处理函数必须在备用栈上执行，而不是在主栈上。

---

## 3. 崩溃时的信号处理流程

当执行 `int* p = nullptr; *p = 42;` 时，MMU 检测到非法内存访问，CPU 陷入内核态，内核向进程投递 `SIGSEGV`。此时程序计数器被强制跳转到注册的信号 handler。

### 3.1 信号安全写日志（SignalSafeLogStacktrace）

```cpp
static void SignalSafeLogStacktrace(int sig) {
    // 防重入：handler 内部不能再触发信号，否则直接退出
    if (signalSafeBufferInUse.exchange(true)) {
        const char* msg = "[FATAL] SignalSafeLogStacktrace re-entrant call, aborting\n";
        write(STDERR_FILENO, msg, strlen(msg));
        _Exit(1);
    }

    // 只用系统调用打开文件，不能用 C++ fstream
    int fd = open(gLogFilenamePtr ? gLogFilenamePtr : "stacktrace.log",
                  O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        signalSafeBufferInUse = false;
        _Exit(1);
    }

    // 写头部信息（时间、线程ID、信号名）——全部用信号安全函数组装
    char header[512];
    int n = snprintf(header, sizeof(header),
        "\n=== SIGNAL CAUGHT ===\nSignal: %d (%s)\nTimestamp: %s\nThread: %s\n",
        sig, sigName, timestamp, threadId);
    if (n > 0 && n < (int)sizeof(header)) {
        write(fd, header, n);
    }

    // ===== 核心：捕获堆栈 =====
    void* buffer[64];
    int size = backtrace(buffer, 64);           // 获取返回地址数组
    backtrace_symbols_fd(buffer, size, fd);     // 直接将符号信息写入文件描述符

    const char* footer = "\n=== END OF SIGNAL STACK TRACE ===\n\n";
    write(fd, footer, strlen(footer));
    close(fd);

    signalSafeBufferInUse = false;
}
```

### 3.2 为什么不能用 Stacktrace::Capture()？

信号处理函数处于**极端受限的执行上下文**。进程可能在任何时刻被中断——比如 `malloc` 刚拿到锁、一个 `std::string` 正在扩容、一个 `fstream` 正在 flush buffer。如果信号 handler 里再调用这些函数，极有可能**死锁**或**二次崩溃**。

因此信号 handler 只能使用 **async-signal-safe** 的函数。POSIX 标准中明确列为信号安全的包括：

| 允许使用 | 禁止使用的（危险） |
|---------|------------------|
| `backtrace()` | `malloc()` / `free()` / `new` |
| `backtrace_symbols_fd()` | `backtrace_symbols()`（内部会 malloc） |
| `write()` / `read()` / `open()` / `close()` | `printf()` / `cout` / `fstream` |
| `snprintf()`（部分实现） | `exit()`（会调用 atexit 和全局析构） |
| `_Exit()` / `_exit()` | `std::terminate()` |

**`_Exit(1)` vs `exit(1)` 的区别**：
- `exit()` 会刷新 stdio 缓冲区、调用 `atexit` 注册的函数、执行全局对象的析构函数。
- `_Exit()` 直接让进程消失，什么都不做。

在崩溃状态下，全局对象可能已经处于不一致状态，调用析构函数反而会加剧破坏。因此必须用 `_Exit()`。

---

## 4. 正常 C++ 异常的堆栈捕获

当代码主动 `throw StacktraceException(...)` 或在 `try/catch` 里调用 `ExceptionHandler::LogStacktrace()` 时，走的是正常 C++ 流程，没有信号安全的限制。

### 4.1 调用链

```cpp
void ExceptionHandler::LogStacktrace(const std::string& reason, int signalNum) {
    LogStacktrace(reason, StacktraceNS::Stacktrace::Capture(), signalNum);
}
```

```cpp
std::string Stacktrace::Capture(size_t maxFrames) {
    Stacktrace st(maxFrames);
    return st.ToString();
}
```

### 4.2 平台相关的 CaptureImpl

根据编译时配置，有三套实现：

#### 方案 A：Boost.Stacktrace（推荐）

```cpp
void Stacktrace::CaptureImpl() {
    const auto& st = boost::stacktrace::stacktrace();
    for (size_t i = 0; i < st.size(); ++i) {
        Frame frame;
        frame.address = const_cast<void*>(st[i].address());
        frame.symbol = st[i].name();
        frame.filename = st[i].source_file();
        frame.lineNumber = static_cast<int>(st[i].source_line());
        frames_.push_back(frame);
    }
}
```

Boost 直接给出符号名、源文件和行号，最方便。

#### 方案 B：macOS 原生（libunwind）

```cpp
void Stacktrace::CaptureImpl() {
    unw_cursor_t cursor;
    unw_context_t context;
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    unw_step(&cursor);  // 跳过本函数
    while (unw_step(&cursor) > 0) {
        unw_word_t ip;
        unw_get_reg(&cursor, UNW_REG_IP, &ip);

        Frame frame;
        frame.address = reinterpret_cast<void*>(ip);

        char symbol[256] = {0};
        if (unw_get_proc_name(&cursor, symbol, sizeof(symbol) - 1, nullptr) == 0) {
            frame.symbol = symbol;
        }

        DladdrResolve(frame.address, frame);  // dladdr 补全模块路径
        frames_.push_back(frame);
    }
}
```

`libunwind` 是 macOS 上比 `backtrace()` 更可靠的方式，能正确遍历 ARM64 的栈帧。

#### 方案 C：Linux 原生（glibc backtrace）

```cpp
void Stacktrace::CaptureImpl() {
    void* buffer[DEFAULT_MAX_FRAMES];
    int size = backtrace(buffer, static_cast<int>(DEFAULT_MAX_FRAMES));

    // 跳过前 2 帧：CaptureImpl 和 Stacktrace 构造函数
    for (int i = 2; i < size; ++i) {
        Frame frame;
        frame.address = buffer[i];
        DladdrResolve(buffer[i], frame);
        frames_.push_back(frame);
    }
}
```

`backtrace()` 是 glibc 提供的函数，内部遍历栈帧的返回地址。`DladdrResolve` 用 `dladdr()` 把地址翻译为模块路径和符号名。

---

## 5. 符号解析与行号还原

`backtrace()` 和 `libunwind` 只能拿到**返回地址**和**原始符号名**（如 `_ZN12StacktraceNS15StacktraceExceptionC1ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE`）。需要进一步还原为：

1. **可读的函数名**（demangle）
2. **源文件路径**
3. **行号**

### 5.1 C++ 符号还原（Demangle）

```cpp
std::string Demangle(const char* name) {
    int status = 0;
    std::unique_ptr<char, decltype(&std::free)> demangled(
        abi::__cxa_demangle(name, nullptr, nullptr, &status),
        std::free
    );
    if (status == 0 && demangled) {
        return demangled.get();
    }
    return name;
}
```

`abi::__cxa_demangle` 是 Itanium C++ ABI 标准函数，把编译后的 mangled name 还原为人类可读的 `StacktraceNS::StacktraceException::StacktraceException(std::string const&)`。

### 5.2 源文件与行号还原

Boost.Stacktrace 自带行号解析。原生实现需要借助外部工具：

**macOS —— atos**

```cpp
snprintf(cmd, sizeof(cmd),
    "atos -o \"%s\" -l 0x%lx 0x%lx 2>/dev/null | head -1",
    info.dli_fname, reinterpret_cast<uintptr_t>(info.dli_fbase),
    reinterpret_cast<uintptr_t>(address));
```

atos 输出格式：`symbolName (in binaryName) (file.cpp:123)`

**Linux —— addr2line**

```cpp
snprintf(cmd, sizeof(cmd),
    "addr2line -e /proc/self/exe -f 0x%lx 2>/dev/null | head -2",
    reinterpret_cast<uintptr_t>(address));
```

addr2line 输出格式：
```
FunctionName
/path/to/file.cpp:123
```

### 5.3 注意事项

- 行号解析依赖**调试信息**（`-g` 编译选项）。Release 构建若 strip 了符号表，`dladdr` 只能返回空或模块名，`atos`/`addr2line` 也无法工作。
- Linux 下链接时加 `-rdynamic` 可以让动态符号表保留更多信息，提高 `backtrace_symbols` 的解析成功率。

---

## 6. 两种场景对比总结

| 对比项 | 主动捕获（try/catch） | 崩溃捕获（信号） |
|--------|----------------------|-----------------|
| **触发方式** | `throw` / 手动调用 `LogStacktrace()` | `SIGSEGV`、`SIGABRT` 等 |
| **注册机制** | 无（C++ 异常机制本身） | `sigaction` + `sigaltstack` |
| **堆栈获取** | `Stacktrace::Capture()` → `backtrace()` / `libunwind` / Boost | `backtrace()` |
| **符号解析** | `dladdr()` + `atos`/`addr2line` | `backtrace_symbols_fd()` |
| **写日志方式** | `std::ofstream`（正常 C++ IO） | `open()`/`write()`/`close()`（系统调用） |
| **函数限制** | 无限制 | **只能使用 async-signal-safe 函数** |
| **退出方式** | 继续执行或正常 throw | `_Exit(1)`（直接退出，不调用析构） |
| **栈溢出保护** | 不适用 | `sigaltstack` 提供备用栈 |

### 整体流程图

```
主动路径（C++ 异常）
====================
throw StacktraceException(...)
        │
        ▼
catch (const StacktraceException& e)
        │
        ▼
ExceptionHandler::LogStacktrace(reason, e.GetStacktrace())
        │
        ▼
Stacktrace::Capture() ──→ backtrace() / libunwind / Boost
        │                      │
        │                      ▼
        │              dladdr() 解析符号
        │                      │
        │                      ▼
        │              atos / addr2line 解析行号
        │                      │
        └──────────────────────┘
        │
        ▼
std::ofstream 写日志文件（格式化输出）


崩溃路径（信号）
================
*p = nullptr
        │
        ▼
MMU 触发段错误 → 内核投递 SIGSEGV
        │
        ▼
sigaction() 注册的 handler
        │
        ▼
SignalSafeLogStacktrace(sig)
        │
        ├── open() 打开日志文件
        ├── write() 写头部信息
        ├── backtrace() 获取地址数组
        ├── backtrace_symbols_fd() 直接写入符号
        ├── write() 写尾部信息
        ├── close() 关闭文件
        └── _Exit(1) 直接退出
```

---

## 相关文件

- [`src/ExceptionHandler.cpp`](../src/ExceptionHandler.cpp) — 信号注册与日志记录
- [`src/include/ExceptionHandler.h`](../src/include/ExceptionHandler.h) — 对外接口
- [`stacktrace/include/stacktrace/Stacktrace.h`](../stacktrace/include/stacktrace/Stacktrace.h) — 堆栈捕获类
- [`stacktrace/src/Stacktrace.cpp`](../stacktrace/src/Stacktrace.cpp) — 平台无关实现
- [`stacktrace/src/StacktraceMacNative.cpp`](../stacktrace/src/StacktraceMacNative.cpp) — macOS 原生实现
- [`stacktrace/src/StacktraceLinuxNative.cpp`](../stacktrace/src/StacktraceLinuxNative.cpp) — Linux 原生实现
