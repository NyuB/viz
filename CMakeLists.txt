cmake_minimum_required(VERSION 3.29 FATAL_ERROR)
project(Viz LANGUAGES CXX)
cmake_policy(SET CMP0135 NEW)

# Custom imports
include(cmake/testing.cmake)

# Compiler options
add_library(compiler_options INTERFACE)
target_compile_features(compiler_options INTERFACE cxx_std_17)
# All warnings, warnings as errors
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(
  compiler_options
  INTERFACE
    "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused;-Werror>>"
    "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3;-WX>>")

# Lib
add_library(Lib lib.cpp)
target_include_directories(Lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Lib PRIVATE compiler_options)

# Main
add_executable(Main main.cpp)
target_include_directories(Main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Main PUBLIC Lib)
target_link_libraries(Main PRIVATE compiler_options)
install(TARGETS Main)

# Test
add_executable(Test test.cpp)
test_with_gtest(Test)
target_link_libraries(Test PUBLIC Lib)
target_link_libraries(Test PRIVATE compiler_options)

# Standalone visualization library not included in main
add_library(Viz SHARED viz.cpp)
target_include_directories(Viz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Viz PRIVATE compiler_options)
