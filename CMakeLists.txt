cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_COMPILER "/bin/clang++-18")
add_compile_options(--gcc-install-dir=/usr/lib/gcc/x86_64-linux-gnu/13)
project(TaskRunner CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wpedantic -Wall -W)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()
include_directories(${CMAKE_SOURCE_DIR}/include)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(taskrunner
    src/main.cpp
    src/Runner.cpp
)
