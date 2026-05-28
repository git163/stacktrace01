#pragma once

#include <string>
#include <functional>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class ExceptionHandler {
public:
    using LogCallback = std::function<void(LogLevel level, const std::string& message)>;

    static void Init(const std::string& logFilename = "stacktrace.log");
    static void LogStacktrace(const std::string& reason, int signalNum = 0);
    static void LogStacktrace(const std::string& reason, const std::string& stacktrace, int signalNum = 0);
    static std::string GetLogFilename();

    static void SetLogCallback(LogCallback callback);
    static LogCallback GetLogCallback();

private:
    static std::string logFilename_;
    static LogCallback logCallback_;
};
