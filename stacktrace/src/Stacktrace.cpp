#include "stacktrace/Stacktrace.h"

#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <array>
#include <regex>

#ifndef STACKTRACE_HAS_BOOST
#include <cxxabi.h>
#endif

#ifdef STACKTRACE_HAS_BOOST
#include <boost/stacktrace.hpp>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <dlfcn.h>
#endif

namespace StacktraceNS {

Stacktrace::Stacktrace(size_t maxFrames) {
    frames_.reserve(maxFrames);
    CaptureImpl();
}

Stacktrace::~Stacktrace() = default;

// Extract file:line from resolved address string
// Supports:
//   macOS atos: "symbol (in lib) (file.cpp:123)"
//   Linux addr2line: "function_name at /path/file.cpp:123"
//   addr2line fallback: "/path/file.cpp:123"
static std::string ExtractFileLine(const std::string& resolved) {
    if (resolved.empty()) return "";

    // Linux addr2line format: last part is " at file:line" or just "file:line"
    size_t atPos = resolved.rfind(" at ");
    if (atPos != std::string::npos) {
        std::string loc = resolved.substr(atPos + 4);
        size_t colonPos = loc.rfind(':');
        if (colonPos != std::string::npos && colonPos > 0) {
            bool isNumber = true;
            for (size_t i = colonPos + 1; i < loc.size(); ++i) {
                if (!std::isdigit(static_cast<unsigned char>(loc[i]))) {
                    isNumber = false;
                    break;
                }
            }
            if (isNumber) return loc;
        }
    }

    // macOS atos format: "(file.cpp:line)" at the very end
    size_t closeParen = resolved.rfind(')');
    if (closeParen != std::string::npos && closeParen == resolved.size() - 1) {
        size_t openParen = resolved.rfind('(', closeParen);
        if (openParen != std::string::npos) {
            std::string inside = resolved.substr(openParen + 1, closeParen - openParen - 1);
            size_t colonPos = inside.rfind(':');
            if (colonPos != std::string::npos && colonPos > 0) {
                bool isNumber = true;
                for (size_t i = colonPos + 1; i < inside.size(); ++i) {
                    if (!std::isdigit(static_cast<unsigned char>(inside[i]))) {
                        isNumber = false;
                        break;
                    }
                }
                if (isNumber) return inside;
            }
        }
    }

    // Raw file:line anywhere in string
    static const std::regex fileLineRe(R"(([^\s:]+:\d+))");
    std::smatch match;
    if (std::regex_search(resolved, match, fileLineRe)) {
        std::string candidate = match[1].str();
        size_t colonPos = candidate.rfind(':');
        if (colonPos != std::string::npos && colonPos > 0) {
            bool isNumber = true;
            for (size_t i = colonPos + 1; i < candidate.size(); ++i) {
                if (!std::isdigit(static_cast<unsigned char>(candidate[i]))) {
                    isNumber = false;
                    break;
                }
            }
            if (isNumber) return candidate;
        }
    }

    return "";
}

std::string Stacktrace::ResolveAddress(void* address) {
    std::string result;

#ifdef __APPLE__
    Dl_info info;
    if (dladdr(address, &info) && info.dli_fname && info.dli_fbase) {
        uintptr_t slide = reinterpret_cast<uintptr_t>(address) - reinterpret_cast<uintptr_t>(info.dli_fbase);

        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
            "atos -o \"%s\" -l 0x%lx 0x%lx 2>/dev/null | head -1",
            info.dli_fname, reinterpret_cast<uintptr_t>(info.dli_fbase), reinterpret_cast<uintptr_t>(address));

        FILE* f = popen(cmd, "r");
        if (f) {
            char line[512] = {0};
            if (fgets(line, sizeof(line), f)) {
                size_t len = strlen(line);
                while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = 0;
                if (len > 0) {
                    result = line;
                }
            }
            pclose(f);
        }

        if (result.empty()) {
            snprintf(cmd, sizeof(cmd),
                " atos -o \"%s\" 0x%lx 2>/dev/null | head -1",
                info.dli_fname, reinterpret_cast<uintptr_t>(address));
            f = popen(cmd, "r");
            if (f) {
                char line[512] = {0};
                if (fgets(line, sizeof(line), f)) {
                    size_t len = strlen(line);
                    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = 0;
                    if (len > 0) {
                        result = line;
                    }
                }
                pclose(f);
            }
        }
    }
#else
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "addr2line -e /proc/self/exe -f 0x%lx 2>/dev/null | head -2",
             reinterpret_cast<uintptr_t>(address));
    FILE* f = popen(cmd, "r");
    if (f) {
        char func[256] = {0}, loc[256] = {0};
        if (fgets(func, sizeof(func), f) && fgets(loc, sizeof(loc), f)) {
            size_t len;
            len = strlen(func); if (len > 0 && func[len-1] == '\n') func[len-1] = 0;
            len = strlen(loc); if (len > 0 && loc[len-1] == '\n') loc[len-1] = 0;
            result = std::string(func) + " at " + loc;
        }
        pclose(f);
    }
#endif

    return result;
}

std::string Stacktrace::ToString() const {
#ifdef STACKTRACE_HAS_BOOST
    std::ostringstream oss;
    const auto& st = boost::stacktrace::stacktrace();
    for (size_t i = 0; i < st.size(); ++i) {
        const void* addr = st[i].address();
        oss << std::setw(2) << i << "# ";

        std::string name = st[i].name();
        if (!name.empty()) {
            oss << Demangle(name.c_str());
        } else {
            oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(addr);
        }

        // Use Boost's source location directly if available
        std::string srcFile = st[i].source_file();
        unsigned int srcLine = st[i].source_line();
        if (!srcFile.empty() && srcLine > 0) {
            oss << " at " << srcFile << ":" << srcLine;
        } else {
            // Fallback to ResolveAddress (atos / addr2line)
            std::string resolved = ResolveAddress(const_cast<void*>(addr));
            std::string loc = ExtractFileLine(resolved);
            if (!loc.empty()) {
                oss << " at " << loc;
            } else if (!srcFile.empty()) {
                oss << " at " << srcFile;
            }
        }

        oss << "\n";
    }
    return oss.str();
#else
    return ToStringWithLineInfo();
#endif
}

std::string Stacktrace::ToStringWithLineInfo() const {
    std::ostringstream oss;
    oss << "Stacktrace (" << frames_.size() << " frames):\n";

    for (size_t i = 0; i < frames_.size(); ++i) {
        const auto& frame = frames_[i];
        oss << std::setw(2) << i << "# ";

        std::string symbolName;
        if (!frame.symbol.empty()) {
            symbolName = Demangle(frame.symbol.c_str());
            oss << symbolName;
        } else if (frame.address) {
            oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(frame.address);
        } else {
            oss << "(unknown)";
        }

        // Try to resolve source location via atos/addr2line
        std::string location;
        if (frame.address) {
            std::string resolved = ResolveAddress(frame.address);
            location = ExtractFileLine(resolved);
        }

        if (!location.empty()) {
            oss << " at " << location;
        } else if (!frame.filename.empty()) {
            oss << " in " << frame.filename;
            if (frame.lineNumber > 0) {
                oss << ":" << std::dec << frame.lineNumber;
            }
        }

        oss << "\n";
    }
    return oss.str();
}

std::string Stacktrace::ToStringSimple() const {
    return ToStringWithLineInfo();
}

std::string Stacktrace::Capture(size_t maxFrames) {
    Stacktrace st(maxFrames);
    return st.ToString();
}

#ifdef STACKTRACE_HAS_BOOST
void Stacktrace::CaptureImpl() {
    // When using Boost, the actual capture happens in ToString()
}
#endif

std::string Demangle(const char* name) {
    if (!name) return "";

    int status = 0;
    std::unique_ptr<char, decltype(&std::free)> demangled(
        abi::__cxa_demangle(name, nullptr, nullptr, &status),
        std::free
    );

    if (status == 0 && demangled) {
        return demangled.get();
    }
    return name;
}

} // namespace StacktraceNS
