TOPDIR = $(CURDIR)/..
include $(TOPDIR)/conf.mk

.PHONY: all
all: mbedtls-ree libwebsockets-ree QCBOR-tee


mbedtls-ree-common-FLAGS = \
	-DCMAKE_INSTALL_PREFIX:PATH=staging \
	-DENABLE_TESTING=0  \
	-DCMAKE_BUILD_TYPE=DEBUG \
	-DUSE_STATIC_MBEDTLS_LIBRARY=1 \
	-DUSE_SHARED_MBEDTLS_LIBRARY=0

.PHONY: mbedtls-ree
mbedtls-ree:
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

.PHONY: libwebsockets-ree
libwebsockets-ree:
	mkdir -p $(BUILD)/ree/libwebsockets
	cd $(BUILD)/ree/libwebsockets && \
		cmake $($@-common-FLAGS) $(cmake-ree-TOOLCHAIN) $(TOPDIR)/submodule/libwebsockets && \
		make -j `nproc`

.PHONY: QCBOR-tee
QCBOR-tee:
	mkdir -p $(BUILD)/tee/QCBOR
	rm -rf $(BUILD)/tee/QCBOR
	cp -r $(TOPDIR)/submodule/QCBOR $(BUILD)/tee/QCBOR
	make -C $(BUILD)/tee/QCBOR CC=$(CROSS_COMPILE)gcc
