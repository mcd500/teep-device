const util = require('util')
const fs = require('fs')
const path = require('path')
const { JWE, JWK, JWS } = require('node-jose')

const keystore = JWK.createKeyStore()
const keytype = 'RSA'
const keybitlen = 4096
const writeFile = util.promisify(fs.writeFile)
const readFile = util.promisify(fs.readFile)
const priv_jwk_fname = process.argv[2]
const pub_jwk_fname = process.argv[3]
let params = ""
if (process.argv[4]) {
	params = JSON.parse(process.argv[4])
}

if (!priv_jwk_fname || !pub_jwk_fname) {
	console.log("please input priv/pub key filename")
	console.log(`usage: node generate-key.js PRIV_JWK_FILENAME PUB_JWK_FILENAME [PARAMS]`)
	process.exit(1)
}

async function go() {
	console.log(`generating ${keytype} key, ${keybitlen} bit, param = ${JSON.stringify(params)}`)
	var err
	const key = await keystore.generate(keytype, keybitlen, params)
	const priv_key = JSON.stringify(key.toJSON(true))
	const pub_key = JSON.stringify(key.toJSON())
	err = await writeFile(priv_jwk_fname, priv_key)
	if (err) {
		throw err
	}
	err = await writeFile(pub_jwk_fname, pub_key)
	if (err) {
		throw err
	}
}

go().catch((err) => {
	console.error(err)
	process.exitCode =1
})
