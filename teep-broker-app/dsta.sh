#!/bin/bash -xi

IP=127.0.0.1
PORT=3000

if [ ! -z "$1" ]; then
  PORT=$1
fi

teep-broker-app --tamurl http://${IP}:${PORT}/api/tam_jose_delete --jose --talist 8d82573a-926d-4754-9353-32dc29997f74
