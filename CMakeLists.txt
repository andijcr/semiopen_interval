cmake_minimum_required(VERSION 3.20)

project(ab_take_home)

# generate compile_commands.json for the benefit of clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define a target to set -std=c++20, some warnings, and sanitizers
add_library(common_options INTERFACE)
target_compile_features(common_options INTERFACE cxx_std_20)
target_compile_options(
  common_options
  INTERFACE -Wall
            -Wextra
            -Wshadow
            -Wnon-virtual-dtor
            -pedantic
            -Wunused
            -Woverloaded-virtual
            -Wpedantic
            -Wduplicated-cond
            -Wduplicated-branches
            -Wnull-dereference
            -Wconversion
            -Wsign-conversion)
if(CMAKE_BUILD_TYPE MATCHES "^(Debug|RelWithDebInfo)$")
  # Throw sanitizers in the mix for debug builds
  target_compile_options(common_options INTERFACE -fsanitize=undefined,address)
  target_link_options(common_options INTERFACE -fsanitize=undefined,address)
endif()

# code under test
add_library(semiopen_interval INTERFACE)
target_sources(semiopen_interval INTERFACE semiopen_interval.hpp)
target_link_libraries(semiopen_interval INTERFACE common_options)

# testing section
include(CTest)
# deps
find_package(
  Boost
  COMPONENTS unit_test_framework
  REQUIRED)
# deps
find_package(fmt REQUIRED)

# library for boost test main()
add_library(boost_test_main STATIC boost_test_main.cc)
target_link_libraries(boost_test_main PUBLIC common_options
                                             Boost::unit_test_framework)

# unit test
add_executable(semiopen_interval_test semiopen_interval_test.cc)
target_link_libraries(semiopen_interval_test semiopen_interval boost_test_main fmt::fmt)
add_test(NAME semiopen_interval_test COMMAND semiopen_interval_test)

# fuzz testing if we are under clang
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_BUILD_TYPE MATCHES
                                              "^(Debug|RelWithDebInfo)$")
  message(STATUS "enabling fuzz target")
  add_executable(semiopen_interval_fuzz semiopen_interval_fuzz.cc)
  target_link_libraries(semiopen_interval_fuzz semiopen_interval)
  target_compile_options(semiopen_interval_fuzz PUBLIC -fsanitize=fuzzer)
  target_link_options(semiopen_interval_fuzz PUBLIC -fsanitize=fuzzer)
endif()
