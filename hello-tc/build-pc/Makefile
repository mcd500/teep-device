TOPDIR = $(CURDIR)/../..
include $(TOPDIR)/conf.mk

.PHONY: all
all: dir manifest

.PHONY: dir
dir:
	mkdir -p $(BUILD)/hello-tc

.PHONY: clean
clean: clean-manifest

MANIFEST_OUT_DIR = $(BUILD)/hello-tc
MANIFEST_DIR = ../manifest
SUIT_PRIV_KEY = $(TOPDIR)/key/tc-signer-P256-priv.pem
TC_URI = $(TAM_URL)/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
TC_BINARY = dummy.ta

include ../manifest/manifest.mk
