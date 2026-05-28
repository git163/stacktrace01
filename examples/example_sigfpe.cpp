// 示例3：浮点异常 (SIGFPE)
// 本示例演示当程序执行非法数学运算（如除以零）时，ExceptionHandler 如何捕获
// SIGFPE 信号并记录堆栈。

#include "ExceptionHandler.h"
#include <iostream>
#include <csignal>

__attribute__((noinline)) void DangerousDivide() {
    // 使用 volatile 防止编译器优化掉除法运算
    volatile int numerator = 100;
    volatile int denominator = 0;
    volatile int result = numerator / denominator;  // 某些平台触发 SIGFPE
    (void)result;
    // 在 ARM64 macOS 等平台上整数除零不会触发信号，使用 raise 确保
    raise(SIGFPE);
}

__attribute__((noinline)) void CalculateStatistics() {
    DangerousDivide();
}

__attribute__((noinline)) void RunAlgorithm() {
    CalculateStatistics();
}

int main() {
    ExceptionHandler::Init("example_sigfpe.log");

    std::cout << "正在运行示例：浮点异常 (SIGFPE)" << std::endl;
    std::cout << "程序将执行除以零操作，触发 SIGFPE，信号处理器会记录堆栈" << std::endl;
    std::cout << "日志文件：example_sigfpe.log" << std::endl;
    std::cout << std::endl;

    RunAlgorithm();

    return 0;
}
