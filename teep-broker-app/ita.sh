#!/bin/bash -x

IP=127.0.0.1
PORT=3000

teep-broker-app --tamurl http://${IP}:${PORT}/api/tam
