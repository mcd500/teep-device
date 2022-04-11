export BUILD ?= $(CURDIR)/build
export SOURCE ?= $(CURDIR)/../..

include ../conf-sgx.mk

all: libteep hello-tc teep

include ../common.mk

clean: clean-hello-tc
	rm -rf $(BUILD)

TAM_URL ?= http://tamproto_tam_api_1:8888

test: upload-hello-tc-download
	cd $(BUILD)/teep-agent-ta && \
		../teep-broker-app/teep-broker-app --tamurl $(TAM_URL)/api/tam_cbor
