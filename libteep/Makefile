#
# We're building several different pieces in one step, related to OTRP above TEE level
#
# - aarch64 mbedtls dynamic library for REE
# - aarch64 libwebsockets dynamic library for REE
# - host    libwebsockets dynamic library for host (uses host mbedtls)
# - aarch64 libteep dynamic library for REE (include lws + mbedtls above)
# - aarch64 aist-otrp-testapp
# - host    aist-otrp-tam for host (uses host mbedtls)

REE_CFLAGS := -Wall -Werror -fPIC -I./libwebsockets/include -I./libwebsockets/build/include -I../../../mbedtls/include -I ./mbedtls/include -I../../optee_client/public -I$(TA_DEV_KIT_DIR)
CFLAGS += $(REE_CFLAGS) -I$(TA_DEV_KIT_DIR)/host_include
LDFLAGS += -Llibwebsockets/build/lib -Lmbedtls-build/lib


%.o: %.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@


all:
	#
	# mbedtls (REE library)
	#
	$(shell mkdir -p mbedtls-build/staging libwebsockets/build libwebsockets/build/build-host lib/build)
	cd mbedtls-build && \
	cmake ../mbedtls -DCMAKE_TOOLCHAIN_FILE=../cross-aarch64.cmake \
		 -DCMAKE_INSTALL_PREFIX:PATH=staging -DENABLE_TESTING=0  \
		 -DCMAKE_BUILD_TYPE=RELEASE -DUSE_SHARED_MBEDTLS_LIBRARY=1 && \
	make -j `nproc`
	#
	# libwebsockets (REE library)
	#
	cd libwebsockets/build && \
	cmake -DCMAKE_TOOLCHAIN_FILE=../../cross-aarch64.cmake \
	 CMAKE_SYSTEM_PROCESSOR=aarch64 \
	 -DLWS_ROLE_WS=0 \
	 -DLWS_WITH_SSL=1 \
	 -DLWS_WITH_MBEDTLS=1 \
	 -DLWS_MBEDTLS_LIBRARIES="mbedx509.so;mbedtls.so;mbedcrypto.so" \
	 -DLWS_MBEDTLS_INCLUDE_DIRS="../mbedtls/include" \
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
	#
	# libwebsockets (host library)
	#
	# (on older build hosts, mbedtls is too old, so use OpenSSL)
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

	#
	# libteep (REE library)
	#
	$(CROSS_COMPILE)gcc $(REE_CFLAGS) -Ilib -c lib/main.c -o lib/main.o
	$(CROSS_COMPILE)gcc $(LDFLAGS) \
		-shared -Wl,-soname,libteep.so.1 \
		-L../../optee_client/out/libteec -L mbedtls-build/library \
		-Llibwebsockets/build/lib \
		lib/main.o -lteec -lwebsockets -lmbedx509 -lmbedtls -l mbedcrypto \
		-o lib/libteep.so.1
	ln -sf libteep.so.1 lib/libteep.so

clean:
	rm -fr lib/main.o lib/libteep.so* mbedtls-build libwebsockets/build libwebsockets/build/build-host lib/build

update:
	scp libteep.so root@${HIKEY_IP}:/usr/bin

.PHONY: .FORCE
