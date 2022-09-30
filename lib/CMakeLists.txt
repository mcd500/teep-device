cmake_minimum_required(VERSION 3.1)

set(TEE_PLATFORM pc CACHE STRING "tee platform (optee, keystone, sgx or pc)")

if(${TEE_PLATFORM} STREQUAL optee)
add_definitions(-DPLAT_OPTEE)
elseif(${TEE_PLATFORM} STREQUAL keystone)
add_definitions(-DPLAT_KEYSTONE)
add_definitions(-DKEYSTONE_SDK_DIR=${KEYSTONE_SDK_DIR})
elseif(${TEE_PLATFORM} STREQUAL sgx)
add_definitions(-DPLAT_SGX)
elseif(${TEE_PLATFORM} STREQUAL pc)
add_definitions(-DPLAT_PC)
set(ENABLE_TEST ON)
set(ENABLE_EXAMPLE ON)
enable_testing()
else()
message(FATAL_ERROR "TEE_PLATFORM must be optee, keystone, sgx or pc")
endif()

set (CMAKE_C_STANDARD 99)

project(teep-device)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_subdirectory(log)
add_subdirectory(cbor)
add_subdirectory(cose)
add_subdirectory(suit)
add_subdirectory(teep)
