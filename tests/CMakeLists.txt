cmake_minimum_required(VERSION 3.20.0)
project(moving_average_tests CXX)

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

add_executable(moving_average_tests
    moving_average/test_moving_average.cpp
)

target_include_directories(moving_average_tests PRIVATE
    ${SRC_DIR}   # moving_average.h
)

target_link_libraries(moving_average_tests PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(moving_average_tests)
