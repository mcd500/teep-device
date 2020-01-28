# the PEM certificate for the TAM root of trust
#
TAM_ROOT_CERT=pki/tam/tam-rootca/tam-rootca-ec-cert.pem
TAM_ID_PUBKEY_JWK=test-jw/tsm/identity/tam-mytam-public.jwk
TEE_PRIVKEY_JWK=test-jw/tee/sds/xbank/spaik-priv.jwk

TEEP_KEYS=$(TAM_ROOT_CERT) $(TAM_ID_PUBKEY_JWK) $(TEE_PRIVKEY_JWK)

TEEP_KEY_SRCS=teep-agent-ta/tam_root_cert.h \
              teep-agent-ta/tam_id_pubkey_jwk.h \
              teep-agent-ta/tee_privkey_jwk.h

.PHONY: all
ifneq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk),)
all: aist-teep
else
all:
	@echo "$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk does not exist. Is TA_DEV_KIT_DIR correctly defined?" && false
endif

.PHONY: convert_teep_keys
convert_teep_keys $(TEEP_KEY_SRCS): $(TEEP_KEYS)
	cat $(TAM_ROOT_CERT) | sed 's/^/\"/g' | sed 's/$$/\\\n\"/g' > \
		teep-agent-ta/tam_root_cert.h
	cat $(TAM_ID_PUBKEY_JWK) | sed 's/\"/\\\"/g' | sed 's/^/\"/g' | sed 's/$$/\\\n\"/g' > \
		teep-agent-ta/tam_id_pubkey_jwk.h
	cat $(TEE_PRIVKEY_JWK) | sed 's/\"/\\\"/g' | sed 's/^/\"/g' | sed 's/$$/\\\n\"/g' > \
		teep-agent-ta/tee_privkey_jwk.h

.PHONY: teep-agent-ta
teep-agent-ta: $(TEEP_KEY_SRCS)
	make -C teep-agent-ta CROSS_COMPILE_HOST=$(CROSS_COMPILE) \
		CROSS_COMPILE_TA=aarch64-linux-gnu- \
		TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) \
		CFG_MSG_LONG_PREFIX_THRESHOLD=3 \
		CMAKE_C_FLAGS=-Wno-deprecated-declarations \
		LDADD="-L$(TA_DEV_KIT_DIR)/lib -lutils -lutee -lmbedtls -lwebsockets" \
		V=1 VERBOSE=1 all

.PHONY: sp-hello-ta
sp-hello-ta:
	make -C sp-hello-ta CROSS_COMPILE_HOST=$(CROSS_COMPILE) \
		CROSS_COMPILE_TA=aarch64-linux-gnu- \
		TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) \
		CFG_MSG_LONG_PREFIX_THRESHOLD=3 \
		V=0 VERBOSE=0 all

.PHONY: aist-teep
aist-teep: $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk ../optee_client/out/export/bin/tee-supplicant teep-agent-ta sp-hello-ta
	make -C libteep TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE)
	make -C teep-broker-app TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE)
	make -C sp-hello-app TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE)

.PHONY: clean
clean:
	make -C libteep TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C teep-broker-app TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C teep-agent-ta TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C sp-hello-app TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C sp-hello-ta TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	rm -f $(TEEP_KEY_SRCS)

.PHONY: clean-ta
clean-ta:
	make -C teep-agent-ta TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C sp-hello-ta TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
