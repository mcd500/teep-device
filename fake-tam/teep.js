const {JWK, JWS, JWE} = require('node-jose');
const QUERY_REQUEST = 1
const QUERY_RESPONSE = 2
const TRUSTED_APP_INSTALL = 3
const TRUSTED_APP_DELETE = 4
const SUCCESS = 5
const ERROR = 6

module.exports = (tamPrivKey, teePubKey, taImage) => ({
	token: 0,
	session: {},
	sign(data) {return JWS.createSign(tamPrivKey).update(data).final()},
	verify(data) {return JWS.createVerify(teePubKey).verify(data)},
	encrypt(data) {return JWE.createEncrypt(teePubKey).update(data).final()},
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

	async finishTeep(res) {
		res.statusCode = 204
		res.end()
	},

	async sendTeepMessage(mesg, res, statusCode) {
		this.token += 1;
		mesg.TOKEN = this.token.toString()
		this.session[mesg.TOKEN] = mesg
		wrapped = await this.wrap(mesg);
		res.statusCode = statusCode
		res.setHeader('Content-Type', 'application/teep+json')
		res.setHeader('Content-Length', wrapped.length)
		res.end(wrapped)
	},
	
	async handleMessage(req, body, res) {
		console.log("teep message detected")
		if (!body) {
			// if body is empty, goto OTrP:GetDeviceState or TEEP:QueryRequest
			console.log("handle empty message")
			return this.sendTeepMessage({message: "hello"}, res, 200)
		}
		// parse json for switch TEEP Response
		let teepRes = await this.unwrap(body)
		if (teepRes) {
			return this.sendTeepMessage({}, res, 200)
		}
		console.log("parsed JSON:", bodyJson)
		if (teepRes.TYPE == QUERY_RESPONSE) {
			return this.sendTeepMessage({}, res, 200)
		}
		teepReq = this.session[bodyJson.TOKEN]
		if (teepReq.TYPE == TRUSTED_APP_INSTALL) {
			if (teepRes.TYPE == SUCCESS) {
				return this.finishTeep(res)
			} else if (teepRes.TYPE == ERROR) {
				return this.finishTeep(res)
			} else {
				return this.finishTeep(res)
			}
		}
		if (teepReq.TYPE == TRUSTED_APP_DELETE) {
			if (teepRes.TYPE == SUCCESS) {
				return this.finishTeep(res)
			} else if (teepRes.TYPE == ERROR) {
				return this.finishTeep(res)
			} else {
				return this.finishTeep(res)
			}
			return
		}
	}
})
