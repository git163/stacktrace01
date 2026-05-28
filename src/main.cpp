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

__attribute__((noinline)) static void NestedInnerLevel3() {
    throw StacktraceNS::StacktraceException("inner exception");
}
__attribute__((noinline)) static void NestedInnerLevel2() { NestedInnerLevel3(); }
__attribute__((noinline)) static void NestedInnerLevel1() { NestedInnerLevel2(); }

// Test 2: Nested exceptions (std::throw_with_nested)
void TestNestedExceptions() {
    LOG_INFO("TestNestedExceptions: starting");
    try {
        try {
            LOG_INFO("TestNestedExceptions: throwing inner exception");
            NestedInnerLevel1();
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

__attribute__((noinline)) static void ThreadLevel3() {
    LOG_INFO("TestThreadException: thread started, about to throw");
    throw std::runtime_error("exception in child thread");
}
__attribute__((noinline)) static void ThreadLevel2() { ThreadLevel3(); }
__attribute__((noinline)) static void ThreadLevel1() { ThreadLevel2(); }

// Test 3: Exception in child thread
void TestThreadException() {
    LOG_INFO("TestThreadException: starting, will spawn thread");
    std::thread t([]() {
        ThreadLevel1();
    });
    LOG_INFO("TestThreadException: joining thread");
    t.join();
    LOG_INFO("TestThreadException: thread joined");
}

__attribute__((noinline)) static void CustomExceptionLevel3() {
    throw StacktraceNS::StacktraceException("custom exception: something went wrong");
}
__attribute__((noinline)) static void CustomExceptionLevel2() { CustomExceptionLevel3(); }
__attribute__((noinline)) static void CustomExceptionLevel1() { CustomExceptionLevel2(); }

// Test 4: Custom exception type
void TestCustomException() {
    LOG_INFO("TestCustomException: throwing custom exception");
    CustomExceptionLevel1();
}


__attribute__((noinline)) static void InvalidMemoryLevel3() {
    int* ptr = nullptr;
    *ptr = 42;
}
__attribute__((noinline)) static void InvalidMemoryLevel2() { InvalidMemoryLevel3(); }
__attribute__((noinline)) static void InvalidMemoryLevel1() { InvalidMemoryLevel2(); }

// Test 5: Invalid memory access (SIGSEGV)
void TestInvalidMemory() {
    LOG_INFO("TestInvalidMemory: about to dereference nullptr");
    InvalidMemoryLevel1();
}

__attribute__((noinline)) static void DivisionByZeroLevel3() {
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
__attribute__((noinline)) static void DivisionByZeroLevel2() { DivisionByZeroLevel3(); }
__attribute__((noinline)) static void DivisionByZeroLevel1() { DivisionByZeroLevel2(); }

// Test 6: Division by zero (SIGFPE)
void TestDivisionByZero() {
    LOG_INFO("TestDivisionByZero: about to divide by zero");
    DivisionByZeroLevel1();
}

__attribute__((noinline)) static void LogicErrorLevel3() {
    throw StacktraceNS::StacktraceException("logic error: invalid argument");
}
__attribute__((noinline)) static void LogicErrorLevel2() { LogicErrorLevel3(); }
__attribute__((noinline)) static void LogicErrorLevel1() { LogicErrorLevel2(); }

// Test 7: std::logic_error
void TestLogicError() {
    LOG_INFO("TestLogicError: throwing logic_error");
    LogicErrorLevel1();
}


__attribute__((noinline)) static void RuntimeErrorLevel3() {
    throw StacktraceNS::StacktraceException("runtime error: system failure");
}
__attribute__((noinline)) static void RuntimeErrorLevel2() { RuntimeErrorLevel3(); }
__attribute__((noinline)) static void RuntimeErrorLevel1() { RuntimeErrorLevel2(); }

// Test 8: std::runtime_error
void TestRuntimeError() {
    LOG_INFO("TestRuntimeError: throwing runtime_error");
    RuntimeErrorLevel1();
}


__attribute__((noinline)) static void OutOfRangeLevel3() {
    std::vector<int> v;
    v.at(100) = 1;
}
__attribute__((noinline)) static void OutOfRangeLevel2() { OutOfRangeLevel3(); }
__attribute__((noinline)) static void OutOfRangeLevel1() { OutOfRangeLevel2(); }

// Test 9: std::out_of_range
void TestOutOfRange() {
    LOG_INFO("TestOutOfRange: accessing out-of-bounds vector element");
    OutOfRangeLevel1();
}

__attribute__((noinline)) static void InvalidArgumentLevel3() {
    throw StacktraceNS::StacktraceException("invalid argument: must be positive");
}
__attribute__((noinline)) static void InvalidArgumentLevel2() { InvalidArgumentLevel3(); }
__attribute__((noinline)) static void InvalidArgumentLevel1() { InvalidArgumentLevel2(); }

// Test 10: std::invalid_argument
void TestInvalidArgument() {
    LOG_INFO("TestInvalidArgument: throwing invalid_argument");
    InvalidArgumentLevel1();
}


__attribute__((noinline)) static void LengthErrorLevel3() {
    throw StacktraceNS::StacktraceException("length error: exceeds maximum size");
}
__attribute__((noinline)) static void LengthErrorLevel2() { LengthErrorLevel3(); }
__attribute__((noinline)) static void LengthErrorLevel1() { LengthErrorLevel2(); }

// Test 11: std::length_error
void TestLengthError() {
    LOG_INFO("TestLengthError: throwing length_error");
    LengthErrorLevel1();
}


__attribute__((noinline)) static void OverflowErrorLevel3() {
    throw StacktraceNS::StacktraceException("overflow error: value too large");
}
__attribute__((noinline)) static void OverflowErrorLevel2() { OverflowErrorLevel3(); }
__attribute__((noinline)) static void OverflowErrorLevel1() { OverflowErrorLevel2(); }

// Test 12: std::overflow_error
void TestOverflowError() {
    LOG_INFO("TestOverflowError: throwing overflow_error");
    OverflowErrorLevel1();
}


__attribute__((noinline)) static void UnderflowErrorLevel3() {
    throw StacktraceNS::StacktraceException("underflow error: value too small");
}
__attribute__((noinline)) static void UnderflowErrorLevel2() { UnderflowErrorLevel3(); }
__attribute__((noinline)) static void UnderflowErrorLevel1() { UnderflowErrorLevel2(); }

// Test 13: std::underflow_error
void TestUnderflowError() {
    LOG_INFO("TestUnderflowError: throwing underflow_error");
    UnderflowErrorLevel1();
}


__attribute__((noinline)) static void RethrowLevel3() {
    throw StacktraceNS::StacktraceException("initial exception");
}
__attribute__((noinline)) static void RethrowLevel2() { RethrowLevel3(); }
__attribute__((noinline)) static void RethrowLevel1() { RethrowLevel2(); }

// Test 14: Re-throw exception
void TestRethrowException() {
    LOG_INFO("TestRethrowException: catching and re-throwing");
    try {
        try {
            LOG_INFO("TestRethrowException: throwing initial exception");
            RethrowLevel1();
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

__attribute__((noinline)) static void MultiFirstLevel3() {
    throw StacktraceNS::StacktraceException("first exception");
}
__attribute__((noinline)) static void MultiFirstLevel2() { MultiFirstLevel3(); }
__attribute__((noinline)) static void MultiFirstLevel1() { MultiFirstLevel2(); }

__attribute__((noinline)) static void MultiSecondLevel3() {
    throw StacktraceNS::StacktraceException("second exception");
}
__attribute__((noinline)) static void MultiSecondLevel2() { MultiSecondLevel3(); }
__attribute__((noinline)) static void MultiSecondLevel1() { MultiSecondLevel2(); }

// Test 15: Multiple exceptions in sequence
void TestMultipleExceptions() {
    LOG_INFO("TestMultipleExceptions: throwing first exception");
    try {
        MultiFirstLevel1();
    } catch (const StacktraceNS::StacktraceException& e) {
        ExceptionHandler::LogStacktrace("multiple exceptions - first", e.GetStacktrace());
    } catch (...) {
        ExceptionHandler::LogStacktrace("multiple exceptions - first");
    }

    LOG_INFO("TestMultipleExceptions: throwing second exception");
    try {
        MultiSecondLevel1();
    } catch (const StacktraceNS::StacktraceException& e) {
        ExceptionHandler::LogStacktrace("multiple exceptions - second", e.GetStacktrace());
    } catch (...) {
        ExceptionHandler::LogStacktrace("multiple exceptions - second");
    }
}

__attribute__((noinline)) static void InnerOuterLevel3() {
    throw StacktraceNS::StacktraceException("outer error");
}
__attribute__((noinline)) static void InnerOuterLevel2() { InnerOuterLevel3(); }
__attribute__((noinline)) static void InnerOuterLevel1() { InnerOuterLevel2(); }

__attribute__((noinline)) static void InnerInnerLevel3() {
    throw StacktraceNS::StacktraceException("inner error");
}
__attribute__((noinline)) static void InnerInnerLevel2() { InnerInnerLevel3(); }
__attribute__((noinline)) static void InnerInnerLevel1() { InnerInnerLevel2(); }

// Test 16: Exception with inner exception
void TestExceptionWithInner() {
    LOG_INFO("TestExceptionWithInner: starting");
    try {
        InnerOuterLevel1();
    } catch (...) {
        try {
            InnerInnerLevel1();
        } catch (const StacktraceNS::StacktraceException& e) {
            ExceptionHandler::LogStacktrace("exception with inner exception", e.GetStacktrace());
        } catch (...) {
            ExceptionHandler::LogStacktrace("exception with inner exception");
        }
    }
}

__attribute__((noinline)) static void FutureLevel3() {
    LOG_INFO("TestFutureException: async task throwing");
    throw StacktraceNS::StacktraceException("async exception");
}
__attribute__((noinline)) static void FutureLevel2() { FutureLevel3(); }
__attribute__((noinline)) static void FutureLevel1() { FutureLevel2(); }

// Test 17: Async/future exception
void TestFutureException() {
    LOG_INFO("TestFutureException: starting");
    std::future<void> f = std::async(std::launch::async, []() {
        FutureLevel1();
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

__attribute__((noinline)) static void VoidPointerLevel3() {
    void* p = nullptr;
    *(int*)p = 100;
}
__attribute__((noinline)) static void VoidPointerLevel2() { VoidPointerLevel3(); }
__attribute__((noinline)) static void VoidPointerLevel1() { VoidPointerLevel2(); }

// Test 18: Void pointer dereference
void TestVoidPointerDereference() {
    LOG_INFO("TestVoidPointerDereference: about to dereference void pointer");
    VoidPointerLevel1();
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

__attribute__((noinline)) static void AbortLevel3() {
    std::abort();
}
__attribute__((noinline)) static void AbortLevel2() { AbortLevel3(); }
__attribute__((noinline)) static void AbortLevel1() { AbortLevel2(); }

// Test 20: Abort signal (SIGABRT)
void TestAbort() {
    LOG_INFO("TestAbort: calling abort()");
    AbortLevel1();
}

__attribute__((noinline)) static void IllegalInstructionLevel3() {
#if defined(__x86_64__) || defined(_M_X64)
    __asm__ volatile(".byte 0xFF, 0xFF, 0xFF, 0xFF");
#elif defined(__aarch64__)
    __asm__ volatile(".long 0xFFFFFFFF");
#else
    raise(SIGILL);
#endif
}
__attribute__((noinline)) static void IllegalInstructionLevel2() { IllegalInstructionLevel3(); }
__attribute__((noinline)) static void IllegalInstructionLevel1() { IllegalInstructionLevel2(); }

// Test 21: SIGILL (Illegal instruction)
void TestIllegalInstruction() {
    LOG_INFO("TestIllegalInstruction: triggering illegal instruction");
    IllegalInstructionLevel1();
}

__attribute__((noinline)) static void SigtrapLevel3() {
    raise(SIGTRAP);
}
__attribute__((noinline)) static void SigtrapLevel2() { SigtrapLevel3(); }
__attribute__((noinline)) static void SigtrapLevel1() { SigtrapLevel2(); }

// Test 22: SIGTRAP (Trace/breakpoint trap)
void TestSigtrap() {
    LOG_INFO("TestSigtrap: raising SIGTRAP");
    SigtrapLevel1();
}

__attribute__((noinline)) static void BadAllocLevel3() {
    std::vector<int> v;
    v.resize(SIZE_MAX);
}
__attribute__((noinline)) static void BadAllocLevel2() { BadAllocLevel3(); }
__attribute__((noinline)) static void BadAllocLevel1() { BadAllocLevel2(); }

// Test 23: std::bad_alloc
void TestBadAlloc() {
    LOG_INFO("TestBadAlloc: about to allocate huge memory");
    BadAllocLevel1();
}

__attribute__((noinline)) static void BadCastLevel3() {
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
__attribute__((noinline)) static void BadCastLevel2() { BadCastLevel3(); }
__attribute__((noinline)) static void BadCastLevel1() { BadCastLevel2(); }

// Test 24: std::bad_cast
void TestBadCast() {
    LOG_INFO("TestBadCast: triggering bad_cast");
    BadCastLevel1();
}

__attribute__((noinline)) static void BadTypeidLevel3() {
    struct Polymorphic { virtual ~Polymorphic() {} };
    Polymorphic* p = nullptr;
    try {
        (void)typeid(*p);
    } catch (...) {
        ExceptionHandler::LogStacktrace("bad_typeid caught in catch");
    }
}
__attribute__((noinline)) static void BadTypeidLevel2() { BadTypeidLevel3(); }
__attribute__((noinline)) static void BadTypeidLevel1() { BadTypeidLevel2(); }

// Test 25: std::bad_typeid
void TestBadTypeid() {
    LOG_INFO("TestBadTypeid: triggering bad_typeid");
    BadTypeidLevel1();
}

__attribute__((noinline)) static void BadExceptionLevel3() {
    throw StacktraceNS::StacktraceException("std::bad_exception");
}
__attribute__((noinline)) static void BadExceptionLevel2() { BadExceptionLevel3(); }
__attribute__((noinline)) static void BadExceptionLevel1() { BadExceptionLevel2(); }

// Test 26: std::bad_exception
void TestBadException() {
    LOG_INFO("TestBadException: throwing bad_exception");
    BadExceptionLevel1();
}


__attribute__((noinline)) static void NullFunctionPointerLevel3() {
    typedef void (*FuncPtr)();
    FuncPtr fp = nullptr;
    fp();
}
__attribute__((noinline)) static void NullFunctionPointerLevel2() { NullFunctionPointerLevel3(); }
__attribute__((noinline)) static void NullFunctionPointerLevel1() { NullFunctionPointerLevel2(); }

// Test 27: null function pointer call
void TestNullFunctionPointer() {
    LOG_INFO("TestNullFunctionPointer: calling null function pointer");
    NullFunctionPointerLevel1();
}

__attribute__((noinline)) static void AssertionFailureLevel3() {
    bool condition = false;
    if (!condition) {
        LOG_ERROR("Assertion failed: condition must be true");
        std::terminate();
    }
}
__attribute__((noinline)) static void AssertionFailureLevel2() { AssertionFailureLevel3(); }
__attribute__((noinline)) static void AssertionFailureLevel1() { AssertionFailureLevel2(); }

// Test 28: assertion failure (SIGABRT variant)
void TestAssertionFailure() {
    LOG_INFO("TestAssertionFailure: triggering assertion-like condition");
    AssertionFailureLevel1();
}

// Test 29: exception in constructor
struct ThrowingCtorLevel3 {
    __attribute__((noinline)) ThrowingCtorLevel3() {
        LOG_INFO("ThrowingCtorLevel3: constructor throwing");
        throw StacktraceNS::StacktraceException("exception in constructor");
    }
};
struct ThrowingCtorLevel2 {
    __attribute__((noinline)) ThrowingCtorLevel2() : m_obj() {}
    ThrowingCtorLevel3 m_obj;
};
struct ThrowingCtorLevel1 {
    __attribute__((noinline)) ThrowingCtorLevel1() : m_obj() {}
    ThrowingCtorLevel2 m_obj;
};

void TestExceptionInConstructor() {
    LOG_INFO("TestExceptionInConstructor: creating object that throws in ctor");
    try {
        ThrowingCtorLevel1 obj;
    } catch (const StacktraceNS::StacktraceException& e) {
        LOG_INFO("TestExceptionInConstructor: caught StacktraceException from constructor");
        ExceptionHandler::LogStacktrace("exception in constructor", e.GetStacktrace());
    } catch (...) {
        LOG_INFO("TestExceptionInConstructor: caught exception from constructor");
        ExceptionHandler::LogStacktrace("exception in constructor");
    }
}

// Test 30: exception in destructor (during stack unwinding)
struct ThrowingDtorLevel3 {
    __attribute__((noinline)) ~ThrowingDtorLevel3() noexcept(false) {
        LOG_INFO("ThrowingDtorLevel3: destructor throwing");
        throw StacktraceNS::StacktraceException("exception in destructor");
    }
};
struct ThrowingDtorLevel2 {
    __attribute__((noinline)) ~ThrowingDtorLevel2() noexcept(false) {}
    ThrowingDtorLevel3 m_obj;
};
struct ThrowingDtorLevel1 {
    __attribute__((noinline)) ~ThrowingDtorLevel1() noexcept(false) {}
    ThrowingDtorLevel2 m_obj;
};

void TestExceptionInDestructor() {
    LOG_INFO("TestExceptionInDestructor: creating object that throws in dtor");
    try {
        ThrowingDtorLevel1 obj;
    } catch (const StacktraceNS::StacktraceException& e) {
        LOG_INFO("TestExceptionInDestructor: caught StacktraceException from destructor");
        ExceptionHandler::LogStacktrace("exception in destructor", e.GetStacktrace());
    } catch (...) {
        LOG_INFO("TestExceptionInDestructor: caught exception from destructor");
        ExceptionHandler::LogStacktrace("exception in destructor");
    }
}

__attribute__((noinline)) static void SigpipeLevel3() {
    raise(SIGPIPE);
}
__attribute__((noinline)) static void SigpipeLevel2() { SigpipeLevel3(); }
__attribute__((noinline)) static void SigpipeLevel1() { SigpipeLevel2(); }

// Test 31: SIGPIPE (broken pipe)
void TestSigpipe() {
    LOG_INFO("TestSigpipe: triggering SIGPIPE");
    SigpipeLevel1();
}

__attribute__((noinline)) static void SigsysLevel3() {
    raise(SIGSYS);
}
__attribute__((noinline)) static void SigsysLevel2() { SigsysLevel3(); }
__attribute__((noinline)) static void SigsysLevel1() { SigsysLevel2(); }

// Test 32: SIGSYS (bad system call)
void TestSigsys() {
    LOG_INFO("TestSigsys: triggering SIGSYS");
    SigsysLevel1();
}

__attribute__((noinline)) static void SignalInChildThreadLevel3() {
    LOG_INFO("TestSignalInChildThread: thread started, about to cause SIGSEGV");
    int* ptr = nullptr;
    *ptr = 42;  // This will trigger SIGSEGV
}
__attribute__((noinline)) static void SignalInChildThreadLevel2() { SignalInChildThreadLevel3(); }
__attribute__((noinline)) static void SignalInChildThreadLevel1() { SignalInChildThreadLevel2(); }

// Test 33: Signal in child thread (SIGSEGV)
void TestSignalInChildThread() {
    LOG_INFO("TestSignalInChildThread: starting, will spawn thread");
    std::thread t([]() {
        SignalInChildThreadLevel1();
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
