const {JWK, JWS, JWE} = require('node-jose')
const fs = require('fs')
const signKeyName = process.argv[2]
const encKeyName = process.argv[3]
const taname = process.argv[4]
const taImage = fs.readFileSync(taname, (err) => {console.log(err)})

function loadJwk(jwkfile) {
	return JWK.asKey(fs.readFileSync(jwkfile, function(err, data) { console.log(data) }))
}


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
	const signKey = await loadJwk(signKeyName)
	const encKey = await loadJwk(encKeyName)
	const signed = await JWS.createSign(signParam, signKey).update(taImage).final()
	const encrypted = await JWE.createEncrypt(encParam, encKey).update(JSON.stringify(signed)).final()
	fs.writeFileSync(`${taname}.sign.enc`, JSON.stringify(encrypted), (err) => {console.log(err)})
}

signAndEnc()
