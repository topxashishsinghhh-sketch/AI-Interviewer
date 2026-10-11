cmake_minimum_required(VERSION 3.20)
project(ai_interviewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -Wconversion -Wpedantic")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /permissive-")
endif()

option(BUILD_GUI "Build Qt GUI" OFF)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_GUI_TESTS "Build Qt GUI tests (needs Qt6::Test)" OFF)
option(BUILD_WEB "Build the HTTP server + browser front-end" OFF)
option(USE_MONGO "Enable MongoDB backend" OFF)
option(BUILD_TOOLS "Build MongoDB tools" ON)

# Try to find OpenSSL (optional - we have fallback SHA-256)
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    add_definitions(-DHAVE_OPENSSL)
else()
    message(WARNING "OpenSSL not found, using built-in SHA-256 fallback")
endif()

# Try to find nlohmann/json, fmt, gtest via find_package (vcpkg or system)
find_package(nlohmann_json QUIET)
find_package(fmt QUIET)

if(BUILD_TESTS)
    find_package(GTest QUIET)
endif()

if(USE_MONGO)
    # mongocxx/bsoncxx are looked up as CMake config packages. A non-standard
    # install prefix can be supplied via -DMONGO_DRIVER_PREFIX=<path>. It is
    # added to CMAKE_PREFIX_PATH because the mongocxx/bsoncxx config files in
    # turn call find_dependency() for the mongoc/bson C driver packages, and to
    # CMAKE_FIND_ROOT_PATH because the Qt MinGW toolchain file restricts
    # find_package() searches with CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY.
    if(MONGO_DRIVER_PREFIX)
        list(APPEND CMAKE_PREFIX_PATH "${MONGO_DRIVER_PREFIX}")
        list(APPEND CMAKE_FIND_ROOT_PATH "${MONGO_DRIVER_PREFIX}")
    endif()
    find_package(bsoncxx QUIET CONFIG)
    find_package(mongocxx QUIET CONFIG)
    if(bsoncxx_FOUND AND mongocxx_FOUND)
        add_definitions(-DUSE_MONGO)
        message(STATUS "MongoDB backend enabled (mongocxx ${mongocxx_VERSION})")
    else()
        message(WARNING "MongoDB driver not found, disabling USE_MONGO "
                        "(set -DMONGO_DRIVER_PREFIX=<install path> or -DCMAKE_PREFIX_PATH)")
        set(USE_MONGO OFF)
    endif()
endif()

if(BUILD_GUI)
    find_package(Qt6 QUIET COMPONENTS Widgets Concurrent)
    if(Qt6_FOUND)
        add_definitions(-DBUILD_GUI)
        if(BUILD_GUI_TESTS)
            find_package(Qt6 QUIET COMPONENTS Test)
        endif()
    else()
        message(WARNING "Qt6 not found, disabling BUILD_GUI")
        set(BUILD_GUI OFF)
        set(BUILD_GUI_TESTS OFF)
    endif()
endif()

# If dependencies not found, provide instructions
if(NOT nlohmann_json_FOUND)
    message(WARNING "nlohmann_json not found. Using local third_party/json.hpp")
endif()
if(NOT fmt_FOUND)
    message(WARNING "fmt not found. Using iostreams instead")
endif()
if(BUILD_TESTS AND NOT GTest_FOUND)
    message(WARNING "GTest not found. Tests will be skipped")
endif()

# CTest must be enabled in the top-level scope so add_test() in subdirectories
# (tests/, tests/gui) is picked up by `ctest`.
enable_testing()

add_subdirectory(src)
if(BUILD_TOOLS AND USE_MONGO)
    add_subdirectory(tools)
endif()
if(BUILD_GUI AND BUILD_GUI_TESTS)
    add_subdirectory(tests/gui)
endif()
if(BUILD_TESTS AND GTest_FOUND)
    add_subdirectory(tests)
endif()

install(TARGETS ai_interviewer_core ai_interviewer_console
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)