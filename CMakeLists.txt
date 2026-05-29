cmake_minimum_required(VERSION 3.10)
project(HelloWorld)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable debug symbols and line info for better stack traces
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2")

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DBOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
    # -pthread for pthread support, -rdynamic to export symbols for backtrace
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -rdynamic")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_definitions(-DBOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src/include)

# Add stacktrace subdirectory
add_subdirectory(stacktrace)

# ------------------------------------------------------------------
# 第三方库路径适配：修改此处即可适配不同机器的 Boost 安装路径
# ------------------------------------------------------------------
# 若 Boost 不在系统默认路径，取消下一行注释并填写实际路径：
# set(BOOST_ROOT "/usr/local/boost-1.85")
# ------------------------------------------------------------------
if(BOOST_ROOT)
    include_directories(${BOOST_ROOT}/include)
    if(NOT TARGET Boost::boost)
        add_library(Boost::boost INTERFACE IMPORTED)
    endif()
    set(Boost_FOUND TRUE)
    message(STATUS "Main project: Using Boost headers at ${BOOST_ROOT}/include")
else()
    set(Boost_FOUND FALSE)
    message(STATUS "Main project: Boost root not set, using native stacktrace")
endif()
# ------------------------------------------------------------------

# Main executable
add_executable(hello src/main.cpp src/ExceptionHandler.cpp)
set_source_files_properties(src/main.cpp PROPERTIES COMPILE_FLAGS "-O0")

# Link with stacktrace library
target_link_libraries(hello PRIVATE stacktrace)

# Linux: link with dl for backtrace / dladdr support
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(hello PRIVATE dl)
endif()

# Link Boost if available
if(Boost_FOUND)
    target_link_libraries(hello PRIVATE Boost::boost)
endif()

# Demo: LOG_DEBUG adapter
add_executable(demo_log_adapter src/demo_log_adapter.cpp src/ExceptionHandler.cpp)
target_link_libraries(demo_log_adapter PRIVATE stacktrace)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(demo_log_adapter PRIVATE dl)
endif()
if(Boost_FOUND)
    target_link_libraries(demo_log_adapter PRIVATE Boost::boost)
endif()

# Examples
set(EXAMPLE_SOURCES
    examples/example_uncaught_exception.cpp
    examples/example_sigsegv.cpp
    examples/example_sigfpe.cpp
    examples/example_stack_overflow.cpp
    examples/example_nested_call.cpp
)

# 为示例程序关闭优化，确保多层调用函数不被内联，堆栈信息完整
set_source_files_properties(${EXAMPLE_SOURCES} PROPERTIES COMPILE_FLAGS "-O0")

foreach(src ${EXAMPLE_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src} src/ExceptionHandler.cpp)
    target_link_libraries(${name} PRIVATE stacktrace)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(${name} PRIVATE dl)
    endif()
    if(Boost_FOUND)
        target_link_libraries(${name} PRIVATE Boost::boost)
    endif()
endforeach()

# Unit tests
enable_testing()
add_executable(test_exceptions tests/test_exceptions.cpp src/ExceptionHandler.cpp)
target_link_libraries(test_exceptions PRIVATE stacktrace)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(test_exceptions PRIVATE dl)
endif()
if(Boost_FOUND)
    target_link_libraries(test_exceptions PRIVATE Boost::boost)
endif()
add_test(NAME ExceptionTests COMMAND test_exceptions)
