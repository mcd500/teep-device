
set(TEESUIT_SRC suit.c)
if(${ENABLE_LOG_STDOUT})
    list(APPEND TEESUIT_SRC tee_log_stdout.c)
endif()

add_library(teesuit ${TEESUIT_SRC})
target_include_directories(teesuit PUBLIC ${CMAKE_SOURCE_DIR}/include)
