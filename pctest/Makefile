LWS_GIT_URL  ?= https://github.com/warmcat/libwebsockets.git
LWS_BRANCH   ?= v3.2-stable
MBED_GIT_URL ?= https://github.com/ARMmbed/mbedtls.git
MBED_BRANCH  ?= mbedtls-2.16

SHELL = /bin/bash -x

CFLAGS := -g -O0 -Wall
LDLIBS := -lwebsockets
INCLUDES := -I../libteep/lib -I./libwebsockets/build/include -I./mbedtls/include -I../../optee_client/public -I../teep-agent-ta
CFLAGS += $(INCLUDES) -DPCTEST=1
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS := -L/usr/local/opt/libwebsockets/lib
endif
LDFLAGS+=-Llibwebsockets/build/lib

TARGET = pctest

# Makefile for TEEP Agent of UT

.PHONY: all
all: mbedtls/build/library/libmbedtls.so libwebsockets/build/lib/libwebsockets.so $(TARGET)

$(TARGET): $(TARGET).c ../teep-broker-app/main.c ../libteep/lib/main.c ../teep-agent-ta/teep_message.c ../teep-agent-ta/ta-store.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: clean
clean:
	$(RM) $(TARGET)

.PHONY: run
run: $(TARGET)
	./$(TARGET) --tamurl http://127.0.0.1:3000/api/tam

.PHONY: run-jose
run-jose: $(TARGET)
	./$(TARGET) --jose --tamurl http://127.0.0.1:3000/api/tam_jose

MBED_JWE = ./libwebsockets/build/bin/lws-crypto-jwe
OSSL_JWE = ./libwebsockets/build-ossl/bin/lws-crypto-jwe
KEY_PRV = ../test-jw/tsm/identity/private/tam-mytam-private.jwk
KEY_PUB = ../test-jw/tsm/identity/tam-mytam-public.jwk

.PHONY: lws-sample-ok lws-sample-fail
lws-sample-ok:
	echo -n 0123456789ABCDEF | \
		$(MBED_JWE) -f -k $(KEY_PUB) -e "RSA1_5 A128CBC-HS256"
	echo -n 0123456789ABCDEF | \
		$(OSSL_JWE) -f -k $(KEY_PUB) -e "RSA1_5 A128CBC-HS256"
	echo -n 0123456789ABCDEF | \
		$(OSSL_JWE) -f -k $(KEY_PUB) -e "RSA-OAEP A128CBC-HS256"
	echo -n 0123456789ABCDEF | \
		$(OSSL_JWE) -f -k $(KEY_PUB) -e "RSA-OAEP A128CBC-HS256" | \
		$(OSSL_JWE) -f -k $(KEY_PRV); echo

lws-sample-fail:
	# no padding bug
	echo -n 0123456789ABCDE  | \
		$(MBED_JWE) -f -k $(KEY_PUB) -e "RSA1_5 A128CBC-HS256"
	echo -n 0123456789ABCDE  | \
		$(OSSL_JWE) -f -k $(KEY_PUB) -e "RSA1_5 A128CBC-HS256"
	# RSA-OAEP not supported
	echo -n 0123456789ABCDEF | \
		$(MBED_JWE) -f -k $(KEY_PUB) -e "RSA-OAEP A128CBC-HS256"
	echo -n 0123456789ABCDEF | \
		$(OSSL_JWE) -f -k $(KEY_PUB) -e "RSA-OAEP A128CBC-HS256" | \
		$(MBED_JWE) -f -k $(KEY_PRV)

.PHONY: mbedtls
mbedtls mbedtls/build/library/libmbedtls.so:
#	rm -fr mbedtls
#	git clone $(MBED_GIT_URL)
	set -e; cd $(CURDIR)/mbedtls; \
	git checkout $(MBED_BRANCH); \
	mkdir build; cd build; \
	cmake 	\
		-DCMAKE_BUILD_TYPE=RELEASE \
		-DUSE_SHARED_MBEDTLS_LIBRARY=1 \
		..; \
	make -j `nproc`
	sudo ldconfig $(CURDIR)/mbedtls/build/library/

.PHONY: libwebsockets
libwebsockets libwebsockets/build/lib/libwebsockets.so: mbedtls/build/library/libmbedtls.so
#	rm -fr libwebsockets
#	git clone $(LWS_GIT_URL)
	set -e; cd $(CURDIR)/libwebsockets; \
	git checkout $(LWS_BRANCH); \
	mkdir build; cd build; \
	cmake 	-DLWS_WITH_SSL=1 \
		-DLWS_WITH_MBEDTLS=1 \
		-DLWS_WITH_JOSE=1 \
		-DLWS_MBEDTLS_INCLUDE_DIRS=../../mbedtls/include \
		-DMBEDTLS_LIBRARY=../../mbedtls/build/library/libmbedtls.so \
		-DMBEDX509_LIBRARY=../../mbedtls/build/library/libmbedx509.so \
		-DMBEDCRYPTO_LIBRARY=../../mbedtls/build/library/libmbedcrypto.so \
		-DLWS_WITH_MINIMAL_EXAMPLES=1 \
		..; \
	make -j `nproc`; \
	mkdir ../build-ossl; cd ../build-ossl; \
	cmake 	-DLWS_WITH_SSL=1 \
		-DLWS_WITH_JOSE=1 \
		-DLWS_WITH_MINIMAL_EXAMPLES=1 \
		..; \
	make -j `nproc`
	sudo ldconfig $(CURDIR)/libwebsockets/build/lib
