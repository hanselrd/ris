cmake_minimum_required(VERSION 3.2)

project(ris)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

file(GLOB TEST_SOURCES "tests/*.cpp")

add_executable(tests ${TEST_SOURCES})
