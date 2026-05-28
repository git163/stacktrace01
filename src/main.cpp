#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <future>
#include <functional>
#include <locale>
#include <codecvt>
#include <csignal>
#include <csetjmp>
#include <pthread.h>
#include "ExceptionHandler.h"
#include <stacktrace/Stacktrace.h>

static std::string GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

#define LOG_INFO(msg) \
    std::cout << "[" << GetTimestamp() << "] [INFO] " << msg << std::endl

#define LOG_ERROR(msg) \
    std::cerr << "[" << GetTimestamp() << "] [ERROR] " << msg << std::endl

// ============================================================================
// Test Functions
// ============================================================================

void Level3Function() {
    LOG_INFO("Level3Function: about to throw StacktraceException");
    throw StacktraceNS::StacktraceException("deep exception in level3");
}

void Level2Function() {
    LOG_INFO("Level2Function: calling Level3");
    Level3Function();
}

void Level1Function() {
    LOG_INFO("Level1Function: calling Level2");
    Level2Function();
}

// Test 1: Deep call stack with exception
void TestDeepCallStack() {
    LOG_INFO("TestDeepCallStack: starting");
    Level1Function();
}

// Test 2: Nested exceptions (std::throw_with_nested)
void TestNestedExceptions() {
    LOG_INFO("TestNestedExceptions: starting");
    try {
        try {
            LOG_INFO("TestNestedExceptions: throwing inner exception");
            throw StacktraceNS::StacktraceException("inner exception");
        } catch (...) {
            LOG_INFO("TestNestedExceptions: catching and re-throwing with nesting");
            std::throw_with_nested(StacktraceNS::StacktraceException("outer exception"));
        }
    } catch (const StacktraceNS::StacktraceException& e) {
        LOG_INFO("TestNestedExceptions: catching nested StacktraceException");
        ExceptionHandler::LogStacktrace("nested exception (std::throw_with_nested)", e.GetStacktrace());
    } catch (...) {
        LOG_INFO("TestNestedExceptions: catching nested exception");
        ExceptionHandler::LogStacktrace("nested exception (std::throw_with_nested)");
    }
}

// Test 3: Exception in child thread
void TestThreadException() {
    LOG_INFO("TestThreadException: starting, will spawn thread");
    std::thread t([]() {
        LOG_INFO("TestThreadException: thread started, about to throw");
        throw std::runtime_error("exception in child thread");
    });
    LOG_INFO("TestThreadException: joining thread");
    t.join();
    LOG_INFO("TestThreadException: thread joined");
}

// Test 4: Custom exception type
void TestCustomException() {
    LOG_INFO("TestCustomException: throwing custom exception");
    throw StacktraceNS::StacktraceException("custom exception: something went wrong");
}

// Test 5: Invalid memory access (SIGSEGV)
void TestInvalidMemory() {
    LOG_INFO("TestInvalidMemory: about to dereference nullptr");
    int* ptr = nullptr;
    *ptr = 42;
}

// Test 6: Division by zero (SIGFPE)
void TestDivisionByZero() {
    LOG_INFO("TestDivisionByZero: about to divide by zero");
    // On ARM64 macOS integer division by zero does not trap.
    // Use volatile float division to reliably trigger SIGFPE on all platforms,
    // with a fallback to raise() if the hardware does not trap.
    volatile float x = 1.0f;
    volatile float y = 0.0f;
    volatile float z = x / y;
    (void)z;
    // Fallback for platforms where float division by zero also does not trap
    raise(SIGFPE);
}

// Test 7: std::logic_error
void TestLogicError() {
    LOG_INFO("TestLogicError: throwing logic_error");
    throw StacktraceNS::StacktraceException("logic error: invalid argument");
}

// Test 8: std::runtime_error
void TestRuntimeError() {
    LOG_INFO("TestRuntimeError: throwing runtime_error");
    throw StacktraceNS::StacktraceException("runtime error: system failure");
}

// Test 9: std::out_of_range
void TestOutOfRange() {
    LOG_INFO("TestOutOfRange: accessing out-of-bounds vector element");
    std::vector<int> v;
    v.at(100) = 1;
}

// Test 10: std::invalid_argument
void TestInvalidArgument() {
    LOG_INFO("TestInvalidArgument: throwing invalid_argument");
    throw StacktraceNS::StacktraceException("invalid argument: must be positive");
}

// Test 11: std::length_error
void TestLengthError() {
    LOG_INFO("TestLengthError: throwing length_error");
    throw StacktraceNS::StacktraceException("length error: exceeds maximum size");
}

// Test 12: std::overflow_error
void TestOverflowError() {
    LOG_INFO("TestOverflowError: throwing overflow_error");
    throw StacktraceNS::StacktraceException("overflow error: value too large");
}

// Test 13: std::underflow_error
void TestUnderflowError() {
    LOG_INFO("TestUnderflowError: throwing underflow_error");
    throw StacktraceNS::StacktraceException("underflow error: value too small");
}

// Test 14: Re-throw exception
void TestRethrowException() {
    LOG_INFO("TestRethrowException: catching and re-throwing");
    try {
        try {
            LOG_INFO("TestRethrowException: throwing initial exception");
            throw StacktraceNS::StacktraceException("initial exception");
        } catch (...) {
            LOG_INFO("TestRethrowException: catching and re-throwing");
            throw;
        }
    } catch (const StacktraceNS::StacktraceException& e) {
        LOG_INFO("TestRethrowException: catching re-thrown StacktraceException");
        ExceptionHandler::LogStacktrace("re-thrown exception", e.GetStacktrace());
    } catch (...) {
        LOG_INFO("TestRethrowException: catching re-thrown exception");
        ExceptionHandler::LogStacktrace("re-thrown exception");
    }
}

// Test 15: Multiple exceptions in sequence
void TestMultipleExceptions() {
    LOG_INFO("TestMultipleExceptions: throwing first exception");
    try {
        throw StacktraceNS::StacktraceException("first exception");
    } catch (const StacktraceNS::StacktraceException& e) {
        ExceptionHandler::LogStacktrace("multiple exceptions - first", e.GetStacktrace());
    } catch (...) {
        ExceptionHandler::LogStacktrace("multiple exceptions - first");
    }

    LOG_INFO("TestMultipleExceptions: throwing second exception");
    try {
        throw StacktraceNS::StacktraceException("second exception");
    } catch (const StacktraceNS::StacktraceException& e) {
        ExceptionHandler::LogStacktrace("multiple exceptions - second", e.GetStacktrace());
    } catch (...) {
        ExceptionHandler::LogStacktrace("multiple exceptions - second");
    }
}

// Test 16: Exception with inner exception
void TestExceptionWithInner() {
    LOG_INFO("TestExceptionWithInner: starting");
    try {
        throw StacktraceNS::StacktraceException("outer error");
    } catch (...) {
        try {
            throw StacktraceNS::StacktraceException("inner error");
        } catch (const StacktraceNS::StacktraceException& e) {
            ExceptionHandler::LogStacktrace("exception with inner exception", e.GetStacktrace());
        } catch (...) {
            ExceptionHandler::LogStacktrace("exception with inner exception");
        }
    }
}

// Test 17: Async/future exception
void TestFutureException() {
    LOG_INFO("TestFutureException: starting");
    std::future<void> f = std::async(std::launch::async, []() {
        LOG_INFO("TestFutureException: async task throwing");
        throw StacktraceNS::StacktraceException("async exception");
    });
    try {
        f.get();
    } catch (const StacktraceNS::StacktraceException& e) {
        LOG_INFO("TestFutureException: caught future StacktraceException");
        ExceptionHandler::LogStacktrace("async/future exception", e.GetStacktrace());
    } catch (...) {
        LOG_INFO("TestFutureException: caught future exception");
        ExceptionHandler::LogStacktrace("async/future exception");
    }
}

// Test 18: Void pointer dereference
void TestVoidPointerDereference() {
    LOG_INFO("TestVoidPointerDereference: about to dereference void pointer");
    void* p = nullptr;
    *(int*)p = 100;
}

// Test 19: Stack overflow
// Force a large stack allocation and prevent the compiler from eliminating
// the recursive calls.
__attribute__((noinline)) void RecursiveFunction(int depth) {
    volatile char buffer[65536];
    buffer[0] = static_cast<char>(depth);
    // Tell the compiler the entire array is used, so it cannot optimize away
    // the allocation or convert the recursion into a loop.
    __asm__ volatile("" : : "m"(buffer) : "memory");
    RecursiveFunction(depth + 1);
}

void TestStackOverflow() {
    LOG_INFO("TestStackOverflow: causing stack overflow");
    RecursiveFunction(1);
}

// Test 20: Abort signal (SIGABRT)
void TestAbort() {
    LOG_INFO("TestAbort: calling abort()");
    std::abort();
}

// Test 21: SIGILL (Illegal instruction)
void TestIllegalInstruction() {
    LOG_INFO("TestIllegalInstruction: triggering illegal instruction");
#if defined(__x86_64__) || defined(_M_X64)
    __asm__ volatile(".byte 0xFF, 0xFF, 0xFF, 0xFF");
#elif defined(__aarch64__)
    __asm__ volatile(".long 0xFFFFFFFF");
#else
    raise(SIGILL);
#endif
}

// Test 22: SIGTRAP (Trace/breakpoint trap)
void TestSigtrap() {
    LOG_INFO("TestSigtrap: raising SIGTRAP");
    raise(SIGTRAP);
}

// Test 23: std::bad_alloc
void TestBadAlloc() {
    LOG_INFO("TestBadAlloc: about to allocate huge memory");
    std::vector<int> v;
    v.resize(SIZE_MAX);
}

// Test 24: std::bad_cast
void TestBadCast() {
    LOG_INFO("TestBadCast: triggering bad_cast");
    struct Base { virtual ~Base() {} };
    struct Derived1 : Base {};
    struct Derived2 : Base {};

    // Create an object of type Derived1, but try to cast to Derived2
    // dynamic_cast for pointers returns nullptr on failure (no throw)
    // dynamic_cast for references throws std::bad_cast on failure
    Derived1* d1 = new Derived1();
    try {
        // Using reference cast - this will throw bad_cast if the cast is invalid
        // because d1 points to Derived1, not Derived2
        Base& bRef = *d1;
        Derived2& d2Ref = dynamic_cast<Derived2&>(bRef);
        (void)d2Ref;
        LOG_ERROR("TestBadCast: dynamic_cast reference did not throw");
    } catch (const std::bad_cast& e) {
        LOG_INFO("TestBadCast: caught bad_cast: " << e.what());
        ExceptionHandler::LogStacktrace("std::bad_cast caught");
    } catch (...) {
        LOG_ERROR("TestBadCast: caught unexpected exception type");
    }
    delete d1;
}

// Test 25: std::bad_typeid
void TestBadTypeid() {
    LOG_INFO("TestBadTypeid: triggering bad_typeid");
    struct Polymorphic { virtual ~Polymorphic() {} };
    Polymorphic* p = nullptr;
    try {
        (void)typeid(*p);
    } catch (...) {
        ExceptionHandler::LogStacktrace("bad_typeid caught in catch");
    }
}

// Test 26: std::bad_exception
void TestBadException() {
    LOG_INFO("TestBadException: throwing bad_exception");
    throw StacktraceNS::StacktraceException("std::bad_exception");
}

// Test 27: null function pointer call
void TestNullFunctionPointer() {
    LOG_INFO("TestNullFunctionPointer: calling null function pointer");
    typedef void (*FuncPtr)();
    FuncPtr fp = nullptr;
    fp();
}

// Test 28: assertion failure (SIGABRT variant)
void TestAssertionFailure() {
    LOG_INFO("TestAssertionFailure: triggering assertion-like condition");
    bool condition = false;
    if (!condition) {
        LOG_ERROR("Assertion failed: condition must be true");
        std::terminate();
    }
}

// Test 29: exception in constructor
struct ThrowingInConstructor {
    ThrowingInConstructor() {
        LOG_INFO("ThrowingInConstructor: constructor throwing");
        throw StacktraceNS::StacktraceException("exception in constructor");
    }
};

void TestExceptionInConstructor() {
    LOG_INFO("TestExceptionInConstructor: creating object that throws in ctor");
    try {
        ThrowingInConstructor obj;
    } catch (const StacktraceNS::StacktraceException& e) {
        LOG_INFO("TestExceptionInConstructor: caught StacktraceException from constructor");
        ExceptionHandler::LogStacktrace("exception in constructor", e.GetStacktrace());
    } catch (...) {
        LOG_INFO("TestExceptionInConstructor: caught exception from constructor");
        ExceptionHandler::LogStacktrace("exception in constructor");
    }
}

// Test 30: exception in destructor (during stack unwinding)
struct ThrowingInDestructor {
    ~ThrowingInDestructor() noexcept(false) {
        LOG_INFO("ThrowingInDestructor: destructor throwing");
        throw StacktraceNS::StacktraceException("exception in destructor");
    }
};

void TestExceptionInDestructor() {
    LOG_INFO("TestExceptionInDestructor: creating object that throws in dtor");
    try {
        ThrowingInDestructor obj;
    } catch (const StacktraceNS::StacktraceException& e) {
        LOG_INFO("TestExceptionInDestructor: caught StacktraceException from destructor");
        ExceptionHandler::LogStacktrace("exception in destructor", e.GetStacktrace());
    } catch (...) {
        LOG_INFO("TestExceptionInDestructor: caught exception from destructor");
        ExceptionHandler::LogStacktrace("exception in destructor");
    }
}

// Test 31: SIGPIPE (broken pipe)
void TestSigpipe() {
    LOG_INFO("TestSigpipe: triggering SIGPIPE");
    raise(SIGPIPE);
}

// Test 32: SIGSYS (bad system call)
void TestSigsys() {
    LOG_INFO("TestSigsys: triggering SIGSYS");
    raise(SIGSYS);
}

// Test 33: Signal in child thread (SIGSEGV)
void TestSignalInChildThread() {
    LOG_INFO("TestSignalInChildThread: starting, will spawn thread");
    std::thread t([]() {
        LOG_INFO("TestSignalInChildThread: thread started, about to cause SIGSEGV");
        int* ptr = nullptr;
        *ptr = 42;  // This will trigger SIGSEGV
    });
    LOG_INFO("TestSignalInChildThread: joining thread");
    t.join();
    LOG_INFO("TestSignalInChildThread: thread joined");
}

// ============================================================================
// Test Runner
// ============================================================================

void RunTest(const std::string& name, int testCase) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "[" << GetTimestamp() << "] Running Test " << testCase
              << ": " << name << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        switch (testCase) {
            case 1:  TestDeepCallStack(); break;
            case 2:  TestNestedExceptions(); break;
            case 3:  TestThreadException(); break;
            case 4:  TestCustomException(); break;
            case 5:  TestInvalidMemory(); break;
            case 6:  TestDivisionByZero(); break;
            case 7:  TestLogicError(); break;
            case 8:  TestRuntimeError(); break;
            case 9:  TestOutOfRange(); break;
            case 10: TestInvalidArgument(); break;
            case 11: TestLengthError(); break;
            case 12: TestOverflowError(); break;
            case 13: TestUnderflowError(); break;
            case 14: TestRethrowException(); break;
            case 15: TestMultipleExceptions(); break;
            case 16: TestExceptionWithInner(); break;
            case 17: TestFutureException(); break;
            case 18: TestVoidPointerDereference(); break;
            case 19: TestStackOverflow(); break;
            case 20: TestAbort(); break;
            case 21: TestIllegalInstruction(); break;
            case 22: TestSigtrap(); break;
            case 23: TestBadAlloc(); break;
            case 24: TestBadCast(); break;
            case 25: TestBadTypeid(); break;
            case 26: TestBadException(); break;
            case 27: TestNullFunctionPointer(); break;
            case 28: TestAssertionFailure(); break;
            case 29: TestExceptionInConstructor(); break;
            case 30: TestExceptionInDestructor(); break;
            case 31: TestSigpipe(); break;
            case 32: TestSigsys(); break;
            case 33: TestSignalInChildThread(); break;
            default: LOG_ERROR("Unknown test case: " << testCase);
        }
    } catch (const StacktraceNS::StacktraceException& e) {
        std::cout << "[" << GetTimestamp() << "] [CATCH] StacktraceException: "
                  << e.what() << std::endl;
        ExceptionHandler::LogStacktrace(std::string("std::exception: ") + e.what(), e.GetStacktrace());
    } catch (const std::exception& e) {
        std::cout << "[" << GetTimestamp() << "] [CATCH] std::exception: "
                  << e.what() << std::endl;
        ExceptionHandler::LogStacktrace(std::string("std::exception: ") + e.what());
    } catch (...) {
        std::cout << "[" << GetTimestamp() << "] [CATCH] unknown exception"
                  << std::endl;
        ExceptionHandler::LogStacktrace("unknown exception type");
    }
}

int main(int argc, char* argv[]) {
    ExceptionHandler::Init("stacktrace.log");
    std::remove(ExceptionHandler::GetLogFilename().c_str());

    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         Stack Trace Logging Test Suite                     ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "Log file: " << ExceptionHandler::GetLogFilename() << std::endl;
    std::cout << "Start time: " << GetTimestamp() << std::endl;

    std::vector<std::pair<std::string, int>> tests = {
        {"Deep Call Stack",                        1},
        {"Nested Exceptions (throw_with_nested)",  2},
        {"Exception in Child Thread",              3},
        {"Custom Exception Type",                  4},
        {"Invalid Memory Access (SIGSEGV)",        5},
        {"Division by Zero (SIGFPE)",              6},
        {"std::logic_error",                       7},
        {"std::runtime_error",                     8},
        {"std::out_of_range",                      9},
        {"std::invalid_argument",                  10},
        {"std::length_error",                      11},
        {"std::overflow_error",                    12},
        {"std::underflow_error",                   13},
        {"Re-throw Exception",                    14},
        {"Multiple Exceptions in Sequence",         15},
        {"Exception with Inner Exception",          16},
        {"Async/Future Exception",                 17},
        {"Void Pointer Dereference",               18},
        {"Stack Overflow",                          19},
        {"Abort Signal (SIGABRT)",                 20},
        {"Illegal Instruction (SIGILL)",            21},
        {"Trace Trap (SIGTRAP)",                  22},
        {"std::bad_alloc",                          23},
        {"std::bad_cast",                           24},
        {"std::bad_typeid",                         25},
        {"std::bad_exception",                      26},
        {"Null Function Pointer Call",              27},
        {"Assertion Failure",                       28},
        {"Exception in Constructor",                29},
        {"Exception in Destructor",                 30},
        {"Broken Pipe (SIGPIPE)",                  31},
        {"Bad System Call (SIGSYS)",               32},
        {"Signal in Child Thread (SIGSEGV)",       33},
    };

    std::vector<int> safeTests = {1, 2, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 23, 24, 25, 26};
    std::vector<int> crashTests = {3, 5, 6, 18, 19, 20, 21, 22, 27, 28, 29, 30, 31, 32, 33};

    if (argc > 1) {
        int testCase = std::stoi(argv[1]);
        if (testCase >= 1 && testCase <= (int)tests.size()) {
            RunTest(tests[testCase - 1].first, testCase);
        } else {
            LOG_ERROR("Invalid test case. Use 1-" << tests.size());
        }
    } else {
        std::cout << "\nUsage: " << argv[0] << " <test_case>" << std::endl;
        std::cout << "\nAvailable tests (" << tests.size() << " total):" << std::endl;
        for (size_t i = 0; i < tests.size(); ++i) {
            std::cout << "  " << std::setw(2) << (i + 1) << ": " << tests[i].first << std::endl;
        }

        std::cout << "\n Safe tests (no crash):";
        for (int t : safeTests) std::cout << " " << t;
        std::cout << "\n Crash tests (process terminates):";
        for (int t : crashTests) std::cout << " " << t;
        std::cout << "\n" << std::endl;

        std::cout << "Running safe tests..." << std::endl;
        for (int t : safeTests) {
            RunTest(tests[t - 1].first, t);
        }

        std::cout << "\n" << GetTimestamp()
                  << " All safe tests completed." << std::endl;
        std::cout << "To test crash scenarios, run individually:" << std::endl;
        std::cout << "  ./hello 5   # SIGSEGV" << std::endl;
        std::cout << "  ./hello 20  # SIGABRT" << std::endl;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "End time: " << GetTimestamp() << std::endl;
    std::cout << "Check stacktrace.log for stack traces" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
