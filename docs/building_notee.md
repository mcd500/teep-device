# Build TEEP-Device without having TEE installed

The building without TEE is prepared for debugging purposes during developing TEEP-Device itself. This method does not require any TEE hardware (TrustZone, SGX and etc) and TEE SDK (Keystone, OP_TEE and SGX SDK) installed in the local machine and it is meant to build and run on TEEP-Device on any x64 PC.

To run TEEP-Device, first we need to run tamproto inside the same host. Let's clone the tamproto and start it.

**Prerequisite**

Installing required packages.

```sh
sudo apt-get install -y build-essential git autoconf automake cmake
sudo apt-get install -y libcap-dev python3-pip
```

Installing suit-tools with specific commit which is compatible with current TEEP-Device.

```sh
git clone https://git.gitlab.arm.com/research/ietf-suit/suit-tool.git
cd suit-tool
git checkout ca66a97bac153864617e7868e44f4b409e3e6ed4 -b for-teep-device
python3 -m pip install --upgrade .
```

**Run tamproto**

```sh
# Clone the tamproto repo and checkout master branch
$ git clone https://192.168.100.100/rinkai/tamproto.git
$ cd tamproto
$ git checkout master
$ docker-compose build
$ docker-compose up &
$ cd ..
```

Trimmed output of starting tamproto. The tamproto is running at 192.168.11.4.

```console
tam_api_1  |   TEE_pub: 'teep.jwk' }
tam_api_1  | Load key TAM_priv
tam_api_1  | Load key TAM_pub
tam_api_1  | Load key TEE_priv
tam_api_1  | Load key TEE_pub
tam_api_1  | Key binary loaded
tam_api_1  | 192.168.11.4
tam_api_1  | Express HTTP  server listening on port 8888
tam_api_1  | Express HTTPS server listening on port 8443
```

**Clone TEEP-Device**

```sh
# Clone the teep-device repo and checkout master branch
$ git clone https://192.168.100.100/rinkai/teep-device.git
$ cd teep-device
$ git checkout master

# Sync and update the submodules
$ git submodule sync --recursive
$ git submodule update --init --recursive
```

**Build**

```sh
# Change to teep-device
$ cd ~/teep-device/

# set the TEE environment to PC which do not use any of TEEs
$ export TEE=pc

# Build the teep device
$ make
```

After the successful build, run the sample TEEP session with tamproto.

```sh
$ export TAM_IP=localhost
$ export TAM_URL="http://$TAM_IP:8888"
$ make run-sample-session
```

Trimmed output of the run.

```console
$ make run-sample-session
make -C sample run-session
make[1]: Entering directory '/home/gitlab-runner/projects/teep-device/sample'
make -C /home/gitlab-runner/projects/teep-device/sample/../hello-tc/build-pc SOURCE=/home/gitlab-runner/projects/teep-device/sample/../hello-tc upload-embed-manifest
make[2]: Entering directory '/home/gitlab-runner/projects/teep-device/hello-tc/build-pc'
curl http://localhost:8888/panel/upload \
	-F "file=@/home/gitlab-runner/projects/teep-device/hello-tc/build-pc/../../build/pc/hello-tc/signed-embed-tc.suit;filename=integrated-payload-manifest.cbor"
tam_api_1  | [
tam_api_1  |   Dirent { name: 'dummy', [Symbol(type)]: 1 },
tam_api_1  |   Dirent { name: 'dummy2.ta', [Symbol(type)]: 1 },
tam_api_1  |   Dirent {
tam_api_1  |     name: 'integrated-payload-manifest.cbor',
tam_api_1  |     [Symbol(type)]: 1
tam_api_1  |   },
tam_api_1  |   Dirent {
tam_api_1  |     name: 'integrated-payload-manifest_hex.txt',
tam_api_1  |     [Symbol(type)]: 1
tam_api_1  |   },
tam_api_1  |   Dirent { name: 'suit_manifest_exp1.cbor', [Symbol(type)]: 1 },
tam_api_1  |   Dirent { name: 'suit_manifest_expX.cbor', [Symbol(type)]: 1 },
tam_api_1  |   Dirent { name: 'tamproto.md', [Symbol(type)]: 1 }
tam_api_1  | ]

......

tam_api_1  |   'content-type': 'application/teep+cbor'
tam_api_1  | }
tam_api_1  | <Buffer 82 05 a1 14 48 77 77 77 77 77 77 77 77>
tam_api_1  | {
tam_api_1  |   TYPE: 5,
tam_api_1  |   token: <Buffer 77 77 77 77 77 77 77 77>,
tam_api_1  |   TOKEN: <Buffer 77 77 77 77 77 77 77 77>
tam_api_1  | }
tam_api_1  | TAM ProcessTeepMessage instance
tam_api_1  | TEEP-Protocol:parse
tam_api_1  | {
tam_api_1  |   TYPE: 5,
tam_api_1  |   token: <Buffer 77 77 77 77 77 77 77 77>,
tam_api_1  |   TOKEN: <Buffer 77 77 77 77 77 77 77 77>
tam_api_1  | }
tam_api_1  | object
tam_api_1  | *parseSuccessMessage
tam_api_1  | <Buffer 77 77 77 77 77 77 77 77>
tam_api_1  | undefined
tam_api_1  | TAM ProcessTeepMessage response
tam_api_1  | undefined
tam_api_1  | WARNING: Agent may sent invalid contents. TAM responses null.
tam_api_1  | POST /api/tam_cbor 204 1.294 ms - -
fgrep 'store component' /home/gitlab-runner/projects/teep-device/sample/../build/pc/pctest.log
store componen
```

