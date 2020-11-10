# TEEP device side implementation

## supported TEEs

* keystone
* optee
* pc (not Trusted, developing purpose)

## Prerequisite

Use Ubuntu 20.04 or docker images below.

For keystone and optee, docker image is recommended.

### Ubuntu (not recommended)

```sh
apt get (TODO)
```

Build keystone and optee, this task is not needed for TEE=pc.
```sh
TODO
```

### Docker (recommended)

```sh
docker pull trasioteam/ta_ref_teep_devel:keystone_qemu
docker pull trasioteam/ta_ref_teep_devel:optee_qemu
```

### booting tamproto

Clone tamproto server

```sh
git clone https://192.168.100.100/rinkai/tamproto.git
cd tamproto
git checkout teep-device-interop-cbor
```

Once, it is cloned, it is able to start tamproto server directly from next time.

```sh
docker-compose build
docker-compose up
```

Check if tamproto server is accessible with this url.
```
curl -d '' http://localhost:8888/api/tam_cbor
```

## git clone

Clone parent module `ta-ref` and clone this repo as sub-module.
This sub-module relation is planned to be removed in the future.

```sh
GIT_SSL_NO_VERIFY=1 git clone https://192.168.100.100/rinkai/ta-ref.git
cd ta-ref
git checkout teep-device-dev_cbor-latest-draft
git submodule update --init --recursive
```

## run Docker container (recommended)

Run docker container when using docker.

TEE=keystone
```sh
docker run --network tamproto_default -it --rm -v $(pwd):/home/main/ta-ref trasioteam/ta_ref_teep_devel:keystone_qemu
```

TEE=optee
```sh
docker run --network tamproto_default -it --rm -v $(pwd):/home/main/ta-ref trasioteam/ta_ref_teep_devel:optee_qemu
```

And sections below should be executed in this shell.

```sh
cd ta-ref
```

Check if tamproto server is accessible with this url.
```
curl -d '' http://tamproto_tam_api_1:8888/api/tam_cbor
```

## build `teep-device`

Initialize environment variables respective using TEE.

TEE=keystone
```sh
source env/keystone.sh
```

TEE=optee
```sh
source env/optee-qemu.sh
```

TEE=pc
```sh
source env/pc.sh
```

Then, build `ta-ref`.
```
make
```

build `teep-device`.
```
cd teep-device
make
```

## test with `tamproto` server

```sh
make test
```

## run qem

```sh
make qemu
```

