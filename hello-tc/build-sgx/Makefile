TOPDIR = $(CURDIR)/../..
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
	rm -f *.o App-sgx *.so

MANIFEST_OUT_DIR = $(CURDIR)
MANIFEST_DIR = ../manifest
SUIT_PRIV_KEY = $(TOPDIR)/key/tc-signer-P256-priv.pem
TC_URI = $(TAM_URL)/TAs/$(HELLO_TA_UUID_NAME)
TC_BINARY = $(HELLO_TA_UUID_NAME)

include ../manifest/manifest.mk
