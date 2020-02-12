# Setting up the Tiny TAM

## Preparing node and dependencies

Install node.js, for Fedora the package is called "nodejs".

Then use npm to install dependent libraries.

```
npm install
```
## How to run Tiny TAM server

```
node app.js [hostname] [port]
```

## File structures

The "Tiny TAM" is a node.js application that listens on port 3000

| File name | Function |
|-----------|----------|
| ./app.js | TAM server application program |
| ./otrp.js | OTrP message handling |
| ./teep.js | TEEP message handling |
| ./config.json | configuration file for `jwk` files and/or TA-image file to be installed |

## Setting up pki pieces and test ta

The "Tiny TAM" serves an encrypted + signed test TA to anyone making requests.
To do that, following assets should be configured on `config.json`

| Setting | Function | Default |
|---------|----------|---------|
|`tam_priv_key` | TAM server private JWK | : "../test-jw/tsm/identity/private/tam-mytam-private.jwk" |
|`tee_pub_key` | TEE public JWK | ../test-jw/tee/identity/tee-mytee-public.jwk |
|`sp_priv_key` | SP private JWK | ../test-jw/tee/sds/xbank/spaik-priv.jwk |
|`ta` | TA-image (plain) | ../sp-hello-ta/8d82573a-926d-4754-9353-32dc29997f74.ta |

## API endpoint (work in progress)

| URL | Method | Function |
|-----|--------|----------|
| `/tam` | `PUSH` |TEEP/OTrP over HTTP without encryption and sign |
| `/tam_jose` | `PUSH` |TEEP/OTrP over HTTP with encryption and sign |

### Force Padding Scheme in spaik pubkey (work in progress)

Edit `spaik-pub.jwk` to force the `RSA1_5` padding scheme.  At the top, change

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

Copy all the listed pieces to a `/var/www/node` directory on your server.


## Edit app,js hostname

Edit `/var/www/node/app,js` so the hostname matches the external IP of your server.

## Open your server filewall for port 3000 if necessary

Eg for Fedora, `firewall-cmd --add-port 3000/tcp`


`node /var/www/node/app.js`

will run the test server.  You may need to open its firewall fop port 3000/tcp.

