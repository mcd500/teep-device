const http = require('http');
const {JWK, JWS, JWE} = require('node-jose');
const fs = require('fs');
const os = require('os');
const config_table = require('./config.json')
const env = process.env.NODE_ENV || 'development';
const config = config_table[env];
const teep = require('./teep.js')
const otrp = require('./otrp.js')

const hostname = process.argv[2] || process.env.TAM_SERVER_HOSTNAME || config.hostname
const port = process.argv[3] || process.env.TAM_SERVER_PORT || config.port

function loadJwk(jwkfile) {
	return JWK.asKey(fs.readFileSync(jwkfile, function(err, data) { console.log(data) }))
}


async function go() {
	/* Load jwks */
	const spSignKey = await loadJwk(config.sp_priv_key)
	const tamPrivKey = await loadJwk(config.tam_priv_key)
	const teePubKey = await loadJwk(config.tee_pub_key)

	/* TA image signed by SP */
	const taImage = await JWS.createSign(spSignKey).update(fs.readFileSync(config.ta, (err) => {console.log(err)})).final()

	teepHandler = teep(tamPrivKey, teePubKey, taImage);
	otrpHandler = otrp(tamPrivKey, teePubKey, taImage);
	const server = http.createServer((req, res) => {
		let res_chunks = []
		let oldEnd = res.end
		let oldWrite = res.write
		res.write = function (chunk) {
			res_chunks.push(Buffer.from(chunk))
			oldWrite.apply(res, arguments)
		}
		res.end = function (chunk) {
			if (chunk)
				res_chunks.push(Buffer.from(chunk))
			oldEnd.apply(res, arguments)
		}
		res.on("finish", function () {
			let body = Buffer.concat(res_chunks).toString();
			dumpHttpResponse(res, body)
		})
		// get request body
		let req_chunks = [];
		req.on('data', (chunk) => {
			req_chunks.push(chunk);
		}).on('end', () => {
			let body = Buffer.concat(req_chunks).toString();
			dumpHttpRequest(req, body);
			const accept = req.headers.accept
			const method = req.method
			if (accept == 'application/otrp+json' && method == 'POST') {
				return otrpHandler.handleMessage(req, body, res).catch(console.log)
			}
			if (accept == 'application/teep+json' && method == 'POST') {
				return teepHandler.handleMessage(req, body, res).catch(console.log)
			}
			console.error("Unknown protocol");
			res.stausCode = 204
			res.end()
		});
	});
	
	server.listen(port, hostname, () => {
		console.log(`Server running at http://${hostname}:${port}/`);
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
	if (body.length < 128 || true) {
		console.log("body: " + body);
	} else {
		// 長いbodyは128文字で区切り
		console.log("body: " + body.substring(0, 128) + "...");
	}
	console.log("\n");
}

go().catch(console.log)
