// 示例5：深层嵌套调用链中的异常
// 本示例演示当异常在深层嵌套调用中抛出时，StacktraceException 如何保存抛出点的
// 完整调用链，并在 catch 后打印到日志。

#include "ExceptionHandler.h"
#include <stacktrace/Stacktrace.h>
#include <iostream>
#include <stdexcept>

// 模拟一个复杂系统的多层调用：网络层 -> 业务层 -> 数据层
__attribute__((noinline)) void DatabaseQuery() {
    throw StacktraceNS::StacktraceException("数据库查询失败：连接超时");
}

__attribute__((noinline)) void BusinessLogic() {
    DatabaseQuery();
}

__attribute__((noinline)) void NetworkHandler() {
    BusinessLogic();
}

__attribute__((noinline)) void ProcessRequest() {
    NetworkHandler();
}

int main() {
    ExceptionHandler::Init("example_nested_call.log");

    std::cout << "正在运行示例：深层嵌套调用链中的异常" << std::endl;
    std::cout << "程序将在深层调用中抛出 StacktraceException，catch 后打印完整堆栈" << std::endl;
    std::cout << "日志文件：example_nested_call.log" << std::endl;
    std::cout << std::endl;

    try {
        ProcessRequest();
    } catch (const StacktraceNS::StacktraceException& e) {
        std::cerr << "捕获到异常: " << e.what() << std::endl;
        std::cerr << "抛出点堆栈:" << std::endl;
        std::cerr << e.GetStacktrace() << std::endl;

        // 同时写入日志文件
        ExceptionHandler::LogStacktrace(
            std::string("caught StacktraceException: ") + e.what(),
            e.GetStacktrace()
        );
    }

    std::cout << std::endl;
    std::cout << "程序正常退出，请查看 example_nested_call.log 中的堆栈日志" << std::endl;
    return 0;
}
