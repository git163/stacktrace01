#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -c, --clean       清理并重新构建"
    echo "  -v, --verbose     显示详细输出"
    echo "  -h, --help        显示帮助信息"
    echo ""
    echo "Examples:"
    echo "  $0                增量编译并运行测试"
    echo "  $0 -c             完整重新构建并运行测试"
    echo "  $0 -c -v          完整重新构建，显示详细测试输出"
}

CLEAN=0
VERBOSE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -v|--verbose)
            VERBOSE="--verbose"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  测试运行脚本${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# 清理并创建 build 目录
if [ "$CLEAN" -eq 1 ]; then
    echo -e "${YELLOW}[1/3] 清理 build 目录...${NC}"
    rm -rf "$BUILD_DIR"
fi

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# 配置（仅在未配置或清理后）
if [ "$CLEAN" -eq 1 ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo -e "${YELLOW}[2/3] 配置项目...${NC}"
    cmake "$PROJECT_DIR"
fi

# 编译
echo -e "${YELLOW}[3/3] 编译项目...${NC}"
cmake --build . --parallel

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  运行单元测试 (CTest)${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# 运行 CTest
ctest --output-on-failure $VERBOSE

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  运行单元测试 (直接执行)${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# 直接运行测试可执行文件
if [ -f "$BUILD_DIR/test_exceptions" ]; then
    echo -e "${YELLOW}直接运行 test_exceptions...${NC}"
    "$BUILD_DIR/test_exceptions"
else
    echo -e "${RED}错误: 未找到 test_exceptions 可执行文件${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  所有测试完成!${NC}"
echo -e "${GREEN}========================================${NC}"
