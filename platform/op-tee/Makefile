BUILD ?= $(CURDIR)/build
SOURCE ?= $(CURDIR)/../..
PLAT = optee

export OPTEE_DEV_KIT_DIR ?= $(OPTEE_DIR)/optee_os/out/arm

export HELLO_TA_UUID  ?= 8d82573a-926d-4754-9353-32dc29997f74
export TEE_AGENT_UUID ?= 68373894-5bb3-403c-9eec-3114a1f5d3fc

export APP_CFLAGS ?= \
	-I$(TA_DEV_KIT_DIR)/host_include \
	-I$(SOURCE)/libteep/lib \
	-I$(SOURCE)/libteep/libwebsockets/include \
	-I$(SOURCE)/libteep/mbedtls/include \
	-I$(OPTEE_DIR)/optee_client/public \
	-I$(TEE_REF_TA_DIR)/include \
	-I$(BUILD)/libteep/ree/mbedtls/include \
	-I$(BUILD)/libteep/ree/libwebsockets/include

export APP_LDFLAGS ?= \
	-L$(OPTEE_DIR)/out-br/target/usr/lib \
	-L$(BUILD)/libteep/ree/lib \
	-L$(BUILD)/libteep/ree/mbedtls/library \
	-L$(BUILD)/libteep/ree/libwebsockets/lib

export APP_LIBS ?= \
	-lteep \
	-lteec \
	-lwebsockets \
	-lmbedtls \
	-lmbedcrypto \
	-lmbedx509

TOOLCHAIN-ree = $(CURDIR)/cross-aarch64.cmake
TOOLCHAIN-tee = $(CURDIR)/cross-aarch64-tee.cmake

REE_CFLAGS ?= -Wall -Werror -fPIC \
	      -I$(BUILD)/libteep/ree/libwebsockets/include \
		  -I$(SOURCE)/libteep/libwebsockets/include \
	      -I$(SOURCE)/libteep/mbedtls/include $(INCLUDES) \
	      -I$(OPTEE_DIR)/optee_client/public \
		  -I$(TA_DEV_KIT_DIR)/export-ta_arm64/include
REE_LDFLAGS ?= \
	-L$(BUILD)/libteep/ree/libwebsockets/lib \
	-L$(BUILD)/libteep/ree/mbedtls/library
export CROSS_COMPILE = aarch64-linux-gnu-
LIBTEEP_SRC = $(SOURCE)/libteep/lib/main.c

libteep-mbedtls-host-DISABLE = y
libteep-libwebsockets-host-DISABLE = y

libteep-mbedtls-tee-DISABLE = y

libteep-mbedtls-ree-FLAGS = \
	-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-ree) \
	-DCMAKE_INSTALL_PREFIX:PATH=staging \
	-DENABLE_TESTING=0  \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DUSE_SHARED_MBEDTLS_LIBRARY=1

libteep-libwebsockets-ree-FLAGS = \
	-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-ree) \
	CMAKE_SYSTEM_PROCESSOR=aarch64 \
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

libteep-libwebsockets-tee-FLAGS = \
	-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-tee) \
	CMAKE_SYSTEM_PROCESSOR=aarch64 \
	-DLWS_ROLE_WS=0 \
	-DLWS_WITH_SSL=1 \
	-DLWS_WITH_MBEDTLS=1 \
	-DLWS_MBEDTLS_LIBRARIES=libmbedtls.a \
	-DLWS_MBEDTLS_INCLUDE_DIRS="$(OPTEE_DEV_KIT_DIR)/export-ta_arm64/include;$(OPTEE_DIR)/optee_os/lib/libmbedtls/mbedtls/include;$(OPTEE_DIR)/optee_os/lib/libmbedtls/include" \
	-DLWS_WITH_JOSE=1 \
	-DLWS_WITHOUT_SERVER=1 \
	-DLWS_WITH_STATIC=1 \
	-DLWS_WITH_SHARED=0 \
	-DLWS_STATIC_PIC=1 \
	-DLWS_MAX_SMP=1 \
	-DLWS_PLAT_OPTEE=1 \
	-DLWS_OPTEE_DIR=$(OPTEE_DIR)/optee_os \
	-DLWS_OPTEE_DEV_KIT_DIR=$(OPTEE_DEV_KIT_DIR) \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DLWS_WITHOUT_TESTAPPS=1 \
	-DLWS_WITHOUT_EXTENSIONS=1 \
	-DLWS_WITH_ZLIB=0 \
	-DLWS_WITH_DIR=0 \
	-DLWS__INCLUDE_DIRS=$(INCLUDES) \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS="-Wno-unused-variable -I$(CURDIR)/early" \

all: libteep hello teep

include ../common.mk

clean:
	rm -rf $(BUILD)


