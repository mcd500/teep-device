# Setting up the Fake TAM

The "Fake TAM" is a node.js application that listens on port 3000
and serves an encrypted + signed test TA to anyone making requests.

To do that, it needs the following assets (in `/var/www/node` as
currently configured).

|Remote Filename on server|Source|Function|
|---|---|---|
|8d82573a-926d-4754-9353-32dc29997f74.ta|./aist-tb-otrp/ta-aist-test|Unencrypted stub TA that just says hello when installed and you open a session to it|
|app.js|./aist-tb-otrp/fake-tam|The Fake TAM node.js application|
|spaik-pub.jwk|./aist-tb-otrp/test-jw/tee/sds/xbank/spaik-pub.jwk|The PKI TEE SPAIK public part|
|tam-mytam-rsa-key.pem|./aist-tb-otrp/pki/tam/tam/tam-mytam-rsa-key.pem|The TAM private key|

## Generating PKI and JWK parts

To generate the additional files, you should run first `./aist-tb-otrp/scripts/otrp-kickstart-pki.sh, and
then `./aist-tb-otrp/scripts/otrp-test.sh`.  See `./aist-tb-otrp/README-pki.md` for advice on that.

That will create `./aist-tb-otrp/pki` which contains the X.509 PKI infrastructure, and `./aist-tb-otrp/test-jw`,
which contains some JWK extracted from certs and keys in the `./aist-tb-otrp/pki` directory.

## Generating the test TA

`make optee_os` should be enough to generate the test TA.

## Copying everything to the "Fake TAM" server

Copy all the listed pieces to a `/var/www/node` directory on your server.

## Running the fake TAM server

`node /var/www/node/app.js`

will run the test server.  You may need to open its firewall fop port 3000/tcp.

