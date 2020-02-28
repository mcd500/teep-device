#!/bin/bash -xi

IP=127.0.0.1
PORT=3000

if [ ! -z "$1" ]; then
  PORT=$1
fi

teep-broker-app --tamurl http://${IP}:${PORT}/api/tam_jose --jose
