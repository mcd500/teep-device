include(GoogleTest)
add_executable(suit_test suit_test.cpp)
target_link_libraries(suit_test teesuit teecbor t_cose teelog gtest_main ${MBEDTLS} ${MBEDCRYPTO} ${MBEDX509} ${QCBOR})
gtest_discover_tests(suit_test)
