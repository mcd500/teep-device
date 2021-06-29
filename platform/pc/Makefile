export BUILD ?= $(CURDIR)/build
export SOURCE ?= $(CURDIR)/../..
export PLAT = pc

export HELLO_TA_UUID  ?= 8d82573a-926d-4754-9353-32dc29997f74
export HELLO_TA_UUID_2 ?= 8d82573a-926d-4754-9353-32dc29997f75
export TEE_AGENT_UUID ?= 68373894-5bb3-403c-9eec-3114a1f5d3fc

export APP_CFLAGS = -g \
	-I./ \
	-I$(SOURCE)/include \
	-I$(SOURCE)/libteep/lib \
	-I$(BUILD)/libteep/host/libwebsockets/include \
	-I$(SOURCE)/libteep/libwebsockets/include \
	-I$(SOURCE)/libteep/mbedtls/include \
	-I$(BUILD)/libteep/host/QCBOR/inc \
	-I$(SOURCE)/suit/include \
	-I$(CURDIR)/include \
	-I$(SOURCE)/key/include \
	-DPCTEST -DPLAT_PC -DAPP_VERBOSE -Wall

export APP_LDFLAGS = \
	-L$(BUILD)/libteep/host/mbedtls/library \
	-L$(BUILD)/libteep/host/libwebsockets/lib \
	-L$(BUILD)/libteep/host/QCBOR \
	-L$(BUILD)/libteep/tee/libteesuit/lib

export APP_LIBS = \
	 -lwebsockets -lmbedtls -lmbedx509 -lmbedcrypto -lqcbor -lteesuit

ifeq ($(shell uname),Linux)
APP_LIBS += -lcap
endif

libteep-mbedtls-host-FLAGS = \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DUSE_STATIC_MBEDTLS_LIBRARY=1 \
	-DUSE_SAHTED_MBEDTLS_LIBRARY=0 \

libteep-libwebsockets-host-FLAGS = \
	-DLWS_WITH_SSL=1 \
	-DLWS_WITH_MBEDTLS=1 \
	-DLWS_WITH_JOSE=1 \
	-DLWS_MBEDTLS_INCLUDE_DIRS=../mbedtls/include \
	-DMBEDTLS_LIBRARY=$(BUILD)/libteep/host/mbedtls/library/libmbedtls.a \
	-DMBEDX509_LIBRARY=$(BUILD)/libteep/host/mbedtls/library/libmbedx509.a \
	-DMBEDCRYPTO_LIBRARY=$(BUILD)/libteep/host/mbedtls/library/libmbedcrypto.a \
	-DLWS_WITH_MINIMAL_EXAMPLES=1 \
	-DLWS_STATIC_PIC=ON \
	-DLWS_WITH_SHARED=OFF \
	-DLWS_WITH_STATIC=1 \
	-DLWS_WITHOUT_SERVER=1 \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_C_FLAGS='-Wno-enum-conversion'

libteep-mbedtls-ree-DISABLE = y
libteep-libwebsockets-ree-DISABLE = y
libteep-QCBOR-ree-DISABLE = y

libteep-mbedtls-tee-DISABLE = y
libteep-libwebsockets-tee-DISABLE = y
libteep-QCBOR-tee-DISABLE = y

libteep-libteep-tee-DISABLE = y

all: libteep teep-broker-app scripts

include ../common.mk

clean:
	rm -rf $(BUILD)

TAM_URL ?= http://localhost:8888
BROKER = ./build/teep-broker-app/teep-broker-app

.PHONY: scripts

scripts:
	mkdir -p $(BUILD)/scripts
	for f in dota dsta dta iota ista ita; do \
		install $(SOURCE)/scripts/$$f.sh $(BUILD)/scripts; \
		sed -i'' -e 's/3000/8888/' $(BUILD)/scripts/$$f.sh; \
		sed -i'' -e 's@teep-broker-app@$(BUILD)/teep-broker-app/teep-broker-app@' $(BUILD)/scripts/$$f.sh; \
	done

upload_encfile_to_tamproto:
	echo 'hello, world!' >$(BUILD)/$(HELLO_TA_UUID).ta
	cd $(SOURCE)/scripts/keygen && npm install
	node $(SOURCE)/scripts/keygen/sign-then-enc.js \
		$(SOURCE)/key/test-jw_tee_sds_xbank_spaik-priv.jwk \
		$(SOURCE)/key/test-jw_tee_identity_tee-mytee-public.jwk \
		$(BUILD)/$(HELLO_TA_UUID).ta
	curl $(TAM_URL)/api/
	curl $(TAM_URL)/panel/upload \
		-F "file=@$(BUILD)/$(HELLO_TA_UUID).ta.sign.enc;filename=$(HELLO_TA_UUID).ta"
	sed "s@http://localhost:8888@$(TAM_URL)@" manifest/hello-ta.json >$(BUILD)/hello-ta.json
	suit-tool create -i $(BUILD)/hello-ta.json -o $(BUILD)/hello-ta.suit
	suit-tool sign -m $(BUILD)/hello-ta.suit -k ../../key/tc-provider-priv.pem -o $(BUILD)/signed-hello-ta.suit
	#curl $(TAM_URL)/panel/upload \
	#	-F "file=@$(BUILD)/signed-hello-ta.suit;filename=suit_manifest_exp1.cbor"
	curl --location --request POST '$(TAM_URL)/panel/upload_manifest_and_TC' \
		--form 'uuid="$(HELLO_TA_UUID_2)"' \
		--form 'file=@"$(BUILD)/signed-hello-ta.suit";filename=$(HELLO_TA_UUID_2).cbor' \
		--form 'file=@"$(BUILD)/$(HELLO_TA_UUID).ta.sign.enc";filename=$(HELLO_TA_UUID_2).ta'


test: upload_encfile_to_tamproto
	$(BROKER) --tamurl $(TAM_URL)/api/tam_cbor --talist $(HELLO_TA_UUID_2) 2>&1 | tee $(BUILD)/pctest.log
	! fgrep 'ERR:' $(BUILD)/pctest.log
	fgrep 'ta_store_install: stub called' $(BUILD)/pctest.log

