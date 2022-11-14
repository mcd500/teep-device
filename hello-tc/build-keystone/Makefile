TOPDIR = $(CURDIR)/../..
include $(TOPDIR)/conf.mk

SOURCE = $(CURDIR)/..
out-dir = $(BUILD)/hello-tc

.PHONY: all
all: $(out-dir)/App-keystone $(out-dir)/Enclave manifest

$(out-dir)/App-keystone.o: $(SOURCE)/App-keystone.cpp
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)g++ -c -o $@ $< $(REE_CFLAGS)

$(out-dir)/App-keystone: $(out-dir)/App-keystone.o
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)g++ -o $@ $^ $(REE_LDFLAGS) $(REE_LIBS)

$(out-dir)/Enclave.o: $(SOURCE)/Enclave.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc -c -o $@ $< $(TEE_CFLAGS)

$(out-dir)/Enclave: $(out-dir)/Enclave.o
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)ld -o $@ $^ $(TEE_LDFLAGS) --start-group $(TEE_LIBS) --end-group

.PHONY: clean
clean: clean-manifest
	rm -f $(out-dir)/App-keystone.o $(out-dir)/App-keystone $(out-dir)/Enclave.o $(out-dir)/Enclave

MANIFEST_OUT_DIR = $(out-dir)
MANIFEST_DIR = ../manifest
SUIT_PRIV_KEY = $(TOPDIR)/key/tc-signer-P256-priv.pem
TC_URI = $(TAM_URL)/TAs/$(HELLO_TA_UUID_NAME)
TC_BINARY = $(out-dir)/$(HELLO_TA_UUID_NAME)

include ../manifest/manifest.mk
