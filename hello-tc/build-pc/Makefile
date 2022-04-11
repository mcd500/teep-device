TOPDIR = $(CURDIR)/../..
include $(TOPDIR)/conf.mk

all: dir manifest

dir:
	mkdir -p $(BUILD)/hello-tc

clean: clean-manifest

TAM_URL ?= http://tamproto_tam_api_1:8888

MANIFEST_OUT_DIR = $(BUILD)/hello-tc
MANIFEST_DIR = ../manifest
SUIT_PRIV_KEY = ../../key/tc-provider-priv.pem
TC_URI = $(TAM_URL)/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
TC_BINARY = dummy.ta

include ../manifest/manifest.mk
