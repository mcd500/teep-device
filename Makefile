# the PEM certificate for the TAM root of trust
#
TAM_PUB_JWK=test-jw/tsm/identity/tam-mytam-public.jwk
TAM_PRIV_JWK=test-jw/tsm/identity/private/tam-mytam-private.jwk
TEE_PUB_JWK=test-jw/tee/identity/tee-mytee-public.jwk
TEE_PRIV_JWK=test-jw/tee/identity/private/tee-mytee-private.jwk
SP_PUB_JWK=test-jw/tee/sds/xbank/spaik-pub.jwk
SP_PRIV_JWK=test-jw/tee/sds/xbank/spaik-priv.jwk

FIXED_TAM_PUB_JWK  = key/test-jw_tsm_identity_tam-mytam-public.jwk
FIXED_TAM_PRIV_JWK = key/test-jw_tsm_identity_private_tam-mytam-private.jwk
FIXED_TEE_PUB_JWK  = key/test-jw_tee_identity_tee-mytee-public.jwk
FIXED_TEE_PRIV_JWK = key/test-jw_tee_identity_private_tee-mytee-private.jwk
FIXED_SP_PUB_JWK   = key/test-jw_tee_sds_xbank_spaik-pub.jwk
FIXED_SP_PRIV_JWK  = key/test-jw_tee_sds_xbank_spaik-priv.jwk

TEEP_KEYS= $(TAM_PRIV_JWK) $(TAM_PUB_JWK) $(TEE_PRIV_JWK) $(TEE_PUB_JWK) $(SP_PRIV_JWK) $(SP_PUB_JWK)

TEEP_KEY_SRCS := teep-agent-ta/tam_id_pubkey_jwk.h \
                teep-agent-ta/tee_id_privkey_jwk.h \
                teep-agent-ta/tee_id_pubkey_jwk.h \
                teep-agent-ta/sp_pubkey_jwk.h

.PHONY: all all- all-optee all-keystone
.PHONY: clean clean-optee clean-keystone
.PHONY: test test-optee test-keystone

all: all-$(TEE)

all-:
	@echo '$$TEE must be "optee" or "keystone"'
	@false

all-optee: build-optee

all-keystone: build-keystone

clean: clean-optee clean-keystone

clean-optee:
	$(MAKE) -C platform/op-tee clean

clean-keystone:
	$(MAKE) -C platform/keystone clean

.PHONY: build-optee build-keystone

build-optee:
	$(MAKE) -C platform/op-tee install_qemu

build-keystone:
	$(MAKE) -C platform/keystone image

test: test-$(TEE)

test-optee:
	$(MAKE) -C platform/op-tee test

test-keystone:
	$(MAKE) -C platform/keystone test


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

.PHONY: distclean
distclean: clean
	rm -fr sample-senario/node_modules/ sample-senario/package-lock.json
	rm -fr test-jw
	rm -f $(TEEP_KEY_SRCS)
