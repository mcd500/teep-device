TOPDIR = $(CURDIR)/../..
include $(TOPDIR)/conf.mk

.PHONY: build
build: app enclave manifest

.PHONY: all
app:
	make -f app.mk

.PHONY: enclave
enclave:
	make -f enclave.mk out-dir=$(BUILD)/hello-tc

.PHONY: clean
clean: clean-manifest
	rm -rf $(BUILD)/hello-tc

MANIFEST_OUT_DIR = $(BUILD)/hello-tc
MANIFEST_DIR = ../manifest
SUIT_PRIV_KEY = $(TOPDIR)/key/tc-signer-P256-priv.pem
TC_URI = $(TAM_URL)/TAs/$(HELLO_TA_UUID_NAME)
TC_BINARY = $(BUILD)/hello-tc/$(HELLO_TA_UUID_NAME)

include ../manifest/manifest.mk
