add_executable(dump dump.cpp example-util.cpp)
target_link_libraries(dump LINK_PUBLIC teesuit teecbor teelog)
add_executable(run-install run-install.cpp example-util.cpp)
target_link_libraries(run-install LINK_PUBLIC teesuit teecbor teelog ${MBEDTLS} ${MBEDCRYPTO} ${MBEDX509})
