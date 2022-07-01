

set(T_COSE_SRC_COMMON
    ../../submodule/t_cose/src/t_cose_sign1_sign.c
    ../../submodule/t_cose/src/t_cose_parameters.c
    ../../submodule/t_cose/src/t_cose_sign1_verify.c
    ../../submodule/t_cose/src/t_cose_util.c
)

add_library(t_cose ${T_COSE_SRC_COMMON} t_cose_mbedtls_crypto.c)
target_include_directories(t_cose PUBLIC ../../submodule/t_cose/inc ../../submodule/QCBOR/inc PRIVATE ../../submodule/t_cose/src)
