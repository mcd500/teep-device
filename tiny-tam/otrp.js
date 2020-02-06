const {JWK, JWS, JWE} = require('node-jose');
module.exports = (tamPrivKey, teePubKey, taImage) => ({
	token: 0,
	session: {},
	sign(data) {return JWS.createSign({alg: 'RS256', format: 'flattend'}, tamPrivKey).update(data).final()},
	verify(data) {return JWS.createVerify(teePubKey).verify(data)},
	encrypt(data) {return JWE.createEncrypt({ alg: 'RSA1_5', contentAlg: 'A128CBC-HS256', format: 'flattened' }, teePubKey).update(data).final()},
	decrypt(data) {return JWE.crateDecrypt(tamPrivKey).decrypt(data)},

	async wrap(data) {
		signed = await this.sign(JSON.stringify(data))
		encrypted = await this.encrypt(JSON.stringify(signed))
		return JSON.stringify(encrypted)
	},

	async unwrap(data) {
		decrypted = await this.decrypt(JSON.parse(data))
		verified = await this.verify(JSON.parse(decrypted.payload))
		if (verified) {
			return JSON.parse(verified.payload)
		}
		return null
	},
	async handleMessage(req, body, res) {
		if (req.url == "/delete") {
			return this.handleAppDelete(req, null, res);
		}
		if (!body) {
			// if body is empty, goto OTrP:GetDeviceState or TEEP:QueryRequest
			return this.handleAppInstall(req, null, res)
				// handleQuery(req, res);
		} else {
			// parse json for switch TEEP Response
			let bodyJson = JSON.parse(body);
			console.log("parsed JSON:", bodyJson);

			if (bodyJson.GetDeviceStateResponse !== undefined) {
				console.log("reviced GetDeviceStateResponse");
				// TODO: switch handleAppInstall or handleAppDelete
				return this.handleAppInstall(req, bodyJson, res);
			} else if (bodyJson.InstallTAResponse !== undefined) {
				console.log("reviced InstallTAResponse");
				// no more sequence, return 204
				res.statusCode = 204;
				res.end();
				return
			} else if (bodyJson.DeleteTAResponse !== undefined) {
				console.log("reviced DeleteTAResponse");
				// no more sequence, return 204
				res.statusCode = 204;
				res.end();
				return
			} else {
				// unknown OTrP or TEEP message
				res.statusCode = 400;
				console.error("Unknown OTrP or TEEP message");
				res.end();
				return
			}
		}
	},
	// OTrP::InstallTA or TEEP:TrustedAppInstall
	async handleAppInstall(req, bodyJson, res) {
		console.log(req.url + ": Request for encrypted TA\n");
		let message = await this.encrypt(taImage)
		res.statusCode = 200;
		res.setHeader('Content-Type', 'application/tee-ta');
		res.setHeader('Content-Length', message.length);
		res.end(f);
	},

	// OTrP::DeleteTA or TEEP:TrustedAppDelete
	async handleAppDelete(req, bodyJson, res) {
		console.log("Request for delete packet\n");
		const cmd = "{\"delete-ta\":\"8d82573a-926d-4754-9353-32dc29997f74.ta\"}";
		let message = await this.wrap(cmd)
		res.statusCode = 200;
		res.setHeader('Content-Type', 'application/tee-ta');
		res.setHeader('Content-Length', message.length);
		res.end(message)
	},

	// OTrP:GetDeviceState or TEEP:QueryRequest
	async handleQuery(req, res) {
		console.log("Request for send QueryRequest\n");
		res.statusCode = 200;
		res.setHeader('Content-Type', 'application/tee-ta');
		res.end();
	}

})

