export BUILD ?= $(CURDIR)/build
export SOURCE ?= $(CURDIR)/../..
export PLAT = pc

export APP_CFLAGS = \
	-I./ \
	-I$(SOURCE)/libteep/lib \
	-I$(BUILD)/libteep/host/libwebsockets/include \
	-I$(SOURCE)/libteep/libwebsockets/include \
	-I$(SOURCE)/libteep/mbedtls/include \
	-I$(CURDIR)/include \
	-I$(SOURCE)/key/include
	-DPCTEST -DPLAT_PC -DAPP_VERBOSE -Wall

export APP_LDFLAGS = \
	-L$(BUILD)/libteep/host/mbedtls/library \
	-L$(BUILD)/libteep/host/libwebsockets/lib

export APP_LIBS = \
	 -lwebsockets -lmbedtls -lmbedx509 -lmbedcrypto -lcap

libteep-mbedtls-host-FLAGS = \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DUSE_STATIC_MBEDTLS_LIBRARY=1 \
	-DUSE_SAHTED_MBEDTLS_LIBRARY=0

libteep-libwebsockets-host-FLAGS = \
	-DLWS_WITH_SSL=1 \
	-DLWS_WITH_MBEDTLS=1 \
	-DLWS_WITH_JOSE=1 \
	-DLWS_MBEDTLS_INCLUDE_DIRS=../mbedtls/include \
	-DMBEDTLS_LIBRARY=$(BUILD)/libteep/host/mbedtls/library/libmbedtls.a \
	-DMBEDX509_LIBRARY=$(BUILD)/libteep/host/mbedtls/library/libmbedx509.a \
	-DMBEDCRYPTO_LIBRARY=$(BUILD)/libteep/host/mbedtls/library/libmbedcrypto.a \
	-DLWS_WITH_MINIMAL_EXAMPLES=1 \
	-DLWS_STATIC_PIC=ON \
	-DLWS_WITH_SHARED=OFF \
	-DLWS_WITH_STATIC=1 \
	-DLWS_WITHOUT_SERVER=1 \
	-DCMAKE_BUILD_TYPE=Debug \

libteep-mbedtls-ree-DISABLE = y
libteep-libwebsockets-ree-DISABLE = y
libteep-mbedtls-tee-DISABLE = y
libteep-libwebsockets-tee-DISABLE = y
libteep-libteep-ree-DISABLE = y

all: libteep teep-broker-app

include ../common.mk

clean:
	rm -rf $(BUILD)
