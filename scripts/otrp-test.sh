#!/bin/bash
#set -x

if [ -z "$LWS_BUILD_DIR" ] ; then
	LWS_BUILD_DIR=/projects/libwebsockets/build-mtls
fi

# create a 50KiB fake "TA"

dd if=/dev/urandom of=test-jw/tsm/sds/xbank/plaintext-ta bs=1024 count=50 2>/dev/null
ORIG_TA_SHA1=`sha1sum test-jw/tsm/sds/xbank/plaintext-ta | cut -d' ' -f1`

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
if [ $? -ne 0 ] ; then
	echo "JWE decrypt failed"
	exit 1
fi
DECRYPTED_TA_SHA1=`sha1sum test-jw/enc-ta.decrypted | cut -d' ' -f1`

if [ "$ORIG_TA_SHA1" = "$DECRYPTED_TA_SHA1" ] ; then
	echo "Decrypted TA matches original"
	exit 0
fi

echo "Decrypted TA doesn't match original $ORIG_TA_SHA1 vs $DECRYPTED_TA_SHA1"

exit 1

