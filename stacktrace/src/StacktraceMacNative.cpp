#if defined(STACKTRACE_PLATFORM_MAC) && !defined(STACKTRACE_HAS_BOOST)

#include "stacktrace/Stacktrace.h"

#include <cxxabi.h>
#include <dlfcn.h>
#include <libunwind.h>
#include <memory>
#include <cstdlib>
#include <execinfo.h>

namespace StacktraceNS {

static bool DladdrResolve(void* address, Frame& frame) {
    Dl_info info;
    if (dladdr(address, &info)) {
        frame.address = address;
        frame.filename = info.dli_fname ? info.dli_fname : "";

        if (info.dli_sname) {
            frame.symbol = info.dli_sname;
        }
        return true;
    }
    return false;
}

void Stacktrace::CaptureImpl() {
#if defined(STACKTRACE_HAS_LIBUNWIND)
    unw_cursor_t cursor;
    unw_context_t context;
    unw_word_t ip;

    unw_getcontext(&context);
    if (unw_init_local(&cursor, &context) != 0) {
        return;
    }

    // Skip first frame (this function)
    unw_step(&cursor);

    while (unw_step(&cursor) > 0) {
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        void* address = reinterpret_cast<void*>(ip);

        Frame frame;
        frame.address = address;

        char symbol[256] = {0};
        if (unw_get_proc_name(&cursor, symbol, sizeof(symbol) - 1, nullptr) == 0) {
            frame.symbol = symbol;
        }

        DladdrResolve(address, frame);
        frames_.push_back(frame);

        if (frames_.size() >= DEFAULT_MAX_FRAMES) break;
    }
#else
    // Fallback: use backtrace from execinfo.h (limited on Mac)
    void* buffer[DEFAULT_MAX_FRAMES];
    int size = backtrace(buffer, static_cast<int>(DEFAULT_MAX_FRAMES));

    for (int i = 2; i < size; ++i) {
        Frame frame;
        frame.address = buffer[i];
        DladdrResolve(buffer[i], frame);
        frames_.push_back(frame);
    }
#endif
}

} // namespace StacktraceNS

#endif // STACKTRACE_PLATFORM_MAC && !STACKTRACE_HAS_BOOST
