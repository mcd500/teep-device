cmake_minimum_required(VERSION 3.1)

set(TEE_PLATFORM pc CACHE STRING "tee platform (optee, keystone, sgx or pc)")

option(ENABLE_LOG_STDOUT "add stdout logger" ON)
option(ENABLE_EXAMPLE "build example" ON)
option(ENABLE_TEST "build test" ON)

if(${TEE_PLATFORM} STREQUAL optee)
add_definitions(-DPLAT_OPTEE)
elseif(${TEE_PLATFORM} STREQUAL keystone)
add_definitions(-DPLAT_KEYSTONE)
elseif(${TEE_PLATFORM} STREQUAL sgx)
add_definitions(-DPLAT_SGX)
elseif(${TEE_PLATFORM} STREQUAL pc)
add_definitions(-DPLAT_PC)
else()
message(FATAL_ERROR "TEE_PLATFORM must be optee, keystone, sgx or pc")
endif()

set (CMAKE_C_STANDARD 99)

project(suit)

add_subdirectory(lib)

if(${ENABLE_EXAMPLE} OR ${ENABLE_TEST})
find_library(MBEDTLS NAMES mbedtls)
find_library(MBEDCRYPTO NAMES mbedcrypto)
find_library(MBEDX509 NAMES mbedx509)
endif()

if(${ENABLE_EXAMPLE})
add_subdirectory(example)
endif()

if(${ENABLE_TEST})
enable_testing()
add_subdirectory(../submodule/googletest
        "${CMAKE_CURRENT_BINARY_DIR}/googletest" EXCLUDE_FROM_ALL)
add_subdirectory(test)
endif()
