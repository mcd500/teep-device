const http = require('http')
const { JWE, JWK, JWS } = require('node-jose')

const signParam = { alg: 'RS256' }
const encParam = { alg: 'RSA1_5' }

async function wrap(obj, signkey, enckey) {
	signed = await JWS.createSign(signParam, signkey).update(JSON.stringify(obj)).final()
	encrypted = await JWE.createEncrypt(encParam, enckey).update(JSON.stringify(signed)).final()
	return JSON.stringify(encrypted)
}

async function unwrap(wrapped, deckey, verikey) {
	decrypted = await JWE.createDecrypt(deckey).decrypt(JSON.parse(wrapped))
	verified = await JWS.createVerify(verikey).verify(JSON.parse(decrypted.payload))
	return JSON.parse(verified.payload)
}

function make_agent(my_key, their_key) {
	return {
		send(message) {
			return wrap(message, my_key, their_key)
		},
		recv(message) {
			return unwrap(message, my_key, their_key)
		}
	}
};

async function make_keypair(keystore, kid) {
	privkey = await keystore.generate('RSA', 1024, {kid: kid})
	pubkey = await JWK.asKey(privkey.toJSON())
	return {
		priv: privkey,
		pub: pubkey,
	}
}

function install_ta(uuid) {
	// TODO
	console.log("-- retrieve TA image (TODO)")
	console.log("TEE->TAM request obtaining ta-image (http GET or otrp protocol)")
	console.log("TAM->TEE get TA image and verify with sp key")
	console.log("")
	return true
}

async function go() {
	keystore = JWK.createKeyStore()
	teekey = await make_keypair(keystore, 'tee');
	tamkey = await make_keypair(keystore, 'tam');

	tee = make_agent(teekey.priv, tamkey.pub);
	tam = make_agent(tamkey.priv, teekey.pub);

	query_request = {
		"TYPE": 1, // Query Request
		"TOKEN": "1",
		"REQUEST": [ 2 ] // trusted apps
	}

	http_res = await tam.send(query_request)

	console.log("-- QueryRequest")
	console.log("-- TAM->TEE")
	console.log("json: ", JSON.stringify(query_request))
	console.log("http_res: ", http_res)
	console.log("")

	query_request = await tee.recv(http_res)
	query_response = {
		"TYPE": 2, // Query Response,
		"TOKEN": query_request.TOKEN,
		"TA_LIST": [
			{
				"Vendor_ID": "ietf-teep-wg",
				"Class_ID": "3cfa03b5-d4b1-453a-9104-4e4bef53b37e", // dummy
				"Device_ID": "teep-device"
			}
		]
	}

	http_req = await tee.send(query_response)

	console.log("-- QueryResponse")
	console.log("-- TEE->TAM")
	console.log("json: ", JSON.stringify(query_response))
	console.log("http_req: ", http_req)
	console.log("")

	query_response = await tam.recv(http_req)
	console.log("-- TAM check whether tee has already installed 8d82573a-926d-4754-9353-32dc29997f74 or not")
	if (!query_response.TA_LIST.find((ta) => (ta.Class_ID == "8d82573a-926d-4754-9353-32dc29997f74.ta"))) {
		// request ta install if tee do not install 8d82573a-926d-4754-9353-32dc29997f74.ta
		trusted_app_install = {
			"TYPE": 3, // TrustedAppInstall
			"TOKEN": "2",
			"MANIFEST_LIST": [
				{
					"apply-image": [
						{
							"directive-set-ver": {
								"uri": "http://127.0.0.1/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta"
							}
						}
					]
				}
			]
		}

		http_res = await tam.send(trusted_app_install)
		console.log("-- TrastedAppInstall")
		console.log("-- TAM->TEE")
		console.log("json: ", JSON.stringify(trusted_app_install))
		console.log("http_res: ", http_res)
		console.log("")

		trusted_app_install = await tee.recv(http_res)
		result = install_ta(trusted_app_install.MANIFEST_LIST[0]["apply-image"][0]["directive-set-ver"].uri)
		if (result) {
			success = {
				"TYPE": 5, // Success
				"TOKEN": trusted_app_install.TOKEN
			}
			http_req = await tee.send(success)
			console.log("-- Success")
			console.log("-- TEE->TAM")
			console.log("json: ", JSON.stringify(success))
			console.log("http_req: ", http_req)
			console.log("")
			res = await tam.recv(http_req)
			if (res.TYPE === 5) {
				console.log("finish")
				return
			}
		}
	}
	console.log("fail")
}

go().catch(function(err) {
	console.log("fail: ", err);
});
