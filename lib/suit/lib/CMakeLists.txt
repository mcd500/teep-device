
set(TEESUIT_SRC suit.c suit-parser.c suit-context.c teerng.c)
if(${ENABLE_LOG_STDOUT})
    list(APPEND TEESUIT_SRC tee_log_stdout.c)
endif()

add_library(teesuit ${TEESUIT_SRC})
target_include_directories(teesuit PUBLIC ${CMAKE_SOURCE_DIR}/include  ../../../submodule/t_cose/inc ../../../submodule/QCBOR/inc)
