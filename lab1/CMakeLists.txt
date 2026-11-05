cmake_minimum_required(VERSION 3.10)
project(Lab1_Hello C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_executable(hello_app 
    src/main.c 
    src/hello.c
)