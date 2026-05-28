// 示例4：栈溢出 (SIGSEGV)
// 本示例演示当程序发生无限递归导致栈溢出时，ExceptionHandler 如何通过备用信号栈
// (sigaltstack) 捕获 SIGSEGV 并记录堆栈。即使主栈已耗尽，信号处理器仍能正常工作。

#include "ExceptionHandler.h"
#include <iostream>

// 使用 noinline 和大数组分配来快速耗尽栈空间
__attribute__((noinline)) void RecursiveAllocation(int depth) {
    volatile char buffer[65536];  // 每次递归分配 64KB
    buffer[0] = static_cast<char>(depth);
    // 阻止编译器优化
    __asm__ volatile("" : : "m"(buffer) : "memory");
    RecursiveAllocation(depth + 1);
}

int main() {
    ExceptionHandler::Init("example_stack_overflow.log");

    std::cout << "正在运行示例：栈溢出 (Stack Overflow)" << std::endl;
    std::cout << "程序将无限递归耗尽栈空间，触发 SIGSEGV" << std::endl;
    std::cout << "ExceptionHandler 使用备用信号栈处理崩溃并记录堆栈" << std::endl;
    std::cout << "日志文件：example_stack_overflow.log" << std::endl;
    std::cout << std::endl;

    RecursiveAllocation(1);

    return 0;
}
