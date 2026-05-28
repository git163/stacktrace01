# Linux + Boost 兼容性遗留问题记录

## 当前状态

- **macOS**: 33/33 测试用例全部通过（已实际验证）
- **Linux + Boost**: 代码设计层面已做兼容处理，但**缺少实际 Linux 环境验证**

---

## 已做的 Linux 兼容处理

### 1. 编译与链接配置（CMakeLists.txt）

| 配置项 | 说明 |
|--------|------|
| `-pthread` | 启用 pthread 支持（`std::thread`、`pthread_create` 依赖） |
| `-rdynamic` | 导出符号表，使 `backtrace_symbols_fd()` 能解析函数名 |
| `-ldl` | 链接动态加载库（`dladdr` 依赖） |
| `-DBOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED` | 避免 Boost.Stacktrace 强制要求 `_GNU_SOURCE` |

### 2. 信号处理程序（ExceptionHandler.cpp）

Linux 信号处理程序使用**完全异步信号安全**的实现：

- `backtrace()` / `backtrace_symbols_fd()`: glibc 内置，信号安全
- `write()` / `open()` / `close()`: POSIX 系统调用，信号安全
- `sigaltstack()` + `SA_ONSTACK`: 备用信号栈，确保栈溢出场景仍可捕获
- `localtime_r()` / `gettimeofday()`: POSIX 信号安全函数
- `_Exit()`: 立即终止，不调用 atexit/析构函数

### 3. 平台差异隔离

```
#ifdef __linux__
    syscall(SYS_gettid)     → 获取线程 ID
    localtime_r()           → 线程安全时间转换
#else  // macOS
    pthread_threadid_np()   → 获取线程 ID
    localtime()             → 时间转换（macOS 回退）
#endif
```

### 4. 堆栈解析（Stacktrace.cpp）

- **Boost 路径**: `boost::stacktrace::stacktrace()` 在 Linux 上默认使用 glibc `backtrace`
- **Fallback**: `addr2line -e /proc/self/exe`（Linux 特有，解析文件/行号）

---

## 遗留问题与风险

### 风险 1: Boost.Stacktrace 后端依赖（中等）

Boost.Stacktrace 在 Linux 上有多个可选后端：

| 后端 | 依赖 | 自动选择优先级 |
|------|------|----------------|
| `libbacktrace` | 需额外安装 | 最高 |
| `backtrace` (glibc) | glibc 内置 | 中等 |
| `addr2line` | binutils | 最低 |

**问题**: 如果线上环境没有 `libbacktrace`，Boost 会自动回退到 glibc `backtrace`。但如果 glibc 版本过旧或被精简（如 Alpine/musl），`backtrace()` 可能不可用。

**建议**: 在目标 Linux 机器上运行以下命令确认 Boost 后端：
```bash
ldd ./hello | grep -i boost
# 或检查编译日志中 Boost 实际选择的后端
```

### 风险 2: `-rdynamic` 链接器兼容性（低）

`-rdynamic` 是 GNU ld/gold 的选项。如果线上使用 `lld` 或其他链接器，可能需要替换为 `-Wl,--export-dynamic`。

### 风险 3: `sigaltstack` 对齐要求（已修复）

备用信号栈需要对齐。已修复为 `alignas(16 * 1024) static char altStack[...]`，但不同内核/架构的对齐要求可能有差异。

### 风险 4: 测试用例平台差异

| 测试 | macOS 行为 | Linux 预期行为 | 风险 |
|------|-----------|----------------|------|
| Test 5 (空指针) | 触发 SIGTRAP | 应触发 SIGSEGV | 信号编号不同，但处理程序覆盖两者 |
| Test 6 (除零) | 整数除零不 trap，用 `raise(SIGFPE)` fallback | x86_64 整数除零触发 SIGFPE | `raise(SIGFPE)` fallback 确保一致 |
| Test 19 (栈溢出) | 通过 `sigaltstack` + 备用栈捕获 | 理论上相同 | 需实际验证 `sigaltstack` 行为 |

### 风险 5: `backtrace_symbols_fd` 符号解析质量（低）

即使没有 `-rdynamic`，`backtrace_symbols_fd` 仍能输出地址，但符号名可能显示为 `??`。日志可用性降低，但不影响捕获功能。

---

## 建议的线上验证步骤

1. **编译测试**
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
   make -j4
   ```

2. **运行单元测试**
   ```bash
   ctest --output-on-failure
   ```

3. **运行所有安全测试**
   ```bash
   for t in 1 2 4 7 8 9 10 11 12 13 14 15 16 17 23 24 25 26; do
       ./hello "$t"
   done
   ```

4. **运行 Crash 测试**（逐个执行，每个会终止进程）
   ```bash
   ./hello 5   # SIGSEGV/SIGTRAP
   ./hello 6   # SIGFPE
   ./hello 19  # 栈溢出
   ./hello 20  # SIGABRT
   # ... 其他 crash 测试
   ```

5. **检查日志输出格式**
   - 非信号路径应输出 `symbol at file:line`
   - 信号路径应输出 `backtrace_symbols_fd` 原始格式

6. **如果编译失败**，请提供：
   - `cmake` 输出日志
   - `make` 错误信息
   - Linux 发行版及版本（`cat /etc/os-release`）
   - glibc 版本（`ldd --version`）
   - Boost 版本

---

## 结论

从代码审查角度，**Linux + Boost 路径的设计是兼容的**，所有关键调用都使用了标准 POSIX/glibc API，CMake 也已配置相应的链接标志。

但由于缺少实际 Linux 编译和运行环境，**无法给出 100% 保证**。强烈建议在线上 Linux 环境编译验证一轮，如有问题随时反馈修复。
