LWS_GIT_URL  ?= https://github.com/warmcat/libwebsockets.git
LWS_BRANCH   ?= for-teep-device
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

MBEDTLS_BDIR := build-mbedtls
MBEDTLS_SRC :=  $(shell find mbedtls -name "*.c") \
                $(shell (find mbedtls -name "*.h"))
MBEDTLS_OBJ = $(MBEDTLS_BDIR)/library

LWS_M_BDIR := build-lws-mbed
LWS_SRC := $(shell find libwebsockets -name "*.c") \
           $(shell find libwebsockets -name "*.h") \
           $(shell find libwebsockets -name "*.cmake")
LWS_OBJ = $(LWS_M_BDIR)/lib ./$(LWS_M_BDIR)/include

LIBTEEP_SRC := lib/main.c
LIBTEEP_OBJ := lib/libteep.so

REE_CFLAGS := -Wall -Werror -fPIC \
	      -I./$(LWS_M_BDIR)/include -I ./libwebsockets/include \
	      -I ./mbedtls/include $(INCLUDES) \
	      -I$(OPTEE_DIR)/optee_client/public -I$(TA_DEV_KIT_DIR)
CFLAGS += $(REE_CFLAGS) -I$(TA_DEV_KIT_DIR)/host_include
LDFLAGS += -L$(LWS_M_BDIR)/lib -L$(MBEDTLS_BDIR)/library $(LDTEEC)


%.o: %.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@

.PHONY: all
all: $(LIBTEEP_OBJ) libwebsockets-tee

.PHONY: mbedtls-ree
mbedtls-ree $(MBEDTLS_OBJ): $(MBEDTLS_SRC)
	#
	# mbedtls (REE library)
	#
#	rm -fr mbedtls
#	git clone $(MBED_GIT_URL);
	cd mbedtls; git checkout $(MBED_BRANCH)
	mkdir -p $(MBEDTLS_BDIR)/staging
	cd $(MBEDTLS_BDIR) && \
	cmake -DCMAKE_TOOLCHAIN_FILE=../cross-aarch64.cmake \
		 -DCMAKE_INSTALL_PREFIX:PATH=staging -DENABLE_TESTING=0  \
		 -DCMAKE_BUILD_TYPE=RELEASE -DUSE_SHARED_MBEDTLS_LIBRARY=1 \
		 ../mbedtls && \
	make -j `nproc`

.PHONY: libwebsockets-ree
libwebsockets-ree $(LWS_OBJ): $(MBEDTLS_OBJ) $(LWS_SRC)
	#
	# libwebsockets (REE library)
	#
#	rm -fr libwebsockets
#	git clone $(LWS_GIT_URL)
	#cd libwebsockets; git checkout $(LWS_BRANCH)
	mkdir -p $(LWS_M_BDIR)
	cd $(LWS_M_BDIR) && \
	cmake -DCMAKE_TOOLCHAIN_FILE=../cross-aarch64.cmake \
	 CMAKE_SYSTEM_PROCESSOR=aarch64 \
	 -DLWS_ROLE_WS=0 \
	 -DLWS_WITH_SSL=1 \
	 -DLWS_WITH_MBEDTLS=1 \
	 -DMBEDTLS_LIBRARY=../$(MBEDTLS_BDIR)/library/libmbedtls.so \
	 -DMBEDX509_LIBRARY=../$(MBEDTLS_BDIR)/library/libmbedx509.so \
	 -DMBEDCRYPTO_LIBRARY=../$(MBEDTLS_BDIR)/library/libmbedcrypto.so \
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
	 ../libwebsockets && \
	make -j `nproc`
	$(CROSS_COMPILE)strip $(LWS_M_BDIR)/lib/libwebsockets.so*

OPTEE_DEV_KIT_DIR ?= ../../build-optee/optee_os/out/arm

.PHONY: libwebsockets-tee
libwebsockets-tee:
	cd libwebsockets; git checkout $(LWS_BRANCH)
	mkdir -p build-lws-tee
	cd build-lws-tee && \
	cmake -DCMAKE_TOOLCHAIN_FILE=../cross-aarch64-tee.cmake \
		CMAKE_SYSTEM_PROCESSOR=aarch64 \
		-DLWS_ROLE_WS=0 \
		-DLWS_WITH_SSL=1 \
		-DLWS_WITH_MBEDTLS=1 \
		-DLWS_MBEDTLS_LIBRARIES=libmbedtls.a \
		-DLWS_MBEDTLS_INCLUDE_DIRS="$(OPTEE_DEV_KIT_DIR)/export-ta_arm64/include;../../../build-optee/optee_os/lib/libmbedtls/mbedtls/include;../../../build-optee/optee_os/lib/libmbedtls/include" \
		-DLWS_WITH_JOSE=1 \
		-DLWS_WITHOUT_SERVER=1 \
		-DLWS_WITH_STATIC=1 \
		-DLWS_WITH_SHARED=0 \
		-DLWS_STATIC_PIC=1 \
		-DLWS_MAX_SMP=1 \
		-DLWS_PLAT_OPTEE=1 \
		-DLWS_OPTEE_DIR=../../../build-optee/optee_os \
		-DLWS_OPTEE_DEV_KIT_DIR=$(OPTEE_DEV_KIT_DIR) \
		-DCMAKE_BUILD_TYPE=RELEASE \
		-DLWS_WITHOUT_TESTAPPS=1 \
		-DLWS_WITHOUT_EXTENSIONS=1 \
		-DLWS_WITH_ZLIB=0 \
		-DLWS_WITH_DIR=0 \
		-DLWS__INCLUDE_DIRS=$(INCLUDES) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS=-Wno-unused-variable \
		../libwebsockets && make V=1 VERBOSE=1

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
libteep-ree $(LIBTEEP_OBJ): $(LWS_OBJ) $(LIBTEEP_SRC)
	#
	# libteep (REE library)
	#
	$(shell mkdir -p lib/build)
	$(CROSS_COMPILE)gcc $(REE_CFLAGS) -Ilib -c $(LIBTEEP_SRC) -o lib/main.o
	$(CROSS_COMPILE)gcc $(LDFLAGS) \
		-shared -Wl,-soname,libteep.so.1 \
		-L$(OPTEE_DIR)/out-br/target/usr/lib \
		lib/main.o -lwebsockets -lmbedx509 -lmbedtls -lmbedcrypto \
		-o lib/libteep.so.1
	ln -sf libteep.so.1 lib/libteep.so

.PHONY: clean
clean:
	rm -fr lib/main.o lib/build lib/libteep.so*
	rm -fr $(MBEDTLS_BDIR) $(LWS_M_BDIR)
	rm -fr libwebsockets/build/build-host
	rm -fr build-lws-tee

.PHONY: update
update:
	scp libteep.so root@${HIKEY_IP}:/usr/bin

.PHONY: .FORCE
