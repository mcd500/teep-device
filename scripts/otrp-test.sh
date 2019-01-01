#!/bin/bash

if [ -z "$LWS_BUILD_DIR" ] ; then
	LWS_BUILD_DIR=/projects/libwebsockets/build-mtls
fi

#SPAIK_KEYTYPE="-t EC -v P-521"
SPAIK_KEYTYPE="-t RSA -b 4096"

rm -rf test-jw

mkdir -p test-jw test-jw/tsm/trusted/sp test-jw/tsm/trusted/tee \
         test-jw/tsm/sds/xbank \
         test-jw/tsm/identity/private \
         test-jw/tee/trusted \
	 test-jw/tee/identity/private \
         test-jw/tee/sds/xbank 

#######
# populate TSM trust

cp pki/sp/sp-rootca/sp-rootca-ec-cert.pem test-jw/tsm/trusted/sp
cp pki/tee/tee-rootca/tee-rootca-ec-cert.pem test-jw/tsm/trusted/tee

# populate TSM TAM identity

cp pki/tam/tam/tam-mytam-rsa-cert-plus-intermediate-cert.pem test-jw/tsm/identity
cp pki/tam/tam/tam-mytam-rsa-key.pem test-jw/tsm/identity/private

# create a private JWK from the TAM identity, so we can JWE-sign with it

$LWS_BUILD_DIR/bin/lws-crypto-x509 -c pki/tam/tam/tam-mytam-rsa-cert.pem \
	-p pki/tam/tam/tam-mytam-rsa-key.pem > test-jw/tsm/identity/private/tam-mytam-private.jwk

# create a public JWK from the TAM identity cert, so we can confirm its signatures

$LWS_BUILD_DIR/bin/lws-crypto-x509 -c pki/tam/tam/tam-mytam-rsa-cert.pem \
	 > test-jw/tsm/identity/tam-mytam-public.jwk

# create a 50KiB fake "TA"

dd if=/dev/urandom of=test-jw/tsm/sds/xbank/plaintext-ta bs=1024 count=50 2>/dev/null
ORIG_TA_SHA1=`sha1sum test-jw/tsm/sds/xbank/plaintext-ta | cut -d' ' -f1`

#######
# populate TEE trust

cp pki/tam/tam-rootca/tam-rootca-ec-cert.pem test-jw/tee/trusted

# populate TEE identity

cp pki/tee/tee/tee-mytee-rsa-cert.pem test-jw/tee/identity
cp pki/tee/tee/tee-mytee-rsa-key.pem test-jw/tee/identity/private

# create xbank SD and SP-AIK keypair

$LWS_BUILD_DIR/bin/lws-crypto-jwk $SPAIK_KEYTYPE --kid mytee \
	--public test-jw/tee/sds/xbank/spaik-pub.jwk > test-jw/tee/sds/xbank/spaik-priv.jwk
if [ $? -ne 0 ] ; then
	echo "Failed to generate TEE's SD JWK keypair"
	exit 1
fi

#######
# TSM: encrypt the "TA" for use by TEE "mytee"
#  it's encrypted using the public part of the recipient SPAIK

cat test-jw/tsm/sds/xbank/plaintext-ta | \
	$LWS_BUILD_DIR/bin/lws-crypto-jwe -k test-jw/tee/sds/xbank/spaik-pub.jwk \
	-e "RSA1_5 A128CBC-HS256" -f > test-jw/enc-ta.jwe
if [ $? -ne 0 ] ; then
	echo "encryption step failed"
	exit 1
fi

# sign the encrypted "TA" using the JWK version of the TAM cert + privkey
# signed with RS256 because the TAM cert key is RSA

cat test-jw/enc-ta.jwe | $LWS_BUILD_DIR/bin/lws-crypto-jws -s "RS256" -f \
	-k test-jw/tsm/identity/private/tam-mytam-private.jwk > test-jw/enc-ta.jwe.jws

# verify the signed, encrypted "TA", using the TAM's public JWK

cat test-jw/enc-ta.jwe.jws | $LWS_BUILD_DIR/bin/lws-crypto-jws -f \
	-k test-jw/tsm/identity/tam-mytam-public.jwk > test-jw/enc-ta.jwe.verified

cat test-jw/enc-ta.jwe.verified | \
	$LWS_BUILD_DIR/bin/lws-crypto-jwe -k test-jw/tee/sds/xbank/spaik-priv.jwk \
		-f > test-jw/enc-ta.decrypted
DECRYPTED_TA_SHA1=`sha1sum test-jw/enc-ta.decrypted | cut -d' ' -f1`

if [ "$ORIG_TA_SHA1" = "$DECRYPTED_TA_SHA1" ] ; then
	echo "Decrypted TA matches original"
	exit 0
fi

echo "Decrypted TA doesn't match original $ORIG_TA_SHA1 vs $DECRYPTED_TA_SHA1"

exit 1

