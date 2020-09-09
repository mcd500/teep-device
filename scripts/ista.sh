#!/bin/bash -x

IP=127.0.0.1
PORT=3000

if [ ! -z "$1" ]; then
  IP=$1
fi

if [ ! -z "$2" ]; then
  PORT=$2
fi

teep-broker-app --tamurl http://${IP}:${PORT}/api/tam_jose --jose
