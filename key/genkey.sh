#!/bin/bash

set -ex

# P-256 = secp256r1 = prime256v1
# P-384 = secp384r1 = ansip384r1
# P-521 = secp521r1 = ansip521r1

openssl ecparam -name prime256v1 -genkey -noout -out tc-developper-P256-priv.pem
openssl ec -in tc-developper-P256-priv.pem -pubout -out tc-developper-P256-pub.pem
openssl ec -in tc-developper-P256-priv.pem -text -out tc-developper-P256-priv.txt
