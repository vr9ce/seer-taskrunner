cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_COMPILER "/bin/clang++-18")
add_compile_options(--gcc-install-dir=/usr/lib/gcc/x86_64-linux-gnu/13)
project(TaskRunner CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wpedantic -Wall -W)

add_executable(taskrunner
    src/main.cpp
    src/Runner.cpp
)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)
target_link_libraries(taskrunner PRIVATE nlohmann_json::nlohmann_json)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(taskrunner PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(taskrunner PRIVATE -fsanitize=address,undefined)
endif()
