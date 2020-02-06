const {JWK, JWS, JWE} = require('node-jose');
const QUERY_REQUEST = 1
const QUERY_RESPONSE = 2
const TRUSTED_APP_INSTALL = 3
const TRUSTED_APP_DELETE = 4
const SUCCESS = 5
const ERROR = 6

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
module.exports = (tamPrivKey, teePubKey, taImage, taUrl) => ({
	token: 0,
	session: {},
	sign(data) {return JWS.createSign(signParam, tamPrivKey).update(data).final()},
	verify(data) {return JWS.createVerify(teePubKey).verify(data)},
	encrypt(data) {return JWE.createEncrypt(encParam, teePubKey).update(data).final()},
	decrypt(data) {return JWE.createDecrypt(tamPrivKey).decrypt(data)},

	async wrap(data) {
		console.log(data)
		signed = await this.sign(JSON.stringify(data))
		console.log(signed)
		encrypted = await this.encrypt(JSON.stringify(signed))
		console.log(encrypted)
		return JSON.stringify(encrypted)
	},

	async unwrap(data) {
		console.log(data)
		decrypted = await this.decrypt(JSON.parse(data))
		console.log(decrypted)
		verified = await this.verify(JSON.parse(decrypted.payload))
		if (verified) {
			return JSON.parse(verified.payload)
		}
		return null
	},

	async finishTeep(res) {
		res.statusCode = 204
		res.setHeader('Content-Length', 0) // if we don't add this header, client wait for rest data
		res.end()
	},

	async sendTeepMessage(jose, mesg, res, statusCode) {
		this.token += 1;
		mesg.TOKEN = this.token.toString()
		this.session[mesg.TOKEN] = mesg
		let wrapped
		if (jose) {
			wrapped = await this.wrap(mesg);
		} else {
			wrapped = JSON.stringify(mesg)
		}
		res.statusCode = statusCode
		res.setHeader('Content-Type', 'application/teep+json')
		res.setHeader('Content-Length', wrapped.length)
		res.end(wrapped)
	},
	sendQueryRequest(jose, res) {
		const mes = {
			TYPE: QUERY_REQUEST,
			REQUEST:[2] // TA list
		}
		this.sendTeepMessage(jose, mes, res, 200)
	},
	sendTrustedAppInstall(jose, res) {
		console.log("sendTrustedAppInstall");
		const mes = {
			TYPE: TRUSTED_APP_INSTALL,
			MANIFEST_LIST: [taUrl] // TA list
		}
		this.sendTeepMessage(jose, mes, res, 200)
	},
	sendTrustedAppDelete(jose, res) {
		console.log("sendTrustedAppDelete");
		const mes = {
			TYPE: TRUSTED_APP_DELETE,
			TA_LIST: [
				{
					Vecndor_ID: "ietf-teep-wg",
					Class_ID: "8d82573a-926d-4754-9353-32dc29997f74",
					Device_ID: "teep-device"
				}
			]
		}
		this.sendTeepMessage(jose, mes, res, 200)
	},

	async handleMessage(req, body, res) {
		console.log("teep message detected")
		let jose;
		if (req.url == '/') {
			jose = false
		} else if (req.url == '/api/tam') {
			jose = false
		} else if (req.url == '/api/tam_jose') {
			jose = true
		} else {
			console.log("invalid url")
			res.statusCode = 404
			res.end()
			return
		}
		if (!body) {
			// if body is empty, goto OTrP:GetDeviceState or TEEP:QueryRequest
			console.log("handle empty message")
			return this.sendQueryRequest(jose, res);
		}
		// parse json for switch TEEP Response	
		let teepRes
		if (jose) {
			teepRes = await this.unwrap(body)
		} else {
			teepRes = JSON.parse(body)
		}
		if (!teepRes || typeof(teepRes.TOKEN) !== 'string' || typeof(teepRes.TYPE) !== 'number') {
			console.log("failed to parse json body" , teepRes)
			return this.finishTeep()
		}
		console.log("parsed JSON:", teepRes)
		if (teepRes.TYPE == QUERY_RESPONSE) {
			console.log("detect QUERY_RESPONSE")
			if (teepRes.TA_LIST.find((triple) => triple.Class_ID == '8d82573a-926d-4754-9353-32dc29997f74')) {
				return this.sendTrustedAppDelete(jose, res)
			} else {
				return this.sendTrustedAppInstall(jose, res);
			}
		}
		teepReq = this.session[teepRes.TOKEN]
		if (teepReq.TYPE == TRUSTED_APP_INSTALL) {
			if (teepRes.TYPE == SUCCESS) {
				console.log("trusted app install succeed")
				return this.finishTeep(res)
			} else if (teepRes.TYPE == ERROR) {
				console.log("trusted app install failed")
				return this.finishTeep(res)
			}
		}
		if (teepReq.TYPE == TRUSTED_APP_DELETE) {
			if (teepRes.TYPE == SUCCESS) {
				console.log("trusted app delete succeed")
				return this.finishTeep(res)
			} else if (teepRes.TYPE == ERROR) {
				console.log("trusted app delete failed")
				return this.finishTeep(res)
			}
		}
	}
})
