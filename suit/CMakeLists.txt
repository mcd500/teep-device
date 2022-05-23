cmake_minimum_required(VERSION 3.1)

option(ENABLE_LOG_STDOUT "add stdout logger" ON)
option(ENABLE_EXAMPLE "build example" ON)
option(ENABLE_TEST "build test" ON)

set (CMAKE_C_STANDARD 99)

project(suit)

add_subdirectory(lib)

if(${ENABLE_EXAMPLE})
add_subdirectory(example)
endif()

if(${ENABLE_TEST})
enable_testing()
add_subdirectory(../submodule/googletest
        "${CMAKE_CURRENT_BINARY_DIR}/googletest" EXCLUDE_FROM_ALL)
add_subdirectory(test)
endif()
