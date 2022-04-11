PLAT = optee

export OPTEE_DEV_KIT_DIR ?= $(OPTEE_DIR)/optee_os/out/arm
export TA_DEV_KIT_DIR ?= $(OPTEE_DEV_KIT_DIR)/export-ta_arm64

export HELLO_TA_UUID  ?= 8d82573a-926d-4754-9353-32dc29997f74
export TEE_AGENT_UUID ?= 68373894-5bb3-403c-9eec-3114a1f5d3fc

export TEE_CFLAGS = \
	-I$(BUILD)/libteep/tee/QCBOR/inc \
	-I$(SOURCE)/include \
	-I$(TEE_REF_TA_DIR)/include \
	-I$(TAREF_DIR)/build/include

export APP_CFLAGS ?=  $(TEE_CFLAGS) \
	-Wall \
	-DPLAT_OPTEE=1 \
	-I$(SOURCE)/libteep/lib \
	-I$(SOURCE)/submodule/libwebsockets/include \
	-I$(SOURCE)/submodule/mbedtls/include \
	-I$(OPTEE_DIR)/optee_client/public \
	-I$(BUILD)/libteep/ree/mbedtls/include \
	-I$(BUILD)/libteep/ree/libwebsockets/include \

export APP_LDFLAGS ?= \
	-L$(OPTEE_DIR)/out-br/target/usr/lib \
	-L$(OPTEE_DIR)/optee_client/out/libteec \
	-L$(BUILD)/libteep/ree/lib \
	-L$(BUILD)/libteep/ree/mbedtls/library \
	-L$(BUILD)/libteep/ree/libwebsockets/lib

export APP_LIBS ?= \
	-lteec \
	-lwebsockets \
	-lmbedtls \
	-lmbedcrypto \
	-lmbedx509

export TA_CFLAGS = $(TEE_CFLAGS) \
	-nostdinc -DPLAT_OPTEE=1  \
	-Wall \
	-Wno-overlength-strings \
	-I$(TA_DEV_KIT_DIR) \
	-I$(BUILD)/libteep/tee/libwebsockets/include \
	-I$(SOURCE)/libteep/lib \
	-I$(SOURCE)/suit/include \
	-I$(SOURCE)/key/include

export TA_LDFLAGS = \
	-L$(BUILD)/libteep/tee/lib \
	-L$(BUILD)/libteep/tee/QCBOR \
	-L$(BUILD)/libteep/tee/libteesuit/lib

export TA_LIBS = \
	-lqcbor -lteesuit

TOOLCHAIN-ree = $(CURDIR)/cross-aarch64.cmake
TOOLCHAIN-tee = $(CURDIR)/cross-aarch64-tee.cmake

REE_CFLAGS ?= -Wall -Werror -fPIC \
	      -I$(BUILD)/libteep/ree/libwebsockets/include \
		  -I$(SOURCE)/submodule/libwebsockets/include \
	      -I$(SOURCE)/submodule/mbedtls/include $(INCLUDES) \
	      -I$(OPTEE_DIR)/optee_client/public \
		  -I$(TA_DEV_KIT_DIR)/export-ta_arm64/include
REE_LDFLAGS ?= \
	-L$(BUILD)/libteep/ree/libwebsockets/lib \
	-L$(BUILD)/libteep/ree/mbedtls/library
export CROSS_COMPILE = aarch64-linux-gnu-
LIBTEEP_SRC = $(SOURCE)/libteep/lib/libteep.c

libteep-mbedtls-host-DISABLE = y
libteep-libwebsockets-host-DISABLE = y
libteep-QCBOR-host-DISABLE = y

libteep-QCBOR-ree-DISABLE = y

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
