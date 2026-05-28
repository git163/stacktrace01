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

# Find Boost if available
find_package(Boost QUIET)
if(Boost_FOUND)
    message(STATUS "Main project: Boost found")
else()
    message(STATUS "Main project: Boost not found, using native stacktrace")
endif()

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
