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

	async sendGetDeviceStateRequest(jose, res) {
		console.log("sendGetDeviceStateRequest");
		this.rid += 1;
		const mes = {
			GetDeviceStateTBSRequest: {
				ver: VER,
				tid: "1",
				ocspdat: [Buffer.from(OCSPDAT).toString('base64')]
			}
		}
		let outer
		if (jose) {
			outer = JSON.stringify({
				GetDeviceStateRequest: await this.sign(JSON.stringify(mes))
			})
		} else {
			outer = JSON.stringify({
				GetDeviceStateRequest: mes
			})
		}
		res.setHeader('Content-Type', 'application/otrp+json')
		res.setHeader('Content-Length', outer.length)
		res.end(outer)
	},
	async sendInstallTARequest(jose, res) {
		console.log("sendInstallTARequest");
		this.rid += 1;
		const mes = {
			InstallTATBSRequest: {
				ver: VER,
				rid: this.rid,
				tid: "1",
				tee: "aist-otrp",
				nextdsi: false,
				dsihash: undefined,
				content: undefined,
				encrypted_ta: JSON.parse(taImage.toString()) // ta_image already wrapped by JWS & JWE
			}
		}
		let content = {
			tamid: TAMID,
			taid: TA_UUID
		};
		let outer
		if (jose) {
			// The "content" is a JSON encrypted message that includes actual input for the SD update. 
			// The standard JSON content encryption key (CEK) is used, and the CEK is encrypted by the target TEE's public key.
			mes.InstallTATBSRequest.content = this.encrypt(JSON.stringify(content));
			outer = JSON.stringify({
				InstallTARequest: await this.sign(JSON.stringify(mes))
			})
		} else {
			mes.InstallTATBSRequest.content = content;
			outer = JSON.stringify({
				InstallTARequest: mes
			})
		}
		res.setHeader('Content-Type', 'application/otrp+json')
		res.setHeader('Content-Length', outer.length)
		res.end(outer)
	},
	async sendDeleteTARequest(jose, res) {
		console.log("sendDeleteTARequest");
		this.rid += 1;
		const mes = {
			DeleteTATBSRequest: {
				ver: VER,
				rid: this.rid,
				tid: "1",
				tee: "aist-otrp",
				nextdsi: false,
				dsihash: undefined,
				content: undefined,
			}
		}
		let content = {
			tamid: TAMID,
			taid: TA_UUID
		};
		let outer
		if (jose) {
			// The "content" is a JSON encrypted message that includes actual input for the SD update. 
			// The standard JSON content encryption key (CEK) is used, and the CEK is encrypted by the target TEE's public key.
			mes.DeleteTATBSRequest.content = this.encrypt(content);
			outer = JSON.stringify({
				DeleteTARequest: await this.sign(JSON.stringify(mes))
			})
		} else {
			mes.DeleteTATBSRequest.content = content;
			outer = JSON.stringify({
				DeleteTARequest: mes
			})
		}
		res.setHeader('Content-Type', 'application/otrp+json')
		res.setHeader('Content-Length', outer.length)
		res.end(outer)
	},

	async handleMessage(req, body, res) {
		console.log("otrp message detected")
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
		teepRes = JSON.parse(body)
		if (!teepRes) {
			console.log("failed to parse json body" , teepRes)
			return this.finishTeep()
		}
		// GetDeviceStateResponse
		if (teepRes.GetDeviceStateResponse) {
			if (!Array.isArray(teepRes.GetDeviceStateResponse)) {
				console.log("failed to parse GetDeviceStateResponse array:" , teepRes.GetDeviceStateResponse)
				return this.finishTeep();
			}
			// if value is exists, verify GetDeviceTEEStateResponse for debug
			if (teepRes.GetDeviceStateResponse.length > 0) {
				let tbsRes;
				let teeState = teepRes.GetDeviceStateResponse[0].GetDeviceTEEStateResponse;
				console.log("GetDeviceTEEStateResponse=", teeState);
				if (jose) {
					let verifyedData = await this.verify(teeState);
					if (!verifyedData) {
						console.log("failed to verify GetDeviceTEEStateResponse");
						return this.finishTeep();
					}
					tbsRes = JSON.parse(verifyedData.payload.toString());
					console.log("verifyedData=", tbsRes);
				} else {
					tbsRes = teeState;
				}
				// try decrypt edsi
				if (tbsRes.GetDeviceTEEStateTBSResponse.edsi) {
					console.log("trying decrypt edsi=", tbsRes.GetDeviceTEEStateTBSResponse.edsi);
					if (jose) {
						let decEdsi = await this.decrypt(tbsRes.GetDeviceTEEStateTBSResponse.edsi);
						if (!decEdsi) {
							console.log("failed to decrypt GetDeviceTEEStateTBSResponse.edsi");
							return this.finishTeep();
						}
						console.log("decEdsi=", decEdsi.plaintext.toString());
					}
				}
			}
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
