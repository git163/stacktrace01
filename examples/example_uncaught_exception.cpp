// 示例1：未捕获的异常
// 本示例演示当程序抛出异常且未被捕获时，ExceptionHandler 如何通过 std::set_terminate
// 自动捕获当前堆栈并写入日志文件。

#include "ExceptionHandler.h"
#include <iostream>
#include <stdexcept>

// 模拟一个深层调用链中的函数，最终抛出异常
__attribute__((noinline)) void Level3Function() {
    throw std::runtime_error("模拟业务逻辑出错：无法连接数据库");
}

__attribute__((noinline)) void Level2Function() {
    Level3Function();
}

__attribute__((noinline)) void Level1Function() {
    Level2Function();
}

int main() {
    // 初始化异常处理器，指定日志文件名
    ExceptionHandler::Init("example_uncaught_exception.log");

    std::cout << "正在运行示例：未捕获的异常" << std::endl;
    std::cout << "程序将抛出 std::runtime_error，然后由 terminate handler 捕获并记录堆栈" << std::endl;
    std::cout << "日志文件：example_uncaught_exception.log" << std::endl;
    std::cout << std::endl;

    // 触发深层调用并抛出未捕获的异常
    Level1Function();

    // 这一行不会被执行
    return 0;
}
