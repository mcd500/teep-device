const http = require('http');
const jose = require('/usr/lib/node_modules/node-jose');
const fs = require('fs');

const hostname = '192.168.2.236';
const port = 3000;

const TA_FILEPATH = "/var/www/node/8d82573a-926d-4754-9353-32dc29997f74.ta";

var keystore = jose.JWK.createKeyStore();

/* this is the key used to encrypt the TA */
var tee_pubkey = fs.readFileSync("/var/www/node/spaik-pub.jwk", function(err, data) {
		console.log(data);
	});
/* this is the key used to sign the JWE */
var tam_privkey = fs.readFileSync("/var/www/node/tam-mytam-rsa-key.pem", function(err, data) {
		console.log(data);
	});

var jwk_tam_privkey, jwk_tee_pubkey;

keystore.add(tee_pubkey, "json").then(function(result) {
		jwk_tee_pubkey = result;
        });
keystore.add(tam_privkey, "pem").then(function(result) {
		jwk_tam_privkey = result;
        });


const server = http.createServer((req, res) => {
	/* the plaintext TA */
	var f, f_pt = fs.readFileSync(TA_FILEPATH, function(err, data) {
		console.log(data);
	});

	if (req.url == "/delete") {
		var cmd = "{\"delete-ta\":\"8d82573a-926d-4754-9353-32dc29997f74.ta\"}";
		console.log("Request for delete packet\n");

		jose.JWE.createEncrypt({ alg: 'RSA1_5', contentAlg: 'A128CBC-HS256', format: "flattened" },
				jwk_tee_pubkey).update(cmd).final().then
					(function(result) {
			f = JSON.stringify(result);

			jose.JWS.createSign({ alg: 'RS256', format: 'flattened' }, jwk_tam_privkey).update(f).final().then
						(function(result) {
				f = JSON.stringify(result);
				res.statusCode = 200;
				res.setHeader('Content-Type', 'application/tee-ta');
				res.setHeader('Content-Length', f.length);
				res.end(f);
			});
		});
	
	} else {

		console.log(req.url + ": Request for encrypted TA\n");

		jose.JWE.createEncrypt({ alg: 'RSA1_5', contentAlg: 'A128CBC-HS256', format: "flattened" },
				jwk_tee_pubkey).update(f_pt).final().then
					(function(result) {
			f = JSON.stringify(result);

			jose.JWS.createSign({ alg: 'RS256', format: 'flattened' }, jwk_tam_privkey).update(f).final().then
						(function(result) {
				f = JSON.stringify(result);
				res.statusCode = 200;
				res.setHeader('Content-Type', 'application/tee-ta');
				res.setHeader('Content-Length', f.length);
				res.end(f);
			});
		});
	}
});

server.listen(port, hostname, () => {
  console.log(`Server running at http://${hostname}:${port}/`);
});

