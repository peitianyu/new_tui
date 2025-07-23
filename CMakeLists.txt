cmake_minimum_required(VERSION 3.3)
project(all_tests)
SET(CMAKE_C_COMPILER "gcc")

include_directories(src src/test)
file(GLOB_RECURSE ALL_LIBRIRY_SRCS "src/*.c")

message(STATUS "All library sources: ${ALL_LIBRIRY_SRCS}")

add_executable(${PROJECT_NAME} ${ALL_LIBRIRY_SRCS})
target_link_libraries(${PROJECT_NAME} m)