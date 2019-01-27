# Notes on PKI config

## One-time PKI setup

You can configure the root certs, intermediate CA certs and
some selected demo certs signed by the intermediate CA certs
by running this script.

It also converts some PEM certs and keys to JWK for later tests (and hence needs the path
to the libwebsockets conversion tools)

```
$ LWS_BUILD_DIR=/path/to/libwebsockets/build ./scripts/otrp-kickstart-pki.sh
```

## PKI layout

There are three cerificate chains laid out the same way, for
"Service Provider" (SP), "Trusted Application Manager" (TAM) and
"Trusted Execution Environment" (TEE).

For SP as an example:

```
 ./pki
  sp
   sp-rootca
    sp-rootca-ec-key.pem
    sp-rootca-ec-cert.pem
   sp-ca
    sp-ca-1-ec-key.pem
    sp-ca-1-ec-cert.pem
   sp
    sp-ybank-rsa-key.pem
    sp-ybank-rsa-cert.pem
    sp-ybank-rsa-cert-plus-intermediate-cert.pem
    sp-xbank-rsa-cert.pem
    sp-xbank-rsa-key.pem
    sp-xbank-rsa-cert-plus-intermediate-cert.pem
```

The pki for TAM (./pki/tam...) and TEE (./pki/tee/...) is the same
except signed certificates are produced for "mytam" and "mytee"
instead of the fictional bank SPs.

Root keys are produced for both rsa and ec, and which to use can
be selected when creating the intermediate CAs.  The kickstart
script uses ec keys for the rootca to sign the intermediate CA,
and 4096-bit RSA keys for the intermediate CA to sign the
final certificates.

For the final signed certs, the normal certificate with the
public key is produced, but also a `...-cert-plus-intermediate-cert.pem`
bundle that contains both the normal signed cert and a copy of the
intermediate CA cert.

## Converting and using JWK/E/S manually

A test script is provided which uses the PKI infrastructure to
encrypt, sign and "send" a "TA" from the "TAM side" to the "TEE
side", where it's verified for signature and decrypted and confirmed
to be unchanged from the original.

This flow has important deviations from OTrP...

 - it runs on a PC not the TEE and TAM server
 - it doesn't do any communication
 - the JWE and JWS are stock RFC without OTrP extensions,
   it doesn't attempt to make correctly formed OTrP
   request or response packets just standard JWE / JWS
 - it deals with 50KiB of random instead of a real TA and
   doesn't try to use the TA in a TEE
 - there are no test vectors available, so it decrypts its
   own encryption and verifies its own signatures.  So there
   are no guarantees the crypto flow is interoperable yet.

... however...

 - it's using exactly the same libwebsockets JW\* code as the TEE
 - it's using exactly the same generic crypto code as the TEE
 - it's using exactly the same mbedtls crypto backend as the TEE
 - it's using real JWS and JWS crypto agility alg / enc
 - it's using the actual ECDH and RSA / AES crypto as used by OTrP
 - it's using the actual PKI infrastructure with the
   actual EC and RSA root CAs, intermediate CAs and
   certs for TAM and TEE
 - it's using real cert keys converted to JWK on the fly for the
   correct crypto flow

... so it proves a small but significant part of one flow around
the certs and crypto.

### Step 1: build libwebsockets with mbedtls

Build libwebsockets on your build machine and to build the minimal examples.

You should install your either your distro mbedtls first (-DLWS_WITH_MBEDTLS=1)

Distro|Package
---|---
Fedora|mbedtls-devel
Ubuntu|libmbedtls-dev

or distro OpenSSL (-DLWS_WITH_MBEDTLS=0)

Distro|Package
---|---
Fedora|openssl-devel
Ubuntu|libssl-dev

```
$ git clone https://libwebsockets.org/repo/libwebsockets
$ cd libwebsockets
$ mkdir build
$ cd build
$ cmake .. -DLWS_WITH_JOSE=1 -DLWS_WITH_MBEDTLS=1 -DLWS_WITH_MINIMAL_EXAMPLES=1
$ make -j12 && sudo make install
$ sudo ldconfig
```

This will build but not install several dozen example applications in
./build/bin, which are able to perform operations from the commandline on
x.509 PEM, JWKs, JWS (signing) and JWE (en/decrypt).  These can be used
to synthesize the core operations around, eg, encrypting and signing a TA
and verifying and decrypting it, using the PKI created above.

### Step 2: run the pki kickstart script

```
$ LWS_BUILD_DIR=/path/to/libwebsockets/build ./scripts/otrp-kickstart-pki.sh
```

This recreates all the PKI, root certs and keys etc... running this again
will mean you will have to rebuild everything and update fake-tam copies of
the crypto etc using the new pki.

### Step 3: run the test script

Run the test script like this

```
$ LWS_BUILD_DIR=/path/to/libwebsockets/build ./scripts/otrp-test.sh
```

You should see output like this

```
[2019/01/03 19:13:10:0065] USER: LWS X509 api example
[2019/01/03 19:13:10:0067] NOTICE: lws_x509_public_to_jwk: RSA key
[2019/01/03 19:13:10:0123] NOTICE: Issuing Cert + Private JWK on stdout
[2019/01/03 19:13:10:0123] NOTICE: main: OK
[2019/01/03 19:13:10:0135] USER: LWS X509 api example
[2019/01/03 19:13:10:0137] NOTICE: lws_x509_public_to_jwk: RSA key
[2019/01/03 19:13:10:0137] NOTICE: Issuing Cert Public JWK on stdout
[2019/01/03 19:13:10:0138] NOTICE: main: OK
[2019/01/03 19:13:10:0243] USER: LWS JWK example
[2019/01/03 19:13:10:0244] NOTICE: lws_jwk_generate: generating 4096 bit RSA key
[2019/01/03 19:13:10:6649] USER: LWS JWE example tool
[2019/01/03 19:13:10:6703] USER: LWS JWS example tool
[2019/01/03 19:13:10:6942] USER: LWS JWS example tool
[2019/01/03 19:13:10:6989] NOTICE: VALID
[2019/01/03 19:13:10:7002] USER: LWS JWE example tool
Decrypted TA matches original
```

Running `otrp-test.sh` doesn't change the pki, so you can run it as
many times as you like.
