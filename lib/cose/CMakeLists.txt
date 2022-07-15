

set(T_COSE_SRC_COMMON
    ../../submodule/t_cose/src/t_cose_sign1_sign.c
    ../../submodule/t_cose/src/t_cose_parameters.c
    ../../submodule/t_cose/src/t_cose_sign1_verify.c
    ../../submodule/t_cose/src/t_cose_util.c
)

add_library(t_cose ${T_COSE_SRC_COMMON} t_cose_mbedtls_crypto.c)
target_include_directories(t_cose PUBLIC ../../submodule/t_cose/inc ../../submodule/QCBOR/inc PRIVATE ../../submodule/t_cose/src)

set(QCBOR_SRC_COMMON
	../../submodule/QCBOR/src/ieee754.c
	../../submodule/QCBOR/src/qcbor_decode.c
	../../submodule/QCBOR/src/qcbor_encode.c
	../../submodule/QCBOR/src/qcbor_err_to_str.c
	../../submodule/QCBOR/src/UsefulBuf.c
)

add_library(qcbor ${QCBOR_SRC_COMMON})
target_include_directories(qcbor PUBLIC ../../submodule/QCBOR/inc)
target_compile_definitions(qcbor PRIVATE USEFULBUF_DISABLE_ALL_FLOAT)
