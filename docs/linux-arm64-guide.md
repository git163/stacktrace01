# Linux ARM64 部署指南

## 环境信息

- **架构**: aarch64 / arm64
- **操作系统**: Linux
- **编译器**: GCC 7+ 或 Clang 5+

---

## 方式一: 使用项目自带脚本 (推荐)

### 1. 安装依赖

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y build-essential cmake

# 安装 Boost (可选，推荐)
sudo apt install -y libboost-stacktrace-dev

# 或者只安装原生依赖
# glibc 和 libstdc++ 通常已预装
```

### 2. 复制项目到 Linux ARM64

```bash
scp -r pybind_test user@linux-arm64:/path/to/project/
```

### 3. 使用构建脚本

```bash
cd /path/to/project
chmod +x scripts/build-linux-arm64.sh
./scripts/build-linux-arm64.sh
```

### 4. 运行测试

```bash
chmod +x scripts/test-linux-arm64.sh
./scripts/test-linux-arm64.sh
```

---

## 方式二: 手动构建

### 1. 配置

```bash
mkdir build && cd build

# 有 Boost
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-march=armv8-a -pthread" \
    -DCMAKE_EXE_LINKER_FLAGS="-pthread -rdynamic" \
    -DSTACKTRACE_USE_BOOST=ON

# 无 Boost (使用 glibc backtrace)
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-march=armv8-a -pthread" \
    -DCMAKE_EXE_LINKER_FLAGS="-pthread -rdynamic" \
    -DSTACKTRACE_USE_BOOST=OFF
```

### 2. 编译

```bash
cmake --build . -j$(nproc)
```

### 3. 测试

```bash
# 运行所有安全测试
./hello

# 运行单个测试
./hello 1    # 深调用栈
./hello 5    # SIGSEGV
./hello 20   # SIGABRT
```

---

## Linux ARM64 注意事项

### 1. 符号链接 `-rdynamic`

**重要**: Linux 上获取函数名需要 `-rdynamic` 链接标志:

```cmake
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
```

不带此标志，堆栈只会显示地址，不会显示函数名。

### 2. glibc backtrace

Linux (包括 ARM64) 的 `execinfo.h` 提供了 `backtrace()` 系列函数:

```c
#include <execinfo.h>

void* buffer[64];
int n = backtrace(buffer, 64);
char** symbols = backtrace_symbols(buffer, n);
```

### 3. dladdr 符号解析

`dladdr()` 在 Linux ARM64 上完全支持:

```c
#include <dlfcn.h>

Dl_info info;
if (dladdr(address, &info)) {
    printf("symbol: %s\n", info.dli_sname);
    printf("file: %s\n", info.dli_fname);
}
```

### 4. ARM64 架构标志

推荐使用 `-march=armv8-a` 标志，这是 ARM64 的标准架构。

---

## 依赖安装

### Ubuntu/Debian (aarch64)

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    gcc \
    g++ \
    make \
    libboost-stacktrace-dev \
    libboost-system-dev \
    libboost-filesystem-dev
```

### CentOS/RHEL (aarch64)

```bash
sudo yum groupinstall -y "Development Tools"
sudo yum install -y \
    cmake \
    boost-devel
```

### Alpine Linux (aarch64)

```bash
apk add --no-cache \
    build-base \
    cmake \
    boost-dev \
    g++
```

---

## 验证

### 检查日志

```bash
cat stacktrace.log
```

### 期望输出示例

```
╔════════════════════════════════════════════════════════════════════════╗
║ STACK TRACE LOG ENTRY                                                    ║
╠════════════════════════════════════════════════════════════════════════╣
║ Timestamp: 2026-04-29 10:00:00.000
║ Thread ID:  0x0000ffffb4001234
║ Reason:     std::exception: test error
╠════════════════════════════════════════════════════════════════════════╣
║ Stack Trace:                                                             ║
╚════════════════════════════════════════════════════════════════════════╝
 0# stacktrace::Stacktrace::capture() in /path/to/hello
 1# ExceptionHandler::log_stacktrace() in /path/to/hello
 2# main() in /path/to/hello
```

---

## 故障排除

### 问题: 堆栈只有地址，没有函数名

**解决**: 添加 `-rdynamic` 链接标志

```cmake
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
```

### 问题: "Stacktrace not found"

**解决**: 确保 `add_subdirectory(stacktrace)` 在 `add_executable` 之前

### 问题: ARM64 编译错误

**解决**: 使用正确的架构标志

```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a")
```
