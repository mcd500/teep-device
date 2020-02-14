const {JWK, JWS, JWE} = require('node-jose');

const VER = "1.0"
const TAMID = "id1.TAMxyz.com"
const TA_UUID = "8d82573a-926d-4754-9353-32dc29997f74"
const OCSPDAT = "sample ocspdat B64 encoded ASN1"
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
	rid: 0,
	session: {},
	sign(data) {return JWS.createSign(signParam, tamPrivKey).update(data).final()},
	verify(data) {return JWS.createVerify(teePubKey).verify(data)},
	encrypt(data) {return JWE.createEncrypt(encParam, teePubKey).update(data).final()},
	decrypt(data) {return JWE.createDecrypt(tamPrivKey).decrypt(data)},

	async wrap(data) {
		console.log(data)
		signed = await this.sign(JSON.stringify(data))
		console.log(signed)
		return JSON.stringify(signed)
	},

	async unwrap(data) {
		console.log(data)
		verified = await this.verify(JSON.parse(data))
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

	async sendOTrPMessage(jose, mesg, res, statusCode) {
		this.rid += 1;
		mesg.rid = this.rid.toString()
		this.session[mesg.rid] = mesg
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
	sendGetDeviceStateRequest(jose, res) {
		console.log("sendGetDeviceStateRequest");
		const mes = {
			GetDeviceStateTBSRequest: {
				ver: VER,
				tid: "1",
				ocspdat: [Buffer.from(OCSPDAT).toString('base64')]
			}
		}
		this.sendOTrPMessage(jose, mes, res, 200)
	},
	sendInstallTARequest(jose, res) {
		console.log("sendInstallTARequest");
		const mes = {
			InstallTATBSRequest: {
				ver: VER,
				tid: "1",
				tee: "aist-otrp",
				nextdsi: false,
				dsihash: undefined,
				content: {
					tamid: TAMID,
					taid: TA_UUID
				},
				encrypted_ta: taImage
			}
		}
		this.sendOTrPMessage(jose, mes, res, 200)
	},
	sendDeleteTARequest(jose, res) {
		console.log("sendDeleteTARequest");
		const mes = {
			DeleteTATBSRequest: {
				ver: VER,
				rid: "1",
				tid: "1",
				tee: "aist-otrp",
				nextdsi: false,
				dsihash: undefined,
				content: {
					tamid: TAMID,
					taid: TA_UUID
				}
			}
		}
		this.sendOTrPMessage(jose, mes, res, 200)
	},

	async handleMessage(req, body, res) {
		console.log("teep message detected")
		let jose = false
		let del = false
		if (req.url == '/') {
		} else if (req.url == '/api/tam') {
		} else if (req.url == '/api/tam_delete') {
			del = true
		} else if (req.url == '/api/tam_jose') {
			jose = true
		} else if (req.url == '/api/tam_jose_delete') {
			del = true
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
			return this.sendGetDeviceStateRequest(jose, res);
		}
		// parse json for switch TEEP Response	
		let teepRes
		if (jose) {
			teepRes = await this.unwrap(body)
		} else {
			teepRes = JSON.parse(body)
		}
		if (!teepRes) {
			console.log("failed to parse json body" , teepRes)
			return this.finishTeep()
		}
		console.log("parsed JSON:", teepRes)
		if (teepRes.GetDeviceTEEStateTBSResponse) {
			console.log("detect getDeviceTEEStateTBResonse")
			if (del) {
				return this.sendDeleteTARequest(jose, res)
			} else {
				return this.sendInstallTARequest(jose, res);
			}
		}
		if (teepRes.InstallTATBSResponse) {
			console.log("trusted app install succeed")
			return this.finishTeep(res)
		}
		if (teepReq.DeleteTATBSResponse) {
			console.log("trusted app delete succeed")
			return this.finishTeep(res)
		}
	}
})
