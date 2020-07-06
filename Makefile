# the PEM certificate for the TAM root of trust
#
TAM_PUB_JWK=test-jw/tsm/identity/tam-mytam-public.jwk
TAM_PRIV_JWK=test-jw/tsm/identity/private/tam-mytam-private.jwk
TEE_PUB_JWK=test-jw/tee/identity/tee-mytee-public.jwk
TEE_PRIV_JWK=test-jw/tee/identity/private/tee-mytee-private.jwk
SP_PUB_JWK=test-jw/tee/sds/xbank/spaik-pub.jwk
SP_PRIV_JWK=test-jw/tee/sds/xbank/spaik-priv.jwk

TEEP_KEYS= $(TAM_PRIV_JWK) $(TAM_PUB_JWK) $(TEE_PRIV_JWK) $(TEE_PUB_JWK) $(SP_PRIV_JWK) $(SP_PUB_JWK)

TEEP_KEY_SRCS := teep-agent-ta/tam_id_pubkey_jwk.h \
                teep-agent-ta/tee_id_privkey_jwk.h \
                teep-agent-ta/tee_id_pubkey_jwk.h \
                teep-agent-ta/sp_pubkey_jwk.h

LIBTEEP_DIR := $(CURDIR)/libteep
OPTEE_DIR ?= build-optee
TEEC_BIN_DIR ?= $(OPTEE_DIR)/out-br/target/usr/sbin

HELLO_TA_UUID  ?= 8d82573a-926d-4754-9353-32dc29997f74
TEE_AGENT_UUID ?= 68373894-5bb3-403c-9eec-3114a1f5d3fc
export HELLO_TA_UUID
export TEE_AGENT_UUID

export LWS_M_BDIR = build-lws-mbed
export MBEDTLS_BDIR = build-mbedtls

.PHONY: all
ifneq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk),)
all: aist-teep
else
all:
	@echo "$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk does not exist. Is TA_DEV_KIT_DIR correctly defined?" && false
endif

.PHONY: generate-jwks
generate-jwks $(TEEP_KEYS):
	(cd sample-senario && npm install)
	mkdir -p test-jw/tsm/identity/private
	mkdir -p test-jw/tee/identity/private
	mkdir -p test-jw/tee/sds/xbank
	node ./sample-senario/generate-jwk.js $(TAM_PRIV_JWK) $(TAM_PUB_JWK)
	node ./sample-senario/generate-jwk.js $(TEE_PRIV_JWK) $(TEE_PUB_JWK)
	node ./sample-senario/generate-jwk.js $(SP_PRIV_JWK) $(SP_PUB_JWK)

.PHONY: check-jwks
check-jwks: $(TEEP_KEYS)
	(cd sample-senario && npm install)
	node ./sample-senario/check-jwk.js $(TAM_PRIV_JWK) $(TAM_PUB_JWK)
	node ./sample-senario/check-jwk.js $(TEE_PRIV_JWK) $(TEE_PUB_JWK)
	node ./sample-senario/check-jwk.js $(SP_PRIV_JWK) $(SP_PUB_JWK)

.PHONY: generate-jwk-headers
generate-jwk-headers $(TEEP_KEY_SRCS): $(TAM_PUB_JWK) $(SP_PUB_JWK) $(TEE_PUB_JWK) $(TEE_PRIV_JWK)
	cat $(TAM_PUB_JWK) | sed 's/\"/\\\"/g' | sed 's/^/\"/g' | sed 's/$$/\\\n\"\n/g' > \
               teep-agent-ta/tam_id_pubkey_jwk.h
	cat $(SP_PUB_JWK) | sed 's/\"/\\\"/g' | sed 's/^/\"/g' | sed 's/$$/\\\n\"\n/g' > \
               teep-agent-ta/sp_pubkey_jwk.h
	cat $(TEE_PUB_JWK) | sed 's/\"/\\\"/g' | sed 's/^/\"/g' | sed 's/$$/\\\n\"\n/g' > \
               teep-agent-ta/tee_id_pubkey_jwk.h
	cat $(TEE_PRIV_JWK) | sed 's/\"/\\\"/g' | sed 's/^/\"/g' | sed 's/$$/\\\n\"\n/g' > \
               teep-agent-ta/tee_id_privkey_jwk.h

.PHONY: libteep
libteep:
	make -C libteep TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) \
		INCLUDES="$(INCLUDES)"

OPTEE_OS ?= $(PWD)/build-optee/optee_os
ARM_PLAT ?= arm

.PHONY: teep-agent-ta
teep-agent-ta: $(TEEP_KEY_SRCS) libteep
	make -C teep-agent-ta CROSS_COMPILE_HOST=$(CROSS_COMPILE) \
		TEE_AGENT_UUID=$(TEE_AGENT_UUID) \
		CROSS_COMPILE_TA=aarch64-linux-gnu- \
		TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) \
		LIBTEEP_DIR=$(LIBTEEP_DIR) \
		CFG_MSG_LONG_PREFIX_THRESHOLD=3 \
		CMAKE_C_FLAGS=-Wno-deprecated-declarations \
		OPTEE_OS=$(OPTEE_OS) \
		LDADD="$(OPTEE_OS)/out/$(ARM_PLAT)/core-lib/libmbedtls/mbedtls/library/gcm.o -L$(TA_DEV_KIT_DIR)/lib -lutils -lutee -L../libteep/build-mbedtls/library -L../libteep/build-lws-tee/lib -lwebsockets $(OPTEE_OS)/out/$(ARM_PLAT)/core-lib/libmbedtls/libmbedtls.a " \
		INCLUDES="$(INCLUDES)" \
		V=1 VERBOSE=1 all

.PHONY: hello-ta
hello-ta:
	make -C hello-ta CROSS_COMPILE_HOST=$(CROSS_COMPILE) \
		CROSS_COMPILE_TA=aarch64-linux-gnu- \
		TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) \
		CFG_MSG_LONG_PREFIX_THRESHOLD=3 \
		V=0 VERBOSE=0 all
	(cd sample-senario && npm install)
	node ./sample-senario/sign-then-enc.js $(SP_PRIV_JWK) $(TEE_PUB_JWK)\
		 $(CURDIR)/hello-ta/$(HELLO_TA_UUID).ta
	cp $(CURDIR)/hello-ta/$(HELLO_TA_UUID).ta* $(CURDIR)/tiny-tam/TAs/

.PHONY: hello-ta-keystone
hello-ta-keystone:
	make -C hello-ta -f keystone.mk all

.PHONY: hello-app
hello-app:
	make -C hello-app TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) INCLUDES="$(INCLUDES)" TARGET=$(TARGET)

.PHONY: hello-app-keystone
hello-app-keystone:
	make -C hello-app keystone

.PHONY: aist-teep
aist-teep: $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk $(TEEC_BIN_DIR)/tee-supplicant teep-agent-ta hello-ta
	make -C teep-broker-app TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE)
	make -C hello-app TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) INCLUDES="$(INCLUDES)"

.PHONY: clean
clean:
	make -C libteep TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C teep-broker-app TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C teep-agent-ta TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C hello-app TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C hello-ta TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C pctest clean
	rm -f $(TEEP_KEY_SRCS)

.PHONY: clean-ta
clean-ta:
	make -C teep-agent-ta TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C hello-ta TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean

.PHONY: clean-hello-ta
clean-hello-ta:
	make -C hello-ta TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C hello-app TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean

.PHONY: distclean
distclean:
	rm -fr sample-senario/node_modules/ sample-senario/package-lock.json
	rm -fr test-jw
