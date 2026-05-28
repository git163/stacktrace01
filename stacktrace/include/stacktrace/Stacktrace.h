#pragma once

#include <exception>
#include <string>
#include <vector>

namespace StacktraceNS {

struct Frame {
    void* address = nullptr;
    std::string symbol;
    std::string filename;
    int lineNumber = 0;
};

class Stacktrace {
public:
    static const size_t DEFAULT_MAX_FRAMES = 64;

    explicit Stacktrace(size_t maxFrames = DEFAULT_MAX_FRAMES);
    ~Stacktrace();

    std::string ToString() const;
    std::string ToStringSimple() const;

    const std::vector<Frame>& Frames() const { return frames_; }
    size_t Size() const { return frames_.size(); }
    bool IsEmpty() const { return frames_.empty(); }

    static std::string Capture(size_t maxFrames = DEFAULT_MAX_FRAMES);

    static std::string ResolveAddress(void* address);

private:
    void CaptureImpl();
    std::string ToStringWithLineInfo() const;

    std::vector<Frame> frames_;
};

class StacktraceException : public std::exception {
public:
    explicit StacktraceException(const std::string& message);
    const char* what() const noexcept override;
    const std::string& GetStacktrace() const noexcept;

private:
    std::string message_;
    std::string stacktrace_;
};

std::string Demangle(const char* name);

} // namespace StacktraceNS
