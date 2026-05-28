#!/bin/bash
# =================================================================
# Linux ARM64 测试脚本
# =================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}=== Linux ARM64 测试脚本 ===${NC}"
echo "Architecture: $(uname -m)"
echo "Kernel: $(uname -r)"
echo ""

# 清理旧的日志
rm -f stacktrace.log

# 测试函数
run_test() {
    local test_num=$1
    local test_name=$2
    echo -e "${YELLOW}测试 $test_num: $test_name${NC}"
    ./hello $test_num 2>&1 || true
    echo ""
}

echo -e "${GREEN}=== 1. 安全测试 ===${NC}"
run_test 1 "深调用栈"
run_test 2 "嵌套异常"
run_test 4 "自定义异常"
run_test 7 "logic_error"
run_test 8 "runtime_error"
run_test 9 "out_of_range"

echo -e "${GREEN}=== 2. 信号处理测试 ===${NC}"
echo -e "${YELLOW}注意: 以下测试会导致进程崩溃${NC}"
echo ""

# SIGSEGV 测试
echo -e "${YELLOW}测试 5: SIGSEGV (空指针访问)${NC}"
./hello 5 2>&1 || echo "进程已终止 (预期行为)"
echo ""

# SIGFPE 测试
echo -e "${YELLOW}测试 6: SIGFPE (除零)${NC}"
./hello 6 2>&1 || echo "进程已终止 (预期行为)"
echo ""

# SIGABRT 测试
echo -e "${YELLOW}测试 20: SIGABRT (abort)${NC}"
./hello 20 2>&1 || echo "进程已终止 (预期行为)"
echo ""

# SIGILL 测试
echo -e "${YELLOW}测试 21: SIGILL (非法指令)${NC}"
./hello 21 2>&1 || echo "进程已终止 (预期行为)"
echo ""

echo -e "${GREEN}=== 3. 验证日志 ===${NC}"
if [ -f stacktrace.log ]; then
    echo "日志文件大小: $(wc -c < stacktrace.log) bytes"
    echo "日志条目数: $(grep -c "STACK TRACE" stacktrace.log)"
    echo ""
    echo -e "${GREEN}日志内容预览:${NC}"
    head -50 stacktrace.log
else
    echo -e "${RED}错误: stacktrace.log 未生成${NC}"
fi

echo ""
echo -e "${GREEN}=== 测试完成 ===${NC}"
