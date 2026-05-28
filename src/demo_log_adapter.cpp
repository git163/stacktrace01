// ============================================================================
// Demo: Adapt ExceptionHandler to user's online LOG_DEBUG macro
//
// User's online log function is a macro:
//   LOG_DEBUG(LEVEL_INFO, format, msg)
// where LEVEL_INFO is the log level, format is the format string,
// and msg is the content. After execution, the macro automatically
// writes to the log file.
//
// This demo shows how to adapt ExceptionHandler::SetLogCallback
// to bridge with the user's LOG_DEBUG macro.
// ============================================================================

#include <iostream>
#include <stdexcept>
#include "ExceptionHandler.h"

// ------------------------------------------------------------------------
// Simulate user's online LOG_DEBUG macro environment
// ------------------------------------------------------------------------
#define LEVEL_DEBUG 0
#define LEVEL_INFO  1
#define LEVEL_WARN  2
#define LEVEL_ERROR 3
#define LEVEL_FATAL 4

// In real online environment, this macro writes to log file automatically.
// Here we simulate it by printing to stdout.
#define LOG_DEBUG(level, format, ...) \
    do { \
        const char* levelStr = "UNKNOWN"; \
        switch (level) { \
            case LEVEL_DEBUG: levelStr = "DEBUG"; break; \
            case LEVEL_INFO:  levelStr = "INFO";  break; \
            case LEVEL_WARN:  levelStr = "WARN";  break; \
            case LEVEL_ERROR: levelStr = "ERROR"; break; \
            case LEVEL_FATAL: levelStr = "FATAL"; break; \
        } \
        printf("[LOG_DEBUG][%s] " format "\n", levelStr, ##__VA_ARGS__); \
    } while(0)

// ------------------------------------------------------------------------
// Adapter: Bridge ExceptionHandler LogCallback to user's LOG_DEBUG macro
// ------------------------------------------------------------------------
static int MapLogLevel(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return LEVEL_DEBUG;
        case LogLevel::INFO:  return LEVEL_INFO;
        case LogLevel::WARN:  return LEVEL_WARN;
        case LogLevel::ERROR: return LEVEL_ERROR;
        case LogLevel::FATAL: return LEVEL_FATAL;
        default:              return LEVEL_INFO;
    }
}

static void OnlineLogAdapter(LogLevel level, const std::string& message) {
    int onlineLevel = MapLogLevel(level);
    LOG_DEBUG(onlineLevel, "%s", message.c_str());
}

// ------------------------------------------------------------------------
// Test functions to generate stack traces
// ------------------------------------------------------------------------
void DeepFunctionLevel3() {
    throw std::runtime_error("exception from deep call stack");
}

void DeepFunctionLevel2() {
    DeepFunctionLevel3();
}

void DeepFunctionLevel1() {
    DeepFunctionLevel2();
}

// ------------------------------------------------------------------------
// Main demo
// ------------------------------------------------------------------------
int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "Demo: ExceptionHandler + LOG_DEBUG adapter" << std::endl;
    std::cout << "============================================================" << std::endl;

    // 1. Initialize ExceptionHandler
    ExceptionHandler::Init("demo_stacktrace.log");

    // 2. Register the online log adapter
    //    From now on, ExceptionHandler internal logs will be routed
    //    through the user's LOG_DEBUG macro.
    ExceptionHandler::SetLogCallback(OnlineLogAdapter);

    std::cout << "\n--- Test 1: Log stacktrace via LOG_DEBUG adapter ---" << std::endl;
    try {
        DeepFunctionLevel1();
    } catch (const std::exception& e) {
        // LogStacktrace will internally call OnlineLogAdapter -> LOG_DEBUG
        ExceptionHandler::LogStacktrace(std::string("caught exception: ") + e.what());
    }

    std::cout << "\n--- Test 2: Direct log callback demo ---" << std::endl;
    OnlineLogAdapter(LogLevel::INFO, "This is a direct info message through LOG_DEBUG");
    OnlineLogAdapter(LogLevel::ERROR, "This is a direct error message through LOG_DEBUG");

    std::cout << "\n============================================================" << std::endl;
    std::cout << "Demo completed. Check 'demo_stacktrace.log' for stack traces." << std::endl;
    std::cout << "============================================================" << std::endl;

    return 0;
}
