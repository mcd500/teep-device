export PLAT = keystone

export KEYSTONE_SDK_DIR ?= $(KEYSTONE_DIR)/sdk

export HELLO_TA_UUID  ?= 8d82573a-926d-4754-9353-32dc29997f74
export TEE_AGENT_UUID ?= 68373894-5bb3-403c-9eec-3114a1f5d3fc

export TAREF_DIR ?= $(CURDIR)/../../..

export TEE_CFLAGS = \
	-I$(BUILD)/libteep/tee/QCBOR/inc \
	-I$(TEE_REF_TA_DIR)/include \
	-I$(TAREF_DIR)/build/include

export APP_CFLAGS = $(TEE_CFLAGS) \
	-Wall \
	-I$(SOURCE)/include \
	-I$(KEYSTONE_SDK_DIR)/lib/host/include \
	-I$(KEYSTONE_SDK_DIR)/lib/edge/include \
	-I$(SOURCE)/libteep/lib \
	-I$(BUILD)/libteep/ree/libwebsockets/include \
	-I$(SOURCE)/submodule/libwebsockets/include \
	-I$(SOURCE)/submodule/mbedtls/include \
	-DPLAT_KEYSTONE

export APP_LDFLAGS = \
	-L$(KEYSTONE_SDK_DIR)/lib \
	-L$(KEYSTONE_SDK_DIR)/sdk/lib \
	-L$(TAREF_DIR)/build/lib \
	-L$(BUILD)/libteep/ree/mbedtls/library \
	-Xlinker -rpath-link -Xlinker $(BUILD)/libteep/ree/mbedtls/library \
	-L$(BUILD)/libteep/ree/libwebsockets/lib \
	-L$(BUILD)/libteep/ree/lib

export APP_LIBS = \
	-lkeystone-host -lkeystone-edge -lEnclave_u -lflatccrt -lmbedcrypto -lmbedx509 -lmbedtls -lwebsockets

export TA_CFLAGS = $(TEE_CFLAGS) \
	-Wall -fno-builtin-printf -DEDGE_IGNORE_EGDE_RESULT \
	-I. \
	-I$(SOURCE)/include \
	-I$(KEYSTONE_SDK_DIR)/include/app \
	-I$(KEYSTONE_SDK_DIR)/include/edge \
	-I$(SOURCE)/suit/include \
	-I$(SOURCE)/libteep/lib \
	-DPLAT_KEYSTONE

export TA_LDFLAGS = \
	-L$(TAREF_DIR)/build/lib \
	-L$(KEYSTONE_SDK_DIR)/lib \
	-L$(BUILD)/libteep/tee/libwebsockets/lib \
	-L$(BUILD)/libteep/tee/QCBOR \
	-L$(BUILD)/libteep/tee/lib \
	-L$(BUILD)/libteep/tee/libteesuit/lib \
	-T $(CURDIR)/Enclave.lds

export TA_LIBS = \
	-ltee_api \
	-lEnclave_t \
	-lflatccrt \
	-lteep \
	-lqcbor \
	-lkeystone-eapp \
	-lteesuit

TOOLCHAIN-ree = $(CURDIR)/cross-riscv64.cmake
TOOLCHAIN-tee = $(CURDIR)/cross-riscv64.cmake

REE_CFLAGS = -Wall -Werror -fPIC \
	      -I$(BUILD)/libteep/ree/libwebsockets/include \
		  -I$(SOURCE)/submodule/libwebsockets/include \
	      -I$(SOURCE)/submodule/mbedtls/include $(INCLUDES) \
	      -I$(TAREF_DIR)/build/include \
	      -I$(TAREF_DIR)/include \
	      -DPLAT_KEYSTONE

REE_LDFLAGS = \
	-L$(BUILD)/libteep/ree/libwebsockets/lib \
	-L$(BUILD)/libteep/ree/mbedtls/library \

export CROSS_COMPILE = riscv64-unknown-linux-gnu-
LIBTEEP_SRC = $(SOURCE)/libteep/lib/libteep.c

libteep-mbedtls-host-DISABLE = y
libteep-libwebsockets-host-DISABLE = y
libteep-QCBOR-host-DISABLE = y

libteep-QCBOR-ree-FLAGS = CC=$(CROSS_COMPILE)gcc

libteep-mbedtls-ree-FLAGS = \
	-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-ree) \
	-DCMAKE_INSTALL_PREFIX:PATH=staging \
	-DENABLE_TESTING=0  \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DUSE_SHARED_MBEDTLS_LIBRARY=1

libteep-libwebsockets-ree-FLAGS = \
	-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-ree) \
	-DLWS_ROLE_WS=0 \
	-DLWS_WITH_SSL=1 \
	-DLWS_WITH_MBEDTLS=1 \
	-DMBEDTLS_LIBRARY=$(BUILD)/libteep/ree/mbedtls/library/libmbedtls.so \
	-DMBEDX509_LIBRARY=$(BUILD)/libteep/ree/mbedtls/library/libmbedx509.so \
	-DMBEDCRYPTO_LIBRARY=$(BUILD)/libteep/ree/mbedtls/library/libmbedcrypto.so \
	-DLWS_MBEDTLS_INCLUDE_DIRS="$(SOURCE)/submodule/mbedtls/include" \
	-DLWS_WITH_JOSE=1 \
	-DLWS_WITHOUT_SERVER=1 \
	-DLWS_WITH_STATIC=0 \
	-DLWS_WITH_SHARED=1 \
	-DLWS_STATIC_PIC=1 \
	-DLWS_MAX_SMP=1 \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DLWS_WITHOUT_TESTAPPS=1 \
	-DLWS_WITHOUT_EXTENSIONS=1 \
	-DLWS_WITH_ZLIB=0

libteep-QCBOR-tee-FLAGS = CC=$(CROSS_COMPILE)gcc

libteesuit-tee-FLAGS = \
	-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-tee) \
	-DCMAKE_BUILD_TYPE=DEBUG \
	-DENABLE_LOG_STDOUT=OFF \
	-DENABLE_EXAMPLE=OFF
