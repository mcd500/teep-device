#!/bin/bash

# For a real situation, this should be done on a machine with no network
# connection with the cwd a tmpfs.  After producing the CAs the keys in
# ./pki/*/*-rootca should be moved to secure storage (eg, USB stick in
# a safe) and deleted from the tmpfs.  Then the remaining files can be
# copied into various places and the machine rebooted or shut down.

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

# trusting the rootca means we can verify the ca signed by it
openssl verify -no-CApath -trusted pki/sp/sp-rootca/sp-rootca-ec-cert.pem \
		pki/sp/sp-ca/sp-ca-1-ec-cert.pem
if [ $? -ne 0 ]; then
	exit 1
fi

# the intermediate cert is added to the signed certifcate file in
# ...-cert-plus-intermediate-cert.pem
# so we only need to trust the rootca to confirm the chain if we
# use the cert chain file.
openssl verify -no-CApath -trusted pki/sp/sp-rootca/sp-rootca-ec-cert.pem \
		pki/sp/sp/sp-xbank-rsa-cert-plus-intermediate-cert.pem
if [ $? -ne 0 ]; then
	exit 1
fi

echo "OK"

exit 0

