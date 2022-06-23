add_library(teep libteep.c qcbor-ext.c)
target_include_directories(teep PUBLIC ${CMAKE_SOURCE_DIR}/include)
