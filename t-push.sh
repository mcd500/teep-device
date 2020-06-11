#!/bin/bash
git push || true
cd ..
git add teep-device
git commit -m "bump submodule"
git push || true
