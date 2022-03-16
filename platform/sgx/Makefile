export BUILD ?= $(CURDIR)/build
export SOURCE ?= $(CURDIR)/../..
export PLAT = sgx

export HELLO_TA_UUID  ?= 8d82573a-926d-4754-9353-32dc29997f74
export TEE_AGENT_UUID ?= 68373894-5bb3-403c-9eec-3114a1f5d3fc

export TAREF_DIR ?= $(CURDIR)/../../..

ifeq ($(MACHINE), SIM)
LIBRARY_SUFFIX = _sim
else
LIBRARY_SUFFIX =
endif

export TEE_CFLAGS = \
	-I$(BUILD)/libteep/tee/QCBOR/inc \
	-I$(TEE_REF_TA_DIR)/include \
	-I$(TAREF_DIR)/build/include

export APP_CFLAGS = $(TEE_CFLAGS) \
	-Wall -g \
	-I$(SOURCE)/include \
	-I$(SOURCE)/libteep/lib \
	-I$(BUILD)/libteep/ree/libwebsockets/include \
	-I$(SOURCE)/submodule/libwebsockets/include \
	-I$(SOURCE)/submodule/mbedtls/include \
	-DPLAT_SGX

export APP_LDFLAGS = \
	-L$(SGX_LIBRARY_DIR) \
	-L$(BUILD)/libteep/ree/mbedtls/library \
	-L$(BUILD)/libteep/ree/libwebsockets/lib \
	-L$(BUILD)/libteep/ree/lib \
	-L$(TAREF_DIR)/build/lib

export APP_LIBS = \
	-lsgx_urts$(LIBRARY_SUFFIX) \
	-lsgx_uae_service$(LIBRARY_SUFFIX) \
	-lEnclave_u -lpthread -lwebsockets -lmbedtls -lmbedx509 -lmbedcrypto -lcap

export TA_CFLAGS = $(TEE_CFLAGS) \
	-g \
	$(SGX_CFLAGS) $(DEBUG_FLAGS) \
	$(addprefix -I, $(INCLUDE_DIRS)) \
	-I$(TAREF_DIR)/gp/include \
	-I$(TAREF_DIR)/api/include \
	-I$(SOURCE)/include \
	-I$(SOURCE)/suit/include \
	-I$(SOURCE)/libteep/lib \
	-DPLAT_SGX

TA_LIBS = \
	-ltee_api \
	-lmbedtls \
	-lgp \
	-lEnclave_t \
	-lteep \
	-lqcbor \
	-lteesuit \
	-lsgx_tstdc -lsgx_tcxx -lsgx_tcrypto -lsgx_tservice$(LIBRARY_SUFFIX)

export TA_LDFLAGS = \
	-L$(TAREF_DIR)/build/lib \
	-L$(BUILD)/libteep/tee/QCBOR \
	-L$(BUILD)/libteep/tee/lib \
	-L$(BUILD)/libteep/tee/libteesuit/lib \
	-Wl,-z,relro,-z,now,-z,noexecstack \
	-Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_DIR) -L$(TAREF_DIR)/build/lib \
	-Wl,--whole-archive -lsgx_trts$(LIBRARY_SUFFIX) -Wl,--no-whole-archive \
	-Wl,--start-group $(TA_LIBS) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 -Wl,--gc-sections   \
	-Wl,--version-script=$(CURDIR)/Enclave.lds

export ENCLAVE_PEM = $(CURDIR)/Enclave_private.pem
export ENCLAVE_CONFIG_FILE = $(CURDIR)/config/Enclave.config.xml

REE_CFLAGS = -Wall -Werror -fPIC \
	      -I$(BUILD)/libteep/ree/libwebsockets/include \
		  -I$(SOURCE)/submodule/libwebsockets/include \
	      -I$(SOURCE)/submodule/mbedtls/include $(INCLUDES) \
	      -I$(TAREF_DIR)/build/include \
	      -I$(TAREF_DIR)/include \
	      -DPLAT_SGX

REE_LDFLAGS = \
	-L$(BUILD)/libteep/ree/libwebsockets/lib \
	-L$(BUILD)/libteep/ree/mbedtls/library \

LIBTEEP_SRC = $(SOURCE)/libteep/lib/libteep.c

libteep-mbedtls-host-DISABLE = y
libteep-libwebsockets-host-DISABLE = y
libteep-QCBOR-host-DISABLE = y

libteep-QCBOR-ree-FLAGS =

libteep-mbedtls-ree-FLAGS = \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DUSE_STATIC_MBEDTLS_LIBRARY=1 \
	-DUSE_SAHTED_MBEDTLS_LIBRARY=0 \

libteep-libwebsockets-ree-FLAGS = \
	-DLWS_WITH_SSL=1 \
	-DLWS_WITH_MBEDTLS=1 \
	-DLWS_WITH_JOSE=1 \
	-DLWS_MBEDTLS_INCLUDE_DIRS=../mbedtls/include \
	-DMBEDTLS_LIBRARY=$(BUILD)/libteep/ree/mbedtls/library/libmbedtls.a \
	-DMBEDX509_LIBRARY=$(BUILD)/libteep/ree/mbedtls/library/libmbedx509.a \
	-DMBEDCRYPTO_LIBRARY=$(BUILD)/libteep/ree/mbedtls/library/libmbedcrypto.a \
	-DLWS_WITH_MINIMAL_EXAMPLES=1 \
	-DLWS_STATIC_PIC=ON \
	-DLWS_WITH_SHARED=OFF \
	-DLWS_WITH_STATIC=1 \
	-DLWS_WITHOUT_SERVER=1 \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_C_FLAGS='-Wno-enum-conversion'

libteep-QCBOR-tee-FLAGS =

libteesuit-tee-FLAGS = \
	-DCMAKE_BUILD_TYPE=DEBUG \
	-DENABLE_LOG_STDOUT=OFF \
	-DENABLE_EXAMPLE=OFF

all: libteep hello-tc teep

include ../common.mk

clean: clean-hello-tc
	rm -rf $(BUILD)

TAM_URL ?= http://tamproto_tam_api_1:8888

test: upload-hello-tc-download
	cd $(BUILD)/teep-agent-ta && \
		../teep-broker-app/teep-broker-app --tamurl $(TAM_URL)/api/tam_cbor
