#!/bin/sh

TA_file="dummy-ta"
PrivateKey="tc-signer-P256-priv.pem"
PublicKey="tc-signer-P256-pub.pem"
CertFile="tc-signer-P256-cert.pem"
HashOutputFile="TA-Hash"
EncryptedHashFile="tc-signature"
DecryptedHashFile="Decrypt-TA-Hash"

# Step 1 : Create a 50KB dummy TA
dd if=/dev/urandom of=$TA_file bs=1024 count=50 2>/dev/null

# Step 2: Calculate hash value for dummy TA file
sha512sum $TA_file | cut -d ' ' -f1 > $HashOutputFile


# Step 3: Encrypt the generated hash value using the private key tc-signer-P256-priv.pem
openssl cms -encrypt -binary -aes-256-cbc -in $HashOutputFile -out $EncryptedHashFile $CertFile

# Step 4: Decrypt the hash value
openssl cms -decrypt -in $EncryptedHashFile -out $DecryptedHashFile -inkey $PrivateKey

# Some other options to encrypt and decrypt
# openssl enc \
# 	-in $HashOutputFile \
# 	-out $EncryptedHashFile \
# 	-e -aes256 \
# 	-md sha512 \
#     -pbkdf2 -salt \
# 	-k $PrivateKey
# openssl rsautl -encrypt -inkey $PublicKey -pubin -in $HashOutputFile -out $EncryptedHashFile

# DecryptedHashValue=$(openssl enc \
# 	-in $EncryptedHashFile \
# 	-d -aes256 \
# 	-md sha512 \
# 	-pbkdf2 -salt \
# 	-k $PrivateKey)

# Step 5: Verify the decrypted hash value with the original hash.
HashValue=`cat $HashOutputFile`
DecryptedHashValue=`cat $DecryptedHashFile`

if [ "$HashValue" = "$DecryptedHashValue" ]; then
    echo "Encryption and Decryption success"
else
    echo "Encryption and Decryption Failed"
fi

# Clean the generated files
rm $TA_file
rm $HashOutputFile
rm $EncryptedHashFile
rm $DecryptedHashFile
