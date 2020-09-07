#!/bin/sh

set -ex

TAM_PUB_JWK=key/test-jw_tsm_identity_tam-mytam-public.jwk
TAM_PRIV_JWK=key/test-jw_tsm_identity_private_tam-mytam-private.jwk
TEE_PUB_JWK=key/test-jw_tee_identity_tee-mytee-public.jwk
TEE_PRIV_JWK=key/test-jw_tee_identity_private_tee-mytee-private.jwk
SP_PUB_JWK=key/test-jw_tee_sds_xbank_spaik-pub.jwk
SP_PRIV_JWK=key/test-jw_tee_sds_xbank_spaik-priv.jwk

npm install

mkdir -p key/include

node ./generate-jwk.js $TAM_PRIV_JWK $TAM_PUB_JWK
node ./generate-jwk.js $TEE_PRIV_JWK $TEE_PUB_JWK
node ./generate-jwk.js $SP_PRIV_JWK $SP_PUB_JWK

node ./check-jwk.js $TAM_PRIV_JWK $TAM_PUB_JWK
node ./check-jwk.js $TEE_PRIV_JWK $TEE_PUB_JWK
node ./check-jwk.js $SP_PRIV_JWK $SP_PUB_JWK

cat $TAM_PUB_JWK | sed 's/\"/\\\"/g' | sed 's/^/\"/g' | sed 's/$$/\\\n\"\n/g' > \
            key/include/tam_id_pubkey_jwk.h
cat $SP_PUB_JWK | sed 's/\"/\\\"/g' | sed 's/^/\"/g' | sed 's/$$/\\\n\"\n/g' > \
            key/include/sp_pubkey_jwk.h
cat $TEE_PUB_JWK | sed 's/\"/\\\"/g' | sed 's/^/\"/g' | sed 's/$$/\\\n\"\n/g' > \
            key/include/tee_id_pubkey_jwk.h
cat $TEE_PRIV_JWK | sed 's/\"/\\\"/g' | sed 's/^/\"/g' | sed 's/$$/\\\n\"\n/g' > \
            key/include/tee_id_privkey_jwk.h
