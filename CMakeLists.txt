cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(rtb_perf_servers LANGUAGES C CXX)


if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default is Release")
    set(CMAKE_BUILD_TYPE "Release")
endif ()

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif ()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cpp)
include(flags)
include(GetGitRevisionDescription)

get_git_head_revision(GIT_REFSPEC GIT_SHA1)
add_definitions(-DBOOST_STACKTRACE_USE_BACKTRACE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(Config++ REQUIRED)

include_directories(SYSTEM ${BOOST_INCLUDE_DIRS})
include_directories(SYSTEM ${CONFIG++_INCLUDE_DIR})

set(libsniper "std" "log" "cache" "xxhash" "net" "strings" "pico" "file" "event" "http" "threads" "udp")

link_directories(/usr/local/lib)

add_subdirectory(libs)
add_subdirectory(src)
