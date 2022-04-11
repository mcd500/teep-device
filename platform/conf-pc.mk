export PLAT = pc

export HELLO_TA_UUID  ?= 8d82573a-926d-4754-9353-32dc29997f74
export HELLO_TA_UUID_2 ?= 8d82573a-926d-4754-9353-32dc29997f75
export TEE_AGENT_UUID ?= 68373894-5bb3-403c-9eec-3114a1f5d3fc

export APP_CFLAGS = -g \
	-I./ \
	-I$(SOURCE)/include \
	-I$(SOURCE)/libteep/lib \
	-I$(BUILD)/libteep/host/libwebsockets/include \
	-I$(SOURCE)/submodule/libwebsockets/include \
	-I$(SOURCE)/submodule/mbedtls/include \
	-I$(BUILD)/libteep/host/QCBOR/inc \
	-I$(SOURCE)/suit/include \
	-I$(CURDIR)/include \
	-I$(SOURCE)/key/include \
	-DPCTEST -DPLAT_PC -DAPP_VERBOSE -Wall

export APP_LDFLAGS = \
	-L$(BUILD)/libteep/host/mbedtls/library \
	-L$(BUILD)/libteep/host/libwebsockets/lib \
	-L$(BUILD)/libteep/host/QCBOR \
	-L$(BUILD)/libteep/tee/libteesuit/lib

export APP_LIBS = \
	 -lwebsockets -lmbedtls -lmbedx509 -lmbedcrypto -lqcbor -lteesuit

ifeq ($(shell uname),Linux)
APP_LIBS += -lcap
endif

libteep-mbedtls-host-FLAGS = \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DUSE_STATIC_MBEDTLS_LIBRARY=1 \
	-DUSE_SAHTED_MBEDTLS_LIBRARY=0 \

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
	-DCMAKE_C_FLAGS='-Wno-enum-conversion'

libteesuit-tee-FLAGS = \
	-DCMAKE_BUILD_TYPE=DEBUG

libteep-mbedtls-ree-DISABLE = y
libteep-libwebsockets-ree-DISABLE = y
libteep-QCBOR-ree-DISABLE = y

libteep-QCBOR-tee-DISABLE = y
libteep-libteep-tee-DISABLE = y
