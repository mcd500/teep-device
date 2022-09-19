export MACHINE ?= SIM

.PHONY: build
build: app enclave manifest

.PHONY: app
app:
	make -f app.mk

.PHONY: enclave
enclave:
	make -f enclave.mk

.PHONY: clean
clean: clean-manifest
	rm -f *.o App_sgx *.so

MANIFEST_OUT_DIR = $(CURDIR)
MANIFEST_DIR = ../manifest
SUIT_PRIV_KEY = ../../key/tc-provider-priv.pem
TC_URI = $(TAM_URL)/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
TC_BINARY = enclave.signed.so

include ../manifest/manifest.mk
