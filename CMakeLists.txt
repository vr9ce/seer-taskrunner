cmake_minimum_required(VERSION 3.16)

project(TaskRunner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TASKRUNNER_SANITIZE "Enable sanitizers" OFF)

add_executable(taskrunner
    src/main.cpp
    src/Runner.cpp
)

target_compile_features(taskrunner PRIVATE cxx_std_20)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(taskrunner PRIVATE nlohmann_json::nlohmann_json)

if (TASKRUNNER_SANITIZE AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    target_compile_options(taskrunner PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(taskrunner PRIVATE -fsanitize=address,undefined)
endif()


