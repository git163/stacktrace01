#include "ExceptionHandler.h"

#include <stacktrace/Stacktrace.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <csignal>
#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>

#if defined(__linux__) || defined(__APPLE__)
#include <execinfo.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#endif

#ifdef __linux__
#include <sys/syscall.h>
#endif

namespace {
    std::mutex logMutex;
    std::atomic<bool> loggingInProgress(false);

    // Re-entrancy guard for signal handlers
    std::atomic<bool> signalSafeBufferInUse(false);

    // Signal-safe path needs a C-string pointer to the log filename.
    const char* gLogFilenamePtr = nullptr;

    // Signal-safe helpers: only use async-signal-safe functions
    void GetSignalNameSafe(int sig, char* buf, size_t bufSize) {
        const char* name;
        switch (sig) {
            case SIGSEGV: name = "SIGSEGV"; break;
            case SIGABRT: name = "SIGABRT"; break;
            case SIGFPE:  name = "SIGFPE"; break;
            case SIGBUS:  name = "SIGBUS"; break;
            case SIGILL:  name = "SIGILL"; break;
            case SIGTRAP: name = "SIGTRAP"; break;
            case SIGPIPE: name = "SIGPIPE"; break;
            case SIGSYS:  name = "SIGSYS"; break;
            default:      name = "UNKNOWN"; break;
        }
        snprintf(buf, bufSize, "%s", name);
    }

    void GetTimestampSafe(char* buf, size_t bufSize) {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        time_t sec = tv.tv_sec;
        struct tm tmBuf;
#ifdef __linux__
        localtime_r(&sec, &tmBuf);
#else
        tmBuf = *localtime(&sec);  // macOS fallback (not signal-safe, but used only for tests)
#endif
        snprintf(buf, bufSize, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                 tmBuf.tm_year + 1900, tmBuf.tm_mon + 1, tmBuf.tm_mday,
                 tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec,
                 static_cast<int>(tv.tv_usec / 1000));
    }

    void GetThreadIdSafe(char* buf, size_t bufSize) {
#ifdef __linux__
        pid_t tid = static_cast<pid_t>(syscall(SYS_gettid));
        snprintf(buf, bufSize, "%d", static_cast<int>(tid));
#else
        // macOS: use pthread_self() converted to a numeric value
        uint64_t tid = 0;
        pthread_threadid_np(nullptr, &tid);
        snprintf(buf, bufSize, "%llu", static_cast<unsigned long long>(tid));
#endif
    }
}

std::string ExceptionHandler::logFilename_;
ExceptionHandler::LogCallback ExceptionHandler::logCallback_;

std::string GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string GetThreadId() {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

std::string GetSignalName(int sig) {
    switch (sig) {
        case SIGSEGV: return "SIGSEGV (Invalid memory access)";
        case SIGABRT: return "SIGABRT (Abort)";
        case SIGFPE:  return "SIGFPE (Floating point exception)";
        case SIGBUS:  return "SIGBUS (Bus error)";
        case SIGILL:  return "SIGILL (Illegal instruction)";
        case SIGTRAP: return "SIGTRAP (Trace/breakpoint trap)";
        case SIGPIPE: return "SIGPIPE (Broken pipe)";
        case SIGSYS:  return "SIGSYS (Bad system call)";
        default:       return "Unknown signal (" + std::to_string(sig) + ")";
    }
}

static std::string BuildLogContent(const std::string& reason, int signalNum,
                                    const std::string& stacktrace) {
    std::ostringstream oss;
    oss << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    oss << "║ STACK TRACE LOG ENTRY                                                    ║" << std::endl;
    oss << "╠════════════════════════════════════════════════════════════════════════╣" << std::endl;
    oss << "║ Timestamp: " << GetTimestamp() << std::endl;
    oss << "║ Thread ID:  " << GetThreadId() << std::endl;
    oss << "║ Reason:     " << reason << std::endl;
    if (signalNum > 0) {
        oss << "║ Signal:     " << GetSignalName(signalNum) << " (" << signalNum << ")" << std::endl;
    }
    oss << "╠════════════════════════════════════════════════════════════════════════╣" << std::endl;
    oss << "║ Stack Trace:                                                             ║" << std::endl;
    oss << "╚════════════════════════════════════════════════════════════════════════╝" << std::endl;
    oss << stacktrace << std::endl;
    oss << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    oss << "║ END OF STACK TRACE                                                      ║" << std::endl;
    oss << "╚════════════════════════════════════════════════════════════════════════╝" << std::endl;
    oss << std::endl;
    return oss.str();
}

// Async-signal-safe stack trace logging.
// Only uses functions that are safe inside a signal handler:
//   backtrace(), backtrace_symbols_fd(), write(), open(), close(), _Exit()
static void SignalSafeLogStacktrace(int sig) {
    if (signalSafeBufferInUse.exchange(true)) {
        const char* msg = "[FATAL] SignalSafeLogStacktrace re-entrant call, aborting\n";
        write(STDERR_FILENO, msg, strlen(msg));
        _Exit(1);
    }

    int fd = open(gLogFilenamePtr ? gLogFilenamePtr : "stacktrace.log",
                  O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        signalSafeBufferInUse = false;
        _Exit(1);
    }

    char sigName[64];
    char timestamp[32];
    char threadId[16];
    GetSignalNameSafe(sig, sigName, sizeof(sigName));
    GetTimestampSafe(timestamp, sizeof(timestamp));
    GetThreadIdSafe(threadId, sizeof(threadId));

    char header[512];
    int n = snprintf(header, sizeof(header),
        "\n=== SIGNAL CAUGHT ===\nSignal: %d (%s)\nTimestamp: %s\nThread: %s\n",
        sig, sigName, timestamp, threadId);
    if (n > 0 && n < (int)sizeof(header)) {
        write(fd, header, n);
    }

    void* buffer[64];
    int size = backtrace(buffer, 64);
    backtrace_symbols_fd(buffer, size, fd);

    const char* footer = "\n=== END OF SIGNAL STACK TRACE ===\n\n";
    write(fd, footer, strlen(footer));
    close(fd);

    signalSafeBufferInUse = false;
}

void ExceptionHandler::Init(const std::string& logFilename) {
    logFilename_ = logFilename;
    gLogFilenamePtr = logFilename_.c_str();

    std::set_terminate([]() {
        LogStacktrace("std::terminate called (uncaught exception)");
        std::_Exit(1);
    });

#if defined(__linux__) || defined(__APPLE__)
    // Set up an alternate signal stack so that stack-overflow crashes
    // (SIGSEGV / SIGBUS) can be handled even when the main stack is exhausted.
    static constexpr size_t ALT_STACK_SIZE = 64 * 1024;
    alignas(16 * 1024) static char altStack[ALT_STACK_SIZE];
    stack_t ss;
    ss.ss_sp = altStack;
    ss.ss_size = ALT_STACK_SIZE;
    ss.ss_flags = 0;
    sigaltstack(&ss, nullptr);

    struct sigaction sa;
    sa.sa_handler = [](int sig) { SignalSafeLogStacktrace(sig); _Exit(1); };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_ONSTACK;

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS,  &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE,  &sa, nullptr);
    sigaction(SIGILL,  &sa, nullptr);
    sigaction(SIGTRAP, &sa, nullptr);
    sigaction(SIGPIPE, &sa, nullptr);
    sigaction(SIGSYS,  &sa, nullptr);
#else
    std::signal(SIGSEGV, [](int sig) { SignalSafeLogStacktrace(sig); std::_Exit(1); });
    std::signal(SIGABRT, [](int sig) { SignalSafeLogStacktrace(sig); std::_Exit(1); });
    std::signal(SIGFPE,  [](int sig) { SignalSafeLogStacktrace(sig); std::_Exit(1); });
    std::signal(SIGBUS,  [](int sig) { SignalSafeLogStacktrace(sig); std::_Exit(1); });
    std::signal(SIGILL,  [](int sig) { SignalSafeLogStacktrace(sig); std::_Exit(1); });
    std::signal(SIGTRAP, [](int sig) { SignalSafeLogStacktrace(sig); std::_Exit(1); });
    std::signal(SIGPIPE, [](int sig) { SignalSafeLogStacktrace(sig); std::_Exit(1); });
    std::signal(SIGSYS,  [](int sig) { SignalSafeLogStacktrace(sig); std::_Exit(1); });
#endif
}

void ExceptionHandler::LogStacktrace(const std::string& reason, int signalNum) {
    LogStacktrace(reason, StacktraceNS::Stacktrace::Capture(), signalNum);
}

void ExceptionHandler::LogStacktrace(const std::string& reason, const std::string& stacktrace, int signalNum) {
    if (loggingInProgress.exchange(true)) {
        std::string msg = "[" + GetTimestamp() + "] [WARN] LogStacktrace re-entrant call detected, ignoring";
        if (logCallback_) {
            logCallback_(LogLevel::WARN, msg);
        } else {
            std::cerr << msg << std::endl;
        }
        return;
    }

    std::lock_guard<std::mutex> lock(logMutex);

    std::string content = BuildLogContent(reason, signalNum, stacktrace);

    std::ofstream ofs(logFilename_, std::ios::app);
    if (ofs) {
        ofs << content;

        if (logCallback_) {
            logCallback_(LogLevel::FATAL, content);
        } else {
            std::cerr << "[" << GetTimestamp() << "] [INFO] Stack trace logged to "
                      << logFilename_ << std::endl;
        }
    } else {
        std::string msg = "Failed to open log file: " + logFilename_;
        if (logCallback_) {
            logCallback_(LogLevel::ERROR, msg + "\n" + content);
        } else {
            std::cerr << "[" << GetTimestamp() << "] [ERROR] " << msg << std::endl;
            std::cerr << content << std::endl;
        }
    }

    loggingInProgress = false;
}

std::string ExceptionHandler::GetLogFilename() {
    return logFilename_;
}

void ExceptionHandler::SetLogCallback(LogCallback callback) {
    logCallback_ = std::move(callback);
}

ExceptionHandler::LogCallback ExceptionHandler::GetLogCallback() {
    return logCallback_;
}
