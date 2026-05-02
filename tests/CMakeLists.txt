cmake_minimum_required(VERSION 3.20.0)
project(ema_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.14.0
)
FetchContent_MakeAvailable(Catch2)
include(CTest)
include(Catch)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/../src)

add_executable(ema_tests
    ema/test_ema.cpp
)

target_include_directories(ema_tests PRIVATE
    ${SRC_DIR}   # ema.h
)

target_link_libraries(ema_tests PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(ema_tests)
