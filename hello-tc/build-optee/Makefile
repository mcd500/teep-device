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
SUIT_PRIV_KEY = ../../key/tc-provider-priv.pem
TC_URI = $(TAM_URL)/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
TC_BINARY = $(BUILD)/hello-tc/8d82573a-926d-4754-9353-32dc29997f74.ta

include ../manifest/manifest.mk
