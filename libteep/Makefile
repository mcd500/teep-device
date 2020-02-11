LWS_GIT_URL  ?= https://github.com/warmcat/libwebsockets.git
LWS_BRANCH   ?= v3.2-stable
MBED_GIT_URL ?= https://github.com/ARMmbed/mbedtls.git
MBED_BRANCH  ?= mbedtls-2.16

#SHELL = /bin/bash -xeu

#
# We're building several different pieces in one step, related to OTRP above TEE level
#
# - aarch64 mbedtls dynamic library for REE
# - aarch64 libwebsockets dynamic library for REE
# - host    libwebsockets dynamic library for host (uses host mbedtls)
# - aarch64 libteep dynamic library for REE (include lws + mbedtls above)
# - aarch64 aist-otrp-testapp
# - host    aist-otrp-tam for host (uses host mbedtls)

M_REE_BDIR := mbedtls-build

REE_CFLAGS := -Wall -Werror -fPIC -I./libwebsockets/include -I./libwebsockets/build/include -I../../../mbedtls/include -I ./mbedtls/include -I../../optee_client/public -I$(TA_DEV_KIT_DIR)
CFLAGS += $(REE_CFLAGS) -I$(TA_DEV_KIT_DIR)/host_include
LDFLAGS += -Llibwebsockets/build/lib -L$(M_REE_BDIR)/lib

MBEDTLS_REE_OBJS := $(M_REE_BDIR)/library/libmbedtls.so $(M_REE_BDIR)/library/libmbedx509.so \
		    $(M_REE_BDIR)/library/libmbedcrypto.so

%.o: %.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@

.PHONY: all
all: libteep-ree

.PHONY: mbedtls-ree
mbedtls-ree $(MBEDTLS_REE_OBJS):
	#
	# mbedtls (REE library)
	#
#	rm -fr mbedtls
#	git clone $(MBED_GIT_URL);
#	cd mbedtls; git checkout $(MBED_BRANCH)
	mkdir -p $(M_REE_BDIR)/staging
	cd $(M_REE_BDIR) && \
	cmake -DCMAKE_TOOLCHAIN_FILE=../cross-aarch64.cmake \
		 -DCMAKE_INSTALL_PREFIX:PATH=staging -DENABLE_TESTING=0  \
		 -DCMAKE_BUILD_TYPE=RELEASE -DUSE_SHARED_MBEDTLS_LIBRARY=1 ../mbedtls && \
	make -j `nproc`

.PHONY: libwebsockets-ree
libwebsockets-ree libwebsockets/build/lib/libwebsockets.so: $(MBEDTLS_REE_OBJS)
	#
	# libwebsockets (REE library)
	#
#	rm -fr libwebsockets
#	git clone $(LWS_GIT_URL)
#	cd libwebsockets; git checkout $(LWS_BRANCH)
	mkdir -p libwebsockets/build
	cd libwebsockets/build && \
	cmake -DCMAKE_TOOLCHAIN_FILE=../../cross-aarch64.cmake \
	 CMAKE_SYSTEM_PROCESSOR=aarch64 \
	 -DLWS_ROLE_WS=0 \
	 -DLWS_WITH_SSL=1 \
	 -DLWS_WITH_MBEDTLS=1 \
	 -DMBEDTLS_LIBRARY=../../$(M_REE_BDIR)/library/libmbedtls.so \
	 -DMBEDX509_LIBRARY=../../$(M_REE_BDIR)/library/libmbedx509.so \
	 -DMBEDCRYPTO_LIBRARY=../../$(M_REE_BDIR)/library/libmbedcrypto.so \
	 -DLWS_MBEDTLS_INCLUDE_DIRS="../../mbedtls/include" \
	 -DLWS_WITH_JOSE=1 \
	 -DLWS_WITHOUT_SERVER=1 \
	 -DLWS_WITH_STATIC=0 \
	 -DLWS_WITH_SHARED=1 \
	 -DLWS_STATIC_PIC=1 \
	 -DLWS_MAX_SMP=1 \
	 -DCMAKE_BUILD_TYPE=RELEASE \
	 -DLWS_WITHOUT_TESTAPPS=1 \
	 -DLWS_WITHOUT_EXTENSIONS=1 \
	 -DLWS_WITH_ZLIB=0 \
	.. && \
	make -j `nproc`
	$(CROSS_COMPILE)strip libwebsockets/build/lib/libwebsockets.so*

.PHONY: libwebsockets-host
libwebsockets-host:
	#
	# libwebsockets (host library)
	#
	# (on older build hosts, mbedtls is too old, so use OpenSSL)
	$(shell mkdir -p libwebsockets/build/build-host)
	export MBTLS=0 && \
	if [ -e "/usr/include/mbedtls/net_sockets.h" ] ; then export MBTLS=1; fi && \
	cd libwebsockets/build/build-host && \
	cmake \
	 -DLWS_ROLE_WS=0 \
	 -DLWS_WITH_SSL=1 \
	 -DLWS_WITH_MBEDTLS=$$MBTLS \
	 -DLWS_WITH_JOSE=1 \
	 -DLWS_WITHOUT_SERVER=0 \
	 -DLWS_WITH_STATIC=0 \
	 -DLWS_WITH_SHARED=1 \
	 -DLWS_STATIC_PIC=1 \
	 -DLWS_MAX_SMP=1 \
	 -DCMAKE_BUILD_TYPE=RELEASE \
	 -DLWS_WITHOUT_TESTAPPS=1 \
	 -DLWS_WITHOUT_EXTENSIONS=1 \
	 -DLWS_WITH_ZLIB=0 \
	../.. && \
	make -j `nproc`

.PHONY: libteep-ree
libteep-ree: libwebsockets/build/lib/libwebsockets.so
	#
	# libteep (REE library)
	#
	$(shell mkdir -p lib/build)
	$(CROSS_COMPILE)gcc $(REE_CFLAGS) -Ilib -c lib/main.c -o lib/main.o
	$(CROSS_COMPILE)gcc $(LDFLAGS) \
		-shared -Wl,-soname,libteep.so.1 \
		-L../../optee_client/out/libteec -L mbedtls-build/library \
		-Llibwebsockets/build/lib \
		lib/main.o -lteec -lwebsockets -lmbedx509 -lmbedtls -l mbedcrypto \
		-o lib/libteep.so.1
	ln -sf libteep.so.1 lib/libteep.so

.PHONY: clean
clean:
	rm -fr lib/main.o lib/libteep.so* mbedtls-build libwebsockets/build libwebsockets/build/build-host lib/build

.PHONY: update
update:
	scp libteep.so root@${HIKEY_IP}:/usr/bin

.PHONY: .FORCE
