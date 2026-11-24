cmake_minimum_required(VERSION 3.10)
project(OS_Lab3 C)

set(CMAKE_C_STANDARD 11)

if(UNIX)
    link_libraries(pthread rt)
endif()

add_executable(lab3 src/main.c src/platform_os.c)
target_include_directories(lab3 PRIVATE src)