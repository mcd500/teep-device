#!/bin/bash
git push || true
cd ..
git add aist-teep
git commit -m "bump submodule"
git push || true
cd ..
git add aist-otrp
git commit -m "bump submodule"
git push || true
