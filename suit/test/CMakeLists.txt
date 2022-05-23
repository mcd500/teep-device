include(GoogleTest)
add_executable(suit_test suit_test.cpp)
target_link_libraries(suit_test teesuit gtest gtest_main)
gtest_discover_tests(suit_test)
