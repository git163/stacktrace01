// 示例2：段错误 (SIGSEGV)
// 本示例演示当程序访问非法内存地址时，ExceptionHandler 如何捕获 SIGSEGV 信号
// 并将信号发生时的堆栈写入日志文件。

#include "ExceptionHandler.h"
#include <iostream>

// 模拟多层调用后的空指针解引用
__attribute__((noinline)) void AccessNullPointer() {
    int* ptr = nullptr;
    *ptr = 42;  // 触发 SIGSEGV
}

__attribute__((noinline)) void ProcessData() {
    AccessNullPointer();
}

__attribute__((noinline)) void HandleRequest() {
    ProcessData();
}

int main() {
    ExceptionHandler::Init("example_sigsegv.log");

    std::cout << "正在运行示例：段错误 (SIGSEGV)" << std::endl;
    std::cout << "程序将解引用空指针，触发 SIGSEGV，信号处理器会记录堆栈" << std::endl;
    std::cout << "日志文件：example_sigsegv.log" << std::endl;
    std::cout << std::endl;

    HandleRequest();

    return 0;
}
