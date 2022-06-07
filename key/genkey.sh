#!/bin/bash

set -ex

# P-256 = secp256r1 = prime256v1
# P-384 = secp384r1 = ansip384r1
# P-521 = secp521r1 = ansip521r1

EC=P256
SECP=secp256r1

# generate teep keys

for p in tc-signer tam-teep device-teep; do
    openssl ecparam -name $SECP -genkey -noout -out $p-$EC-priv.pem
    openssl ec -in $p-$EC-priv.pem -pubout -out $p-$EC-pub.pem
done

# PKI stuffs

for p in rootca tc-signer-ca tam-teep-ca device-teep-ca; do
    mkdir -p CAs/$p
    openssl ecparam -name $SECP -genkey -noout -out CAs/$p/$EC-priv.pem
done

C="/C=JP/ST=Kantou/L=Tokyo/O=AIST/OU=AIST teep"

for p in rootca; do
    openssl req -x509 -new -days +3650 -sha512 \
        -key CAs/$p/$EC-priv.pem \
        -subj "$C/CN=teep sample rootca" \
        -out CAs/$p/$EC-cert.pem
done

for p in tc-signer-ca tam-teep-ca device-teep-ca; do
    openssl req -new -sha512 \
        -key CAs/$p/$EC-priv.pem \
        -subj "$C/CN=teep sample $p" \
        -out CAs/$p/$EC-cert.csr
    openssl x509 -req -days +3650 -sha512 \
        -in CAs/$p/$EC-cert.csr \
        -extfile ossl.conf \
        -CA CAs/rootca/$EC-cert.pem \
        -CAkey CAs/rootca/$EC-priv.pem \
        -CAcreateserial \
        -out CAs/$p/$EC-cert.pem
done

# sign teep keys

for p in tc-signer tam-teep device-teep; do
    openssl req -new -sha512 \
        -key $p-$EC-priv.pem \
        -subj "$C/CN=teep sample $p" \
        -out $p-$EC.csr
    openssl x509 -req -days +3650 -sha512 \
        -in $p-$EC.csr \
        -CA CAs/$p-ca/$EC-cert.pem \
        -CAkey CAs/$p-ca/$EC-priv.pem \
        -CAcreateserial \
        -out $p-$EC-cert.pem
done
