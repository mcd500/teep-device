#!/bin/bash -xi

IP=127.0.0.1
PORT=3000

teep-broker-app --tamurl http://${IP}:${PORT}/api/tam --talist 8d82573a-926d-4754-9353-32dc29997f74
