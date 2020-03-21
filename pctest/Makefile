LWS_GIT_URL  ?= https://github.com/warmcat/libwebsockets.git
LWS_BRANCH   ?= v3.2-stable
MBED_GIT_URL ?= https://github.com/ARMmbed/mbedtls.git
MBED_BRANCH  ?= mbedtls-2.16

SHELL = /bin/bash -eu

CFLAGS := -g -O0 -Wall
UNAME_S := $(shell uname -s)
IP_ADDR := $(shell ../scripts/get-ip.sh)
PORT := 3000

TAM_PUB_JWK  := ../test-jw/tsm/identity/tam-mytam-public.jwk
TAM_PRIV_JWK := ../test-jw/tsm/identity/private/tam-mytam-private.jwk
TEE_PUB_JWK  := ../test-jw/tee/identity/tee-mytee-public.jwk
TEE_PRIV_JWK := ../test-jw/tee/identity/private/tee-mytee-private.jwk
SP_PUB_JWK   := ../test-jw/tee/sds/xbank/spaik-pub.jwk
SP_PRIV_JWK  := ../test-jw/tee/sds/xbank/spaik-priv.jwk

TEEP_KEYS := $(TAM_PRIV_JWK) $(TAM_PUB_JWK) $(TEE_PRIV_JWK) $(TEE_PUB_JWK) \
	     $(SP_PRIV_JWK) $(SP_PUB_JWK)

TEEP_KEY_H := ../teep-agent-ta/tam_id_pubkey_jwk.h \
	      ../teep-agent-ta/tee_id_privkey_jwk.h \
	      ../teep-agent-ta/tee_id_pubkey_jwk.h \
	      ../teep-agent-ta/sp_pubkey_jwk.h

TARGET_SRC := teec_stub.c ../teep-broker-app/main.c ../libteep/lib/main.c \
	      ../teep-agent-ta/ta-store.c ../teep-agent-ta/teep_message.c

MBEDTLS_BDIR := build-mbedtls
MBEDTLS_SRC :=  $(shell find mbedtls -name "*.c") \
		$(shell (find mbedtls -name "*.h"))
MBEDTLS_OBJ = $(MBEDTLS_BDIR)/library

LWS_M_BDIR := build-lws-mbed
LWS_O_BDIR := build-lws-ossl
LWS_SRC := $(shell find libwebsockets -name "*.c") \
	   $(shell find libwebsockets -name "*.h") \
	   $(shell find libwebsockets -name "*.cmake")
LWS_OBJ = $(LWS_M_BDIR)/lib ./$(LWS_M_BDIR)/include

INCLUDES := -I../libteep/lib -I./$(LWS_M_BDIR)/include -I./mbedtls/include \
	    -I../../optee_client/public -I../teep-agent-ta
CFLAGS  += $(INCLUDES) -DPCTEST=1
LDFLAGS += -L$(LWS_M_BDIR)/lib
STATIC_LIB :=   ./$(LWS_M_BDIR)/lib/libwebsockets.a \
		./$(MBEDTLS_BDIR)/library/libmbedtls.a \
		./$(MBEDTLS_BDIR)/library/libmbedx509.a \
		./$(MBEDTLS_BDIR)/library/libmbedcrypto.a

MBED_JWE = ./$(LWS_M_BDIR)/bin/lws-crypto-jwe
OSSL_JWE = ./$(LWS_O_BDIR)/bin/lws-crypto-jwe

SCRIPT := ita dta ista dsta iota dota
SCRIPT_SH := $(patsubst %, %.sh, $(SCRIPT))

TARGET = teep-broker-app

# Makefile for TEEP Agent of UT

.PHONY: all
all: $(TEEP_KEY_H) $(LWS_OBJ) $(TARGET) $(SCRIPT)

$(SCRIPT): %: ../teep-broker-app/%.sh
	cp ../teep-broker-app/$^ .
	sed -e "s/127\.0\.\0\.1/$(IP_ADDR)/g; \
		s/teep-broker-app/\.\/teep-broker-app/g" < $^ > $@
	chmod u+x $@

$(TEEP_KEYS):
	make -C .. generate-jwks

$(TEEP_KEY_H): $(TEEP_KEYS)
	make -C .. generate-jwk-headers

$(TARGET): $(TARGET_SRC) $(STATIC_LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(STATIC_LIB) $(LDLIBS) -o $@

.PHONY: mbedtls
mbedtls $(MBEDTLS_OBJ): $(MBEDTLS_SRC)
#	rm -fr /mbedtls
#	git clone $(MBED_GIT_URL)
	cd mbedtls; git checkout $(MBED_BRANCH)
	mkdir -p $(MBEDTLS_BDIR); cd $(MBEDTLS_BDIR); \
	cmake 	\
		-DCMAKE_BUILD_TYPE=RELEASE \
		-DUSE_STATIC_MBEDTLS_LIBRARY=1 \
		-DUSE_SAHTED_MBEDTLS_LIBRARY=0 \
		../mbedtls; \
	make -j `nproc`

.PHONY: libwebsockets
libwebsockets $(LWS_OBJ): $(MBEDTLS_OBJ) $(LWS_SRC)
#	rm -fr libwebsockets
#	git clone $(LWS_GIT_URL)
	cd libwebsockets; git checkout $(LWS_BRANCH);
	mkdir -p $(LWS_M_BDIR); cd $(LWS_M_BDIR); \
	cmake 	-DLWS_WITH_SSL=1 \
		-DLWS_WITH_MBEDTLS=1 \
		-DLWS_WITH_JOSE=1 \
		-DLWS_MBEDTLS_INCLUDE_DIRS=../mbedtls/include \
		-DMBEDTLS_LIBRARY=../$(MBEDTLS_BDIR)/library/libmbedtls.a \
		-DMBEDX509_LIBRARY=../$(MBEDTLS_BDIR)/library/libmbedx509.a \
		-DMBEDCRYPTO_LIBRARY=../$(MBEDTLS_BDIR)/library/libmbedcrypto.a \
		-DLWS_WITH_MINIMAL_EXAMPLES=1 \
		-DLWS_STATIC_PIC=ON \
		-DLWS_WITH_SHARED=OFF \
		-DLWS_WITH_STATIC=1 \
		-DLWS_WITHOUT_SERVER=1 \
		-DCMAKE_BUILD_TYPE=Debug \
		../libwebsockets; \
	make -j `nproc`;

$(MBED_JWE) $(OSSL_JWE): $(LWS_SRC)
	mkdir -p $(LWS_O_BDIR); cd $(LWS_O_BDIR); \
	cmake 	-DLWS_WITH_SSL=1 \
		-DLWS_WITH_JOSE=1 \
		-DLWS_WITH_MINIMAL_EXAMPLES=1 \
		../libwebsockets; \
	make -j `nproc`

.PHONY: clean-pctest
clean-pctest:
	$(RM) $(TARGET) $(SCRIPT) $(SCRIPT_SH)

.PHONY: clean
clean: clean-pctest
	rm -fr $(LWS_M_BDIR)
	rm -fr $(LWS_O_BDIR)
	rm -fr $(MBEDTLS_BDIR)

.PHONY: run
run: $(TARGET)
	./$(TARGET) --tamurl http://$(IP_ADDR):$(PORT)/api/tam

.PHONY: drun
drun: $(TARGET)
	./$(TARGET) --tamurl http://$(IP_ADDR):$(PORT)/api/tam_delete --talist 8d82573a-926d-4754-9353-32dc29997f74

.PHONY: run-jose
run-jose: $(TARGET)
	./$(TARGET) --tamurl http://$(IP_ADDR):$(PORT)/api/tam_jose2 --jose

.PHONY: drun-jose
drun-jose: $(TARGET)
	./$(TARGET) --tamurl http://$(IP_ADDR):$(PORT)/api/tam_jose_delete --jose --talist 8d82573a-926d-4754-9353-32dc29997f74

.PHONY: run-otrp
run-otrp: $(TARGET)
	./$(TARGET) --tamurl http://${IP_ADDR}:3000/api/tam_jose -p otrp

.PHONY: run-otrp-jose
run-otrp-jose: $(TARGET)
	./$(TARGET) --tamurl http://${IP_ADDR}:3000/api/tam_jose --jose -p otrp

.PHONY: run-otrp-delete
run-otrp-delete: $(TARGET)
	./$(TARGET) --tamurl http://${IP_ADDR}:3000/api/tam_jose_delete -p otrp

.PHONY: run-otrp-jose-delete
run-otrp-jose-delete: $(TARGET)
	./$(TARGET) --tamurl http://${IP_ADDR}:3000/api/tam_jose_delete --jose -p otrp

.PHONY: lws-sample-ok lws-sample-fail
lws-sample-ok: $(MBED_JWE) $(OSSL_JWE)
	echo -n 0123456789ABCDEF | \
		$(MBED_JWE) -f -k $(TAM_PUB_JWK) -e "RSA1_5 A128CBC-HS256"
	echo -n 0123456789ABCDEF | \
		$(OSSL_JWE) -f -k $(TAM_PUB_JWK) -e "RSA1_5 A128CBC-HS256"
	echo -n 0123456789ABCDEF | \
		$(OSSL_JWE) -f -k $(TAM_PUB_JWK) -e "RSA-OAEP A128CBC-HS256"
	echo -n 0123456789ABCDEF | \
		$(OSSL_JWE) -f -k $(TAM_PUB_JWK) -e "RSA-OAEP A128CBC-HS256" | \
		$(OSSL_JWE) -f -k $(TAM_PRIV_JWK); echo

lws-sample-fail: $(MBED_JWE) $(OSSL_JWE)
	# no padding bug
	echo -n 0123456789ABCDE  | \
		$(MBED_JWE) -f -k $(TAM_PUB_JWK) -e "RSA1_5 A128CBC-HS256"
	echo -n 0123456789ABCDE  | \
		$(OSSL_JWE) -f -k $(TAM_PUB_JWK) -e "RSA1_5 A128CBC-HS256"
	# RSA-OAEP not supported
	echo -n 0123456789ABCDEF | \
		$(MBED_JWE) -f -k $(TAM_PUB_JWK) -e "RSA-OAEP A128CBC-HS256"
	echo -n 0123456789ABCDEF | \
		$(OSSL_JWE) -f -k $(TAM_PUB_JWK) -e "RSA-OAEP A128CBC-HS256" | \
		$(MBED_JWE) -f -k $(TAM_PRIV_JWK)
