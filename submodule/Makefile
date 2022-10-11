TOPDIR = $(CURDIR)/..
include $(TOPDIR)/conf.mk

.PHONY: all
all: mbedtls-ree libwebsockets-ree


mbedtls-ree-common-FLAGS = \
	-DCMAKE_INSTALL_PREFIX:PATH=staging \
	-DENABLE_TESTING=0  \
	-DCMAKE_BUILD_TYPE=DEBUG \
	-DUSE_STATIC_MBEDTLS_LIBRARY=1 \
	-DUSE_SHARED_MBEDTLS_LIBRARY=0

# Building MBEDTLS Library only when the MBEDTLS source code is updated
MBEDTLS_BDIR := $(BUILD)/ree/mbedtls/
MBEDTLS_SRC :=  $(shell find mbedtls -name "*.c") \
		$(shell (find mbedtls -name "*.h"))
MBEDTLS_OBJ = $(MBEDTLS_BDIR)/library

.PHONY: mbedtls-ree
mbedtls-ree $(MBEDTLS_OBJ): $(MBEDTLS_SRC)
	mkdir -p $(BUILD)/ree/mbedtls
	cd $(BUILD)/ree/mbedtls && \
		cmake $($@-common-FLAGS) $(cmake-ree-TOOLCHAIN) $(TOPDIR)/submodule/mbedtls && \
		make -j `nproc`

libwebsockets-ree-common-FLAGS = \
	-DLWS_ROLE_WS=0 \
	-DLWS_WITH_SSL=1 \
	-DLWS_WITH_MBEDTLS=1 \
	-DMBEDTLS_LIBRARY=$(BUILD)/ree/mbedtls/library/libmbedtls.a \
	-DMBEDX509_LIBRARY=$(BUILD)/ree/mbedtls/library/libmbedx509.a \
	-DMBEDCRYPTO_LIBRARY=$(BUILD)/ree/mbedtls/library/libmbedcrypto.a \
	-DLWS_MBEDTLS_INCLUDE_DIRS="$(TOPDIR)/submodule/mbedtls/include" \
	-DLWS_WITH_JOSE=1 \
	-DLWS_WITHOUT_SERVER=1 \
	-DLWS_WITH_STATIC=1 \
	-DLWS_WITH_SHARED=0 \
	-DLWS_STATIC_PIC=1 \
	-DLWS_MAX_SMP=1 \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DLWS_WITHOUT_TESTAPPS=1 \
	-DLWS_WITHOUT_EXTENSIONS=1 \
	-DLWS_WITH_ZLIB=0


# Building MBEDTLS Library only when the MBEDTLS source code is updated
LWS_BDIR := $(BUILD)/ree/libwebsockets/
LWS_SRC := $(shell find libwebsockets -name "*.c") \
	   $(shell find libwebsockets -name "*.h") \
	   $(shell find libwebsockets -name "*.cmake")
LWS_OBJ = $(LWS_BDIR)/lib ./$(LWS_BDIR)/include

.PHONY: libwebsockets-ree
libwebsockets-ree  $(LWS_OBJ): $(MBEDTLS_OBJ) $(LWS_SRC)
	mkdir -p $(BUILD)/ree/libwebsockets
	cd $(BUILD)/ree/libwebsockets && \
		cmake $($@-common-FLAGS) $(cmake-ree-TOOLCHAIN) $(TOPDIR)/submodule/libwebsockets && \
		make -j `nproc`
