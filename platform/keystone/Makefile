BUILD ?= $(CURDIR)/build
SOURCE ?= $(CURDIR)/../..
PLAT = keystone

#export OPTEE_DEV_KIT_DIR ?= $(OPTEE_DIR)/optee_os/out/arm

export KEYSTONE_SDK_DIR ?= $(KEYSTONE_DIR)/sdk

#export HELLO_TA_UUID  ?= 8d82573a-926d-4754-9353-32dc29997f74
#export TEE_AGENT_UUID ?= 68373894-5bb3-403c-9eec-3114a1f5d3fc

TEE_REF_TA_DIR = $(CURDIR)/../../..

APP_CFLAGS = \
	-I$(KEYSTONE_SDK_DIR)/lib/host/include \
	-I$(KEYSTONE_SDK_DIR)/lib/edge/include \
	-I$(KEYSTONE_SDK_DIR)/lib/verifier \
	-I$(TEE_REF_TA_DIR)/include \
	-I$(TEE_REF_TA_DIR)/keyedge/target/include \
	-I$(TEE_REF_TA_DIR)/keyedge/flatcc/include \
	-I$(TEE_REF_TA_DIR)/ref-ta/keystone//Edge \
	-I./ \
	-I$(TEE_REF_TA_DIR)/ref-ta/profiler \
	-DKEYSTONE -DAPP_VERBOSE

APP_LDFLAGS = \
	-L$(KEYSTONE_SDK_DIR)/lib \
	-L$(KEYSTONE_SDK_DIR)/sdk/lib \
	-L$(TEE_REF_TA_DIR)/ref-ta/profiler

APP_LIBS = \
	$(TEE_REF_TA_DIR)/ref-ta/keystone//Edge/App_ocalls.o \
	$(TEE_REF_TA_DIR)/ref-ta/keystone//Edge/Enclave_u.o \
	-lkeystone-host \
	-lkeystone-edge \
	-lkeystone-verifier \
	$(TEE_REF_TA_DIR)/keyedge/flatccrt.a \
	-lhostprofiler

TOOLCHAIN-ree = $(CURDIR)/cross-riscv64.cmake
TOOLCHAIN-tee = $(CURDIR)/cross-riscv64.cmake

REE_CFLAGS = -Wall -Werror -fPIC \
	      -I$(BUILD)/libteep/ree/libwebsockets/include \
		  -I$(SOURCE)/libteep/libwebsockets/include \
	      -I$(SOURCE)/libteep/mbedtls/include $(INCLUDES) \
	      -I$(OPTEE_DIR)/optee_client/public \
		  -I$(TA_DEV_KIT_DIR)
REE_LDFLAGS = \
	-L$(BUILD)/libteep/ree/libwebsockets/lib \
	-L$(BUILD)/libteep/ree/mbedtls/library
CROSS_COMPILE = riscv64-unknown-linux-gnu-
LIBTEEP_SRC = $(SOURCE)/libteep/lib/main.c

libteep-mbedtls-host-DISABLE = y
libteep-libwebsockets-host-DISABLE = y

#libteep-mbedtls-tee-DISABLE = y

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
	-DLWS_MBEDTLS_INCLUDE_DIRS="$(SOURCE)/libteep/mbedtls/include" \
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

libteep-mbedtls-tee-FLAGS = \
	-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-tee) \
	-DCMAKE_INSTALL_PREFIX:PATH=staging \
	-DENABLE_TESTING=0  \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DUSE_SHARED_MBEDTLS_LIBRARY=1

libteep-libwebsockets-tee-FLAGS = \
	-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-tee) \
	-DLWS_ROLE_WS=0 \
	-DLWS_WITH_SSL=1 \
	-DLWS_WITH_MBEDTLS=1 \
	-DMBEDTLS_LIBRARY=$(BUILD)/libteep/tee/mbedtls/library/libmbedtls.a \
	-DMBEDX509_LIBRARY=$(BUILD)/libteep/tee/mbedtls/library/libmbedx509.a \
	-DMBEDCRYPTO_LIBRARY=$(BUILD)/libteep/tee/mbedtls/library/libmbedcrypto.a \
	-DLWS_MBEDTLS_INCLUDE_DIRS="$(SOURCE)/libteep/mbedtls/include" \
	-DLWS_WITH_JOSE=1 \
	-DLWS_WITHOUT_SERVER=1 \
	-DLWS_WITH_STATIC=1 \
	-DLWS_WITH_SHARED=0 \
	-DLWS_STATIC_PIC=1 \
	-DLWS_MAX_SMP=1 \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DLWS_WITHOUT_TESTAPPS=1 \
	-DLWS_WITHOUT_EXTENSIONS=1 \
	-DLWS_WITH_ZLIB=0 \
	-DLWS_WITH_DIR=0 \
	-DLWS__INCLUDE_DIRS=$(INCLUDES) \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS="-Wno-unused-variable"

all: libteep hello teep

include ../common.mk

clean:
	rm -rf $(BUILD)


