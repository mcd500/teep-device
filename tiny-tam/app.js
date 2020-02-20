const http = require('http')
const {JWK, JWS, JWE} = require('node-jose')
const fs = require('fs')
const path = require('path')
const os = require('os')
const config_table = require('./config.json')
const env = process.env.NODE_ENV || 'development'
const config = config_table[env]
const teep = require('./teep.js')
const otrp = require('./otrp.js')

const hostname = process.argv[2] || process.env.TAM_SERVER_HOSTNAME || config.hostname
const port = process.argv[3] || process.env.TAM_SERVER_PORT || config.port
const taname = process.argv[4] || process.env.TAM_SERVER_TA || path.basename(config.ta)
const tapath = config.ta

function loadJwk(jwkfile) {
	return JWK.asKey(fs.readFileSync(jwkfile, function(err, data) { console.log(data) }))
}


async function go() {
	/* Load jwks */
	const spSignKey = await loadJwk(config.sp_priv_key)
	const tamPrivKey = await loadJwk(config.tam_priv_key)
	const teePubKey = await loadJwk(config.tee_pub_key)


	/* TA image signed by SP */
	const taImage = fs.readFileSync(config.ta, (err) => {console.log(err)})
	const taUrl = `http://${hostname}:${port}/TAs/${taname}`
/*
	async function signAndEnc() {
		const signParam = {
			alg: 'RS256',
			format: 'flattened'
		}
		const encParam = {
			fields: {
				alg: 'RSA1_5'
			},
			contentAlg: "A128CBC-HS256",
			format: 'flattened'
		}
		signed = await JWS.createSign(signParam, spSignKey).update(taImage).final()
		encrypted = await JWE.createEncrypt(encParam, teePubKey).update(JSON.stringify(signed)).final()
		return JSON.stringify(encrypted)
	}
	const signAndEncImage = await signAndEnc()
	fs.writeFileSync("hoge", signAndEncImage, (err) => {console.log(err)})
*/
	teepHandler = teep(tamPrivKey, teePubKey, taImage, taUrl, path.basename(taname, ".ta.sign.enc"));
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
			console.log(typeof(chunk))
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
			const url = req.url
			if (method == 'POST' && accept == 'application/otrp+json') {
				return otrpHandler.handleMessage(req, body, res).catch(console.log)
			}
			if (method == 'POST' && accept == 'application/teep+json') {
				return teepHandler.handleMessage(req, body, res).catch(console.log)
			}
			if (method == 'GET' && url.startsWith('/TAs')) {
				if (path.basename(url) == taname ) {
					res.statusCode = 200
					res.end(taImage)
					return taImage
				}
			}
			res.statusCode = 404
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
