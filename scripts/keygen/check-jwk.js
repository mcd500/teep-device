const util = require('util')
const fs = require('fs')
const path = require('path')
const { JWE, JWK, JWS } = require('node-jose')

const writeFile = util.promisify(fs.writeFile)
const readFile = util.promisify(fs.readFile)
const priv_jwk_fname = process.argv[2]
const pub_jwk_fname = process.argv[3]

const sample_mes = "Hello"

async function go() {
	console.log(`importing ${priv_jwk_fname} ${pub_jwk_fname}`)
	const priv_key = await JWK.asKey(await readFile(priv_jwk_fname))
	const pub_key = await JWK.asKey(await readFile(pub_jwk_fname))
	const signed_mes =  await JWS.createSign(priv_key).update(sample_mes).final()
	const verified_mes = await JWS.createVerify(pub_key).verify(signed_mes)
	if (verified_mes) {
		console.log("sign/verify: OK")
	} else {
		console.log("sign/verify: NG")
	}
	const encrypted = await JWE.createEncrypt(pub_key).update(sample_mes).final()
	const decrypted = await JWE.createDecrypt(priv_key).decrypt(encrypted)
	if (decrypted.payload.toString() === sample_mes) {
		console.log("encrypt/decrypt: OK")
	} else {
		console.log("encrypt/decrypt: NG")
	}
}

go().catch((err) => {
	console.error(err)
	process.exitCode =1
})
