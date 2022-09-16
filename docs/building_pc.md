# Building TEEP-Device for PC without Docker

The building PC is prepared for debugging purposes during developing TEEP-Device itself. This method does not require any TEEs installed in the local machine and it is meant to build and run on TEEP-Device on any x64 PC.

To run TEEP-Device, first we need to run tamproto inside the same host. Let's clone the tamproto and start it.

**tamproto**

```sh
# Clone the tamproto repo and checkout master branch
$ git clone https://192.168.100.100/rinkai/tamproto.git
$ cd tamproto
$ git checkout master
$ docker-compose build
$ docker-compose up &
$ cd ..
```

Trimmed output of starting tamproto.

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

**TEEP-Device**

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
# After the successful build
# Run the TEEP-Device
$ make run-sample-session
```

Trimmed output of the run.
The TC can be found in /home/user/teep-device/platform/pc/build/8d82573a-926d-4754-9353-32dc29997f74.ta


```console
build-user@c4435c23705c:~/teep-device$ ls -l /home/user/teep-device/platform/pc/build/
total 40
-rw-r--r-- 1 build-user build-user   21 Feb 15 10:29 8d82573a-926d-4754-9353-32dc29997f74.ta
-rw-r--r-- 1 build-user build-user  734 Feb 15 10:29 hello-ta.json
-rw-r--r-- 1 build-user build-user  255 Feb 15 10:29 hello-ta.suit
drwxr-xr-x 4 build-user build-user 4096 Feb 15 10:28 libteep
-rw-r--r-- 1 build-user build-user 5198 Feb 15 10:29 pctest.log
drwxr-xr-x 2 build-user build-user 4096 Feb 15 10:28 scripts
-rw-r--r-- 1 build-user build-user  357 Feb 15 10:29 signed-hello-ta-payload.suit
-rw-r--r-- 1 build-user build-user  331 Feb 15 10:29 signed-hello-ta.suit
drwxr-xr-x 2 build-user build-user 4096 Feb 15 10:28 teep-broker-app

build-user@c4435c23705c:~/teep-device$ cat /home/user/teep-device/platform/pc/
build/8d82573a-926d-4754-9353-32dc29997f74.ta
Hello TEEP from TEE!
```

