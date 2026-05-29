cmake_minimum_required(VERSION 3.10)
project(stacktrace VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(STACKTRACE_USE_BOOST "Use Boost.Stacktrace if available" ON)
option(STACKTRACE_USE_LIBUNWIND "Use libunwind on macOS" ON)

set(STACKTRACE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(HEADERS
    ${STACKTRACE_INCLUDE_DIR}/stacktrace/Stacktrace.h
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Stacktrace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/StacktraceDemangle.cpp
)

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(STACKTRACE_PLATFORM_LINUX 1)
    message(STATUS "Stacktrace: Detected Linux platform")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(STACKTRACE_PLATFORM_MAC 1)
    message(STATUS "Stacktrace: Detected macOS platform")
endif()

# Boost configuration: set BOOST_ROOT to use Boost.Stacktrace headers
set(BOOST_ROOT "" CACHE PATH "Boost root directory")
if(STACKTRACE_USE_BOOST AND BOOST_ROOT)
    set(STACKTRACE_BOOST_INCLUDE ${BOOST_ROOT}/include)
    if(NOT TARGET Boost::boost)
        add_library(Boost::boost INTERFACE IMPORTED)
    endif()
    set(STACKTRACE_HAS_BOOST 1)
    message(STATUS "Stacktrace: Using Boost headers at ${BOOST_ROOT}/include")
else()
    message(STATUS "Stacktrace: Boost not enabled - using native implementation")
endif()

# Linux native implementation
if(STACKTRACE_PLATFORM_LINUX AND NOT STACKTRACE_HAS_BOOST)
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/StacktraceLinuxNative.cpp)
    message(STATUS "Stacktrace: Using Linux native (glibc backtrace)")
endif()

# macOS native implementation
if(STACKTRACE_PLATFORM_MAC AND NOT STACKTRACE_HAS_BOOST)
    if(STACKTRACE_USE_LIBUNWIND)
        find_library(LIBUNWIND_LIBRARY libunwind HINTS
            /opt/homebrew/lib
            /usr/local/lib
            NO_DEFAULT_PATH
        )
        if(LIBUNWIND_LIBRARY)
            set(STACKTRACE_HAS_LIBUNWIND 1)
            message(STATUS "Stacktrace: libunwind found - using libunwind")
        else()
            message(STATUS "Stacktrace: libunwind not found - using execinfo fallback")
        endif()
    endif()

    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/StacktraceMacNative.cpp)
endif()

# Fallback for unknown platforms
if(NOT STACKTRACE_PLATFORM_LINUX AND NOT STACKTRACE_PLATFORM_MAC)
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/StacktraceFallback.cpp)
    message(WARNING "Stacktrace: Unknown platform - using fallback (addresses only)")
endif()

# Create static library with include directories
add_library(stacktrace_impl STATIC ${SOURCES} ${HEADERS})
target_include_directories(stacktrace_impl PUBLIC
    $<BUILD_INTERFACE:${STACKTRACE_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(STACKTRACE_BOOST_INCLUDE)
    target_include_directories(stacktrace_impl PUBLIC ${STACKTRACE_BOOST_INCLUDE})
endif()

# Set defines
if(STACKTRACE_HAS_BOOST)
    target_compile_definitions(stacktrace_impl PRIVATE STACKTRACE_HAS_BOOST=1)
    target_link_libraries(stacktrace_impl PRIVATE Boost::boost)
endif()
if(STACKTRACE_PLATFORM_LINUX)
    target_compile_definitions(stacktrace_impl PRIVATE STACKTRACE_PLATFORM_LINUX=1)
endif()
if(STACKTRACE_PLATFORM_MAC)
    target_compile_definitions(stacktrace_impl PRIVATE STACKTRACE_PLATFORM_MAC=1)
endif()
if(STACKTRACE_HAS_LIBUNWIND)
    target_compile_definitions(stacktrace_impl PRIVATE STACKTRACE_HAS_LIBUNWIND=1)
    target_link_libraries(stacktrace_impl PRIVATE ${LIBUNWIND_LIBRARY})
endif()

# Linux: link with dl (for dladdr) and pthread (for thread safety)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(stacktrace_impl PRIVATE dl pthread)
endif()

# Create interface library for convenience linking
add_library(stacktrace ALIAS stacktrace_impl)

# Install
install(TARGETS stacktrace_impl
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY ${STACKTRACE_INCLUDE_DIR}/
    DESTINATION include
)

set_target_properties(stacktrace_impl PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
