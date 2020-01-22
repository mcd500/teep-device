const http = require('http');
const jose = require('node-jose');
const fs = require('fs');
const os = require('os');
const config_table = require('./config.json')
const env = process.env.NODE_ENV || 'development';
const config = config_table[env];
config.hostname = process.env.TAM_SERVER_HOSTNAME || config.hostname

var keystore = jose.JWK.createKeyStore();

/* this is the key used to encrypt the TA */
var tee_pubkey = fs.readFileSync(config.ta_pub_key, function(err, data) {
		console.log(data);
	});
/* this is the key used to sign the JWE */
var tam_privkey = fs.readFileSync(config.tam_key, function(err, data) {
		console.log(data);
	});

var jwk_tam_privkey, jwk_tee_pubkey;

keystore.add(tee_pubkey, "json").then(function(result) {
		jwk_tee_pubkey = result;
        });
keystore.add(tam_privkey, "pem").then(function(result) {
		jwk_tam_privkey = result;
        });

/* the plaintext TA */
var f, f_pt = fs.readFileSync(config.ta, function(err, data) {
	console.log(data);
});

const server = http.createServer((req, res) => {
	// get request body
	let body = [];
	req.on('data', (chunk) => {
		body.push(chunk);
	}).on('end', () => {
		body = Buffer.concat(body).toString();
		handleRequest(req, body, res);
	});
});


function handleRequest(req, body, res) {
	dumpHttpRequest(req, body);
	if (!body) {
		// if body is empty, goto OTrP:GetDeviceState or TEEP:QueryRequest
		handleAppInstall(req, null, res)
		// handleQuery(req, res);
	} else {
		// parse json for switch TEEP Response
		let bodyJson = JSON.parse(body);
		console.log("parsed JSON:", bodyJson);

		if (bodyJson.GetDeviceStateResponse !== undefined) {
			console.log("reviced GetDeviceStateResponse");
			// TODO: switch handleAppInstall or handleAppDelete
			handleAppInstall(req, bodyJson, res);
		} else if (bodyJson.InstallTAResponse !== undefined) {
			console.log("reviced InstallTAResponse");
			// no more sequence, return 204
			res.statusCode = 204;
			res.end();
	  } else if (bodyJson.DeleteTAResponse !== undefined) {
			console.log("reviced DeleteTAResponse");
			// no more sequence, return 204
			res.statusCode = 204;
			res.end();
		} else {
			// unknown OTrP or TEEP message
			res.statusCode = 400;
			console.error("Unknown OTrP or TEEP message");
			res.end();
		}
	}
}

// OTrP:GetDeviceState or TEEP:QueryRequest
function handleQuery(req, res) {
	console.log("Request for send QueryRequest\n");
	res.statusCode = 200;
	res.setHeader('Content-Type', 'application/tee-ta');
	res.end();
}

// OTrP::InstallTA or TEEP:TrustedAppInstall
function handleAppInstall(req, bodyJson, res) {
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

			dumpHttpResponse(res, f);
		});
	});
}

// OTrP::DeleteTA or TEEP:TrustedAppDelete
function handleAppDelete(req, bodyJson, res) {
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
}

function dumpHttpRequest(req, body) {
	console.log("========== dump http request ==========");
	console.log("method: " + req.method);
	console.log("url: " + req.url);
	console.log("httpVersion: " + req.httpVersion);

	console.log("headers: " + JSON.stringify(req.headers));

	console.log("body: " + body);
	console.log("\n");
}

function dumpHttpResponse(res, body) {
	console.log("========== dump http response ==========")

	console.log("status code: " + res.statusCode + " " + res.statusMessage);

	console.log("headers: " + res._header);

	if (body.length < 128) {
		console.log("body: " + body);
	} else {
		// 長いbodyは128文字で区切り
		console.log("body: " + body.substring(0, 128) + "...");
	}
	console.log("\n");
}

server.listen(config.port, config.hostname, () => {
  console.log(`Server running at http://${config.hostname}:${config.port}/`);
});

