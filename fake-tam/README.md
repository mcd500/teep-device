# Setting up the Fake TAM

## Preparing node

Install node.js, for Fedora the package is called "nodejs".

Then use npm to install the node package for JOSE.

```
sudo npm install --global node-jose
```

## Setting up pki pieces and test ta

The "Fake TAM" is a node.js application that listens on port 3000
and serves an encrypted + signed test TA to anyone making requests.

To do that, it needs the following assets (in `/var/www/node` as
currently configured).

|Remote Filename on server|Source|Function|
|---|---|---|
|8d82573a-926d-4754-9353-32dc29997f74.ta|./aist-teep/sp-hello-ta|Unencrypted stub TA that just says hello when installed and you open a session to it|
|app.js|./aist-teep/fake-tam|The Fake TAM node.js application|
|spaik-pub.jwk|./aist-teep/test-jw/tee/sds/xbank/spaik-pub.jwk|The PKI TEE SPAIK public part|
|tam-mytam-rsa-key.pem|./aist-teep/pki/tam/tam/tam-mytam-rsa-key.pem|The TAM private key|

### Force Padding Scheme in spaik pubkey

Edit `spaik-pub.jwk` to force the RSA1_5 padding scheme.  At the top, change

```
..."kty":"RSA","n":...
```

to

```
..."kty":"RSA","alg":"RSA1_5","n":...
```

## Generating PKI and JWK parts

To generate the additional files, you should run first `./aist-teep/scripts/otrp-kickstart-pki.sh, and
then `./aist-teep/scripts/otrp-test.sh`.  See `./aist-teep/README-pki.md` for advice on that.

That will create `./aist-teep/pki` which contains the X.509 PKI infrastructure, and `./aist-teep/test-jw`,
which contains some JWK extracted from certs and keys in the `./aist-teep/pki` directory.

## Generating the test TA

`make optee_os` should be enough to generate the test TA.

## Copying everything to the "Fake TAM" server

Copy all the listed pieces to a `/var/www/node` directory on your server.


## Edit app,js hostname

Edit `/var/www/node/app,js` so the hostname matches the external IP of your server.

## Open your server filewall for port 3000 if necessary

Eg for Fedora, `firewall-cmd --add-port 3000/tcp`

## Running the fake TAM server

`node /var/www/node/app.js`

will run the test server.  You may need to open its firewall fop port 3000/tcp.

