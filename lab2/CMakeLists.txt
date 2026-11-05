cmake_minimum_required(VERSION 3.10)
project(ProcRunnerLab C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_library(proc_runner STATIC src/proc_runner.c)

add_executable(test_runner test/main.c)
target_link_libraries(test_runner PRIVATE proc_runner)