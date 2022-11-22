#!/bin/sh

DOWNLOAD_TC_SUIT="signed-download-tc.suit"
TAM_CBOR_FILE="integrated-payload-manifest.cbor"
HELLO_TA="8d82573a-926d-4754-9353-32dc29997f74.ta"

SOURCE_DIR="/home/user/teep-device/build/optee/hello-tc"
TAMPROTO_TA_DIR="/home/user/tamproto/TAs"

# Copy the files into Tamproto
cp $SOURCE_DIR/$DOWNLOAD_TC_SUIT $TAMPROTO_TA_DIR/$TAM_CBOR_FILE && cp $SOURCE_DIR/$HELLO_TA  $TAMPROTO_TA_DIR/

# Check if the copy command was successful
if [ $? -eq 0 ]; then
    echo "Files copied successfully !!"
else
    echo "There was some error in copying the files.."
fi
