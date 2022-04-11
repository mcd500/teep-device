export PLAT = sgx

ifeq ($(MACHINE), SIM)
LIBRARY_SUFFIX = _sim
else
LIBRARY_SUFFIX =
endif

export TEE_CFLAGS = \
	-I$(TEE_REF_TA_DIR)/include \
	-I$(TAREF_DIR)/build/include

export REE_CFLAGS = \
	-I$(TAREF_DIR)/build/include \
	-DPLAT_SGX

export REE_LDFLAGS = \
	-L$(SGX_LIBRARY_DIR) \
	-L$(TAREF_DIR)/build/lib

export REE_LIBS = \
	-lsgx_urts$(LIBRARY_SUFFIX) \
	-lsgx_uae_service$(LIBRARY_SUFFIX) \
	-lEnclave_u

export ENCLAVE_PEM = $(TOPDIR)/platform/sgx/Enclave_private.pem
export ENCLAVE_CONFIG_FILE = $(TOPDIR)/platform/sgx/config/Enclave.config.xml
