#!/bin/bash
# =================================================================
# Linux x86/x86_64 构建脚本
# =================================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Linux x86/x86_64 Stacktrace Build Script ===${NC}"

# 检测架构
ARCH=$(uname -m)
echo "Detected architecture: $ARCH"

if [ "$ARCH" != "x86_64" ] && [ "$ARCH" != "amd64" ] && [ "$ARCH" != "i386" ] && [ "$ARCH" != "i686" ]; then
    echo -e "${YELLOW}Warning: This script is designed for x86/x86_64. Current: $ARCH${NC}"
fi

# 检测 Linux
if [ "$(uname -s)" != "Linux" ]; then
    echo -e "${RED}Error: This script must be run on Linux${NC}"
    exit 1
fi

# 构建目录
BUILD_DIR="build-linux"
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

# 配置 CMake
echo -e "${GREEN}Configuring CMake...${NC}"

# 检查是否有 Boost（-f 检测文件，-d 检测目录）
if [ -f "/usr/include/boost/stacktrace.hpp" ] || [ -f "/usr/local/include/boost/stacktrace.hpp" ]; then
    echo "Boost detected, using Boost.Stacktrace"
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS_RELEASE="-march=x86-64" \
        -DSTACKTRACE_USE_BOOST=ON
else
    echo "Boost not found, using native implementation"
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS_RELEASE="-march=x86-64" \
        -DSTACKTRACE_USE_BOOST=OFF
fi

# 编译
echo -e "${GREEN}Building...${NC}"
cmake --build . -j$(nproc)

echo -e "${GREEN}=== Build Complete ===${NC}"
echo "Executable: $BUILD_DIR/hello"

# 运行测试
echo ""
echo -e "${GREEN}Running tests...${NC}"
echo ""

# 运行安全测试
./hello

# 测试信号处理
echo ""
echo -e "${YELLOW}Testing SIGSEGV (will crash)...${NC}"
./hello 5 || true

echo ""
echo -e "${GREEN}=== All Tests Complete ===${NC}"
echo ""
echo "Check stacktrace.log for stack traces"
