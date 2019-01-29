#!/bin/bash

# For a real situation, this should be done on a machine with no network
# connection with the cwd a tmpfs.  After producing the CAs the keys in
# ./pki/*/*-rootca should be moved to secure storage (eg, USB stick in
# a safe) and deleted from the tmpfs.  Then the remaining files can be
# copied into various places and the machine rebooted or shut down.

if [ -z "$LWS_BUILD_DIR" ] ; then
	LWS_BUILD_DIR=/projects/libwebsockets/build-mtls
fi

#SPAIK_KEYTYPE="-t EC -v P-521"
SPAIK_KEYTYPE="-t RSA -b 4096"

rm -rf pki
SPATH="`dirname \"$0\"`"

# generate the root certs + intermediate CA for everything

for i in sp tam tee ; do
	# create the root certs
	$SPATH/otrp-generate-pki.sh rootca $i ./pki/$i/$i-rootca
	if [ $? -ne 0 ]; then
		exit 1
	fi
	# create the initial intermediate CAs
	echo "Generating $i intermediate CA"
	$SPATH/otrp-generate-pki.sh ca ./pki/$i/$i-rootca/$i-rootca-ec-cert.pem ./pki/$i/$i-ca 1
	if [ $? -ne 0 ]; then
		exit 1
	fi
done

# the keys in ./pki/*/*-rootca should be moved to secure storage

# create SP certs signed by the SP signing CA (which is signed by the SP root cert)
echo "Creating SP certs for 'xbank' and 'ybank'"
$SPATH/otrp-generate-pki.sh sign ./pki/sp/sp-ca/sp-ca-1-ec-cert.pem ./pki/sp/sp xbank
$SPATH/otrp-generate-pki.sh sign ./pki/sp/sp-ca/sp-ca-1-ec-cert.pem ./pki/sp/sp ybank

# create a TAM cert signed by the TAM signing CA (which is signed by the TAM root cert)
$SPATH/otrp-generate-pki.sh sign ./pki/tam/tam-ca/tam-ca-1-ec-cert.pem ./pki/tam/tam mytam

# create a TEE cert signed by the TEE signing CA (which is signed by the TEE root cert)
$SPATH/otrp-generate-pki.sh sign ./pki/tee/tee-ca/tee-ca-1-ec-cert.pem ./pki/tee/tee mytee

#
# let's confirm the expected relationships
#
# NB these should use -no-CApath to disable OS trust store.
# but older OpenSSL found on eg, Xenial, doesn't support it.

# trusting the rootca means we can verify the ca signed by it
openssl verify -trusted pki/sp/sp-rootca/sp-rootca-ec-cert.pem \
		pki/sp/sp-ca/sp-ca-1-ec-cert.pem
if [ $? -ne 0 ]; then
	exit 1
fi

# the intermediate cert is added to the signed certifcate file in
# ...-cert-plus-intermediate-cert.pem
# so we only need to trust the rootca to confirm the chain if we
# use the cert chain file.
openssl verify -trusted pki/sp/sp-rootca/sp-rootca-ec-cert.pem \
		pki/sp/sp/sp-xbank-rsa-cert-plus-intermediate-cert.pem
if [ $? -ne 0 ]; then
	exit 1
fi


#
# These are the one-time setups for ./scripts/otrp-test.sh
#

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

$LWS_BUILD_DIR/bin/lws-crypto-x509 --alg RSA1_5 -c pki/tam/tam/tam-mytam-rsa-cert.pem \
	-p pki/tam/tam/tam-mytam-rsa-key.pem > test-jw/tsm/identity/private/tam-mytam-private.jwk

# create a public JWK from the TAM identity cert, so we can confirm its signatures

$LWS_BUILD_DIR/bin/lws-crypto-x509 --alg RSA1_5 -c pki/tam/tam/tam-mytam-rsa-cert.pem \
	 > test-jw/tsm/identity/tam-mytam-public.jwk

#######
# populate TEE trust

cp pki/tam/tam-rootca/tam-rootca-ec-cert.pem test-jw/tee/trusted

# populate TEE identity

cp pki/tee/tee/tee-mytee-rsa-cert.pem test-jw/tee/identity
cp pki/tee/tee/tee-mytee-rsa-key.pem test-jw/tee/identity/private

# create xbank SD and SP-AIK keypair

$LWS_BUILD_DIR/bin/lws-crypto-jwk $SPAIK_KEYTYPE --alg RSA1_5 --kid mytee \
	--public test-jw/tee/sds/xbank/spaik-pub.jwk > test-jw/tee/sds/xbank/spaik-priv.jwk
if [ $? -ne 0 ] ; then
	echo "Failed to generate TEE's SD JWK keypair"
	exit 1
fi


echo "OK"

exit 0

