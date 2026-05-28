#if defined(STACKTRACE_PLATFORM_LINUX) && !defined(STACKTRACE_HAS_BOOST)

#include "stacktrace/Stacktrace.h"

#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <memory>
#include <cstdlib>
#include <cstring>

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
#ifdef STACKTRACE_HAS_LIBBACKTRACE
    // If libbacktrace is available, use it (better symbol resolution)
#else
    // Use glibc backtrace
    void* buffer[DEFAULT_MAX_FRAMES];
    int size = backtrace(buffer, static_cast<int>(DEFAULT_MAX_FRAMES));

    // Skip first 2 frames (CaptureImpl and constructor)
    for (int i = 2; i < size; ++i) {
        Frame frame;
        frame.address = buffer[i];
        DladdrResolve(buffer[i], frame);
        frames_.push_back(frame);
    }
#endif
}

} // namespace StacktraceNS

#endif // STACKTRACE_PLATFORM_LINUX && !STACKTRACE_HAS_BOOST
