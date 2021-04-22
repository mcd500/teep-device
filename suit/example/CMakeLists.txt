add_executable(dump dump.cpp example-util.cpp)
target_link_libraries(dump LINK_PUBLIC teesuit)
add_executable(run-install run-install.cpp example-util.cpp)
target_link_libraries(run-install LINK_PUBLIC teesuit)
