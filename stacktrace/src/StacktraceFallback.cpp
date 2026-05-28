#if !defined(STACKTRACE_HAS_BOOST) && !defined(STACKTRACE_PLATFORM_LINUX) && !defined(STACKTRACE_PLATFORM_MAC)

#include "stacktrace/Stacktrace.h"

namespace StacktraceNS {

// Fallback implementation - only outputs addresses
// Use addr2line to resolve: addr2line -e <executable> <address>

void Stacktrace::CaptureImpl() {
    // No way to capture stack trace without platform-specific APIs
    // Leave frames_ empty
}

} // namespace StacktraceNS

#endif
