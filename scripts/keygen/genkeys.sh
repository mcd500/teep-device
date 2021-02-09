#!/bin/sh

set -ex

SCRIPTS_DIR=scripts/keygen/

TAM_PUB_JWK=key/test-jw_tsm_identity_tam-mytam-public.jwk
TAM_PRIV_JWK=key/test-jw_tsm_identity_private_tam-mytam-private.jwk
TEE_PUB_JWK=key/test-jw_tee_identity_tee-mytee-public.jwk
TEE_PRIV_JWK=key/test-jw_tee_identity_private_tee-mytee-private.jwk
SP_PUB_JWK=key/test-jw_tee_sds_xbank_spaik-pub.jwk
SP_PRIV_JWK=key/test-jw_tee_sds_xbank_spaik-priv.jwk

npm install

node ${SCRIPTS_DIR}/generate-jwk.js $TAM_PRIV_JWK $TAM_PUB_JWK
node ${SCRIPTS_DIR}/generate-jwk.js $TEE_PRIV_JWK $TEE_PUB_JWK
node ${SCRIPTS_DIR}/generate-jwk.js $SP_PRIV_JWK $SP_PUB_JWK

node ${SCRIPTS_DIR}/check-jwk.js $TAM_PRIV_JWK $TAM_PUB_JWK
node ${SCRIPTS_DIR}/check-jwk.js $TEE_PRIV_JWK $TEE_PUB_JWK
node ${SCRIPTS_DIR}/check-jwk.js $SP_PRIV_JWK $SP_PUB_JWK
