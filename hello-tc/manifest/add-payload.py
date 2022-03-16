#!/usr/bin/python3

import cbor2 as cbor
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--key')
parser.add_argument('--payload')
parser.add_argument('--in')
parser.add_argument('--out')
args = parser.parse_args()

with open(args.payload, "rb") as fp:
    payload = fp.read()

with open(vars(args)["in"], "rb") as fp:
    tagged_envelope = cbor.decoder.load(fp)

tagged_envelope.value[args.key] = payload

with open(args.out, "wb") as fp:
    cbor.encoder.dump(tagged_envelope, fp)
