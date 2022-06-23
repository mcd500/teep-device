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
add_subdirectory(../../submodule/googletest
        "${CMAKE_CURRENT_BINARY_DIR}/googletest" EXCLUDE_FROM_ALL)
add_subdirectory(test)
endif()
