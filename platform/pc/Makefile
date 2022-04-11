export BUILD ?= $(CURDIR)/build
export SOURCE ?= $(CURDIR)/../..

include ../conf-pc.mk

all: libteep teep-broker-app manifest scripts

include ../common.mk

clean:
	rm -rf $(BUILD)

TAM_URL ?= http://tamproto_tam_api_1:8888
BROKER = ./build/teep-broker-app/teep-broker-app

.PHONY: scripts

scripts:
	mkdir -p $(BUILD)/scripts
	for f in dota dsta dta iota ista ita; do \
		install $(SOURCE)/scripts/$$f.sh $(BUILD)/scripts; \
		sed -i'' -e 's/3000/8888/' $(BUILD)/scripts/$$f.sh; \
		sed -i'' -e 's@teep-broker-app@$(BUILD)/teep-broker-app/teep-broker-app@' $(BUILD)/scripts/$$f.sh; \
	done

MANIFEST_OUT_DIR = $(BUILD)
MANIFEST_DIR = $(SOURCE)/hello-tc/manifest
SUIT_PRIV_KEY = $(SOURCE)/key/tc-provider-priv.pem
TC_URI = $(TAM_URL)/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
TC_BINARY = dummy.ta

include $(SOURCE)/hello-tc/manifest/manifest.mk

test: upload-embed-manifest
	$(BROKER) --tamurl $(TAM_URL)/api/tam_cbor --talist $(HELLO_TA_UUID_2) 2>&1 | tee $(BUILD)/pctest.log
	! fgrep 'ERR:' $(BUILD)/pctest.log
	fgrep 'store component' $(BUILD)/pctest.log

