cmake_minimum_required(VERSION 3.1)

option(ENABLE_LOG_STDOUT "add stdout logger" ON)
option(ENABLE_EXAMPLE "build example" ON)

set (CMAKE_C_STANDARD 99)

project(suit)

add_subdirectory(lib)

if(${ENABLE_EXAMPLE})
add_subdirectory(example)
endif()
