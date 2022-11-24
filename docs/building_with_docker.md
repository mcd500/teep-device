# Build TEEP-Device with Docker

The benefit of Docker images is to provide the environment of building and developing TEEP-Device to reduce the overhead of preparing them individually.

The TEEP-Device requires TA-Ref which provides a unified SDK among different TEEs for three CPU architectures, Keystone for RISC-V, OP-TEE for Arm64 and SGX for Intel.

Without the prepared Docker images, the developer will be required to build a massing software stack of Keystone, OP-TEE and SGX and install them on his/her development machine which needs downloading large sizes of source codes, a long time for building them. Also it may result in every individual having a slightly different environment which makes it difficult to reproduce when encountering errors.

The Docker images provide an easy to prepare development environment for TEEP-Device.


## Preparation for Docker

To build the TEEP-Device with Docker, it is required to install Docker on Ubuntu.

For the first time users of Docker, please have a look on https://docs.docker.com/engine/.

The following installation steps is for Ubuntu 20.04

### Install Docker

```sh
$ sudo apt update

# Next, install a few prerequisite packages which let apt use packages over HTTPS:
$ sudo apt install apt-transport-https ca-certificates curl software-properties-common

# Then add the GPG key for the official Docker repository to your system:
$ curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -

# Add the Docker repository to APT sources:
$ sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu focal stable"

# This will also update our package database with the Docker packages from the newly added repo.
# Make sure you are about to install from the Docker repo instead of the default Ubuntu repo:
$ apt-cache policy docker-ce

#Finally, install Docker
$ sudo apt install docker-ce docker-compose
```

### Executing Docker without sudo

By default, the Docker command can only be run by the root user or by a user in the Docker group, which is automatically created during Docker's installation process. If you attempt to run the Docker command without prefixing it with sudo or without being in the Docker group, you will get an output like this:

```console
docker: Cannot connect to the Docker daemon. Is the docker daemon running on this host?.
```

To avoid typing sudo whenever we run the docker command, add your username to the docker group.

```sh
$ sudo groupadd docker

$ sudo gpasswd -a $USER docker

# Logout and then log-in again to apply the changes to the group
 ```

After you logout and login, you can probably run the Docker command without `sudo`.

```sh
$ docker run hello-world
```

Login to the docker to be able to access docker images.
Make sure you have an account on docker-hub. If not please create one on `dockerhub.com`.

```sh
$ docker login -u ${YOUR_USERNAME} -p ${YOUR_PASSWD}
```

### Create a Docker network tamproto

A Docker network named tamproto is required when we run TEEP-Device. The local network is required to connect with tamproto service running locally.

```sh
$ docker network create tamproto_default
```


## Docker Images with pre-built TEEP-Device

The following are the docker images that have pre-built binaries of TEEP-Device with TA-Ref. 
Since these images are already prepared and built already, you can start using it directly without building the TEEP-Device oneself.

| Purpose | Docker image | Description |
| ------ | ------ | ------ |
| Keystone | aistcpsec/teep-dev:keystone | Has pre-built binaries of TEEP-Device with TA-Ref for RISC-V Keystone|
| OP-TEE | aistcpsec/teep-dev:optee | Has pre-built binaries of TEEP-Device with TA-Ref for ARM OP-TEE|
| Intel SGX | aistcpsec/teep-dev:sgx | Has pre-built binaries of TEEP-Device with TA-Ref for Intel SSX|
| tamproto | aistcpsec/teep-dev:tamproto | Used for running tamproto |
| Doxygen | aistcpsec/teep-dev:doxygen | Used for generating TEEP-Device documentation|


## Preparation to build TEEP-Device on Docker

### List of Docker images to build TEEP-Device

It requires Docker images of TA-Ref for building the TEEP-Device since TEEP-Device is developed on top of TA-Ref SDK. The TEEP-Device is one of the applications of TA-Ref.

Docker images have all necessary development packages for building TEEP-Device for all three TEEs. The instructions of usage are described from the next chapters.

| Purpose | Docker image |
| ------ | ------ |
| Keystone | aistcpsec/taref-dev:keystone |
| OP-TEE | aistcpsec/taref-dev:optee |
| Intel SGX | aistcpsec/taref-dev:sgx |


## Run tamproto (TAM Server) - Required by all Keystone/OP-TEE/SGX

To run TEEP-Device, first we need to run tamproto inside the same host. Let's clone the tamproto and start it.

**Running tamproto**

Open the first terminal for the tamproto.

```sh
# Clone the tamproto repo and checkout master branch
$ git clone https://192.168.100.100/rinkai/tamproto.git
$ cd tamproto
$ git checkout fb1961bc964857384c9ed8696c0d5fc0a76a319d
$ docker-compose build
$ docker-compose up
```

Trimmed output of starting tamproto
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

### Build TEEP-Device for Keystone with Docker

**Clone TEEP-Device**

Open the second terminal for editing the sources of TEEP-Device. The directory of cloning sources is mounted when running Docker in the next step.

```sh
# Clone the teep-device repo and checkout master branch
$ git clone https://192.168.100.100/rinkai/teep-device.git
$ cd teep-device
$ git checkout master

# Sync and update the submodules
$ git submodule sync --recursive
$ git submodule update --init --recursive
```

Match the user privilege with the one used in the container to prevent permission errors when editing sources. Container uses a build-user account with 1000:1000.

```sh
# Return back to parent directory of teep-device
$ sudo chown -R 1000:1000 teep-device/
$ sudo chmod -R a+w teep-device/
$ git config --global --add safe.directory $(pwd)/teep-device
```

**Start the Docker**

Open the third terminal. Here we build the TEEP-Device and run it to communicate with tamproto opened on the first terminal. If any error occurs, edit the sources on the second terminal to debug.

```sh
# Change the directory into teep-device before starting the docker
# Start the docker
$ docker run --network tamproto_default -w /home/user/teep-device -it --rm -v $(pwd):/home/user/teep-device aistcpsec/taref-dev:keystone
```

After you start the Docker command, you will be logged-in inside the Docker container.
Following are the  commands to be executed inside the Docker.


**Build**

```sh
# [Inside docker image]

# Change to teep-device
$ cd ~/teep-device/

# Build the teep-device
$ make

# Trimmed output of make command
....
make -C sample rootfs TAM_URL=http://tamproto_tam_api_1:8888
make[1]: Entering directory '/home/user/teep-device/sample'
cp /home/user/keystone/build/buildroot.build/images/rootfs.ext2
	 /home/user/teep-device/sample/../build/keystone/rootfs.ext2
e2mkdir -O root -G root /home/user/teep-device/sample/../build/keystone/rootfs.ext2:/root/teep-broker
e2cp -O root -G root -p /home/user/teep-device/sample/../build/keystone/hello-tc/App-keystone
		 /home/user/teep-device/sample/../build/keystone/rootfs.ext2:/root/teep-broker/hello-app
e2cp -O root -G root -p /home/user/teep-device/sample/../build/keystone/agent/teep-agent-ta 
		/home/user/teep-device/sample/../build/keystone/rootfs.ext2:/root/teep-broker
e2cp -O root -G root -p /home/user/teep-device/sample/../build/keystone/broker/teep-broker-app 
		/home/user/teep-device/sample/../build/keystone/rootfs.ext2:/root/teep-broker
e2cp -O root -G root -p /home/user/teep-device/sample/../build/keystone/scripts/env.sh 
		/home/user/teep-device/sample/../build/keystone/rootfs.ext2:/root/teep-broker
e2cp -O root -G root -p /home/user/teep-device/sample/../build/keystone/scripts/itc.sh 
		/home/user/teep-device/sample/../build/keystone/rootfs.ext2:/root/teep-broker
e2cp -O root -G root -p /home/user/teep-device/sample/../build/keystone/scripts/rtc.sh 
		/home/user/teep-device/sample/../build/keystone/rootfs.ext2:/root/teep-broker
e2cp -O root -G root -p /home/user/teep-device/sample/../build/keystone/scripts/showtamurl.sh 
		/home/user/teep-device/sample/../build/keystone/rootfs.ext2:/root/teep-broker
e2cp -O root -G root -p /home/user/teep-device/sample/../build/keystone/scripts/get-ip.sh 
		/home/user/teep-device/sample/../build/keystone/rootfs.ext2:/root/teep-broker
e2cp -O root -G root -p /home/user/teep-device/sample/../build/keystone/scripts/cp_ta_to_tamproto.sh 
		/home/user/teep-device/sample/../build/keystone/rootfs.ext2:/root/teep-broker
e2cp -O root -G root -p /home/user/keystone/sdk/build64/runtime/eyrie-rt 
		/home/user/teep-device/sample/../build/keystone/rootfs.ext2:/root/teep-broker
e2cp -O root -G root -p /home/user/teep-device/sample/../build/keystone/ree/mbedtls/library/lib* 
		/home/user/teep-device/sample/../build/keystone/rootfs.ext2:/usr/lib
e2cp -O root -G root -p /home/user/teep-device/sample/../build/keystone/ree/libwebsockets/lib/lib* 
		/home/user/teep-device/sample/../build/keystone/rootfs.ext2:/usr/lib

make[1]: Leaving directory '/home/user/teep-device/sample'

```

**Run manually**

After the successful build, run the sample TEEP session with tamproto.

Launch qemu of RISC-V with Keystone.
The password for root is 'sifive'

```sh
$ make run-qemu

make -C sample run-qemu TAM_URL=http://tamproto_tam_api_1:8888
make[1]: Entering directory '/home/user/teep-device/sample'
qemu-system-riscv64 \
	-m 4G \
	-bios /home/user/keystone/build/bootrom.build/bootrom.bin \
	-nographic \
	-machine virt \
	-kernel /home/user/keystone/build/sm.build/platform/generic/firmware/fw_payload.elf \
	-append "console=ttyS0 ro root=/dev/vda cma=256M@0x00000000C0000000" \
	-device virtio-blk-device,drive=hd0
	-drive file=/home/user/teep-device/sample/../build/keystone/rootfs.ext2,format=raw,id=hd0 \
	-netdev user,id=net0,net=192.168.100.1/24,dhcpstart=192.168.100.128,hostfwd=tcp::10032-:22 \
	-device virtio-net-device,netdev=net0 \
	-device virtio-rng-pci
overriding secure boot ROM (file: /home/user/keystone/build/bootrom.build/bootrom.bin)
boot ROM size: 53869
fdt dumped at 57968

OpenSBI v0.8

.....

Starting dropbear sshd: OK

Welcome to Buildroot
buildroot login: root
Password: 
#
```

Please enter login username and password as root and sifive.


After login, start from installing the driver for Keystone.

```sh
# ls
keystone-driver.ko  teep-broker         tests.ke
# cd teep-broker/
# ls
cp_ta_to_tamproto.sh  hello-app             showtamurl.sh
env.sh                hello-ta              teep-agent-ta
eyrie-rt              itc.sh                teep-broker-app
get-ip.sh             rtc.sh
# source env.sh
[  388.139452] keystone_driver: loading out-of-tree module taints kernel.
[  388.146850] keystone_enclave: keystone enclave v1.0.0

```

There are helper scripts to handle the teep-broker.
Following are the few of them and its usage.

* showtamurl.sh
* itc.sh
* rtc.sh


*showtamurl.sh*

This script prints out the tamproto values which has to be suffixed when we execute the built teep-broker-app.
This script gets the url of the Tamproto either from the TAM_URL env variable or by internally executing get-ip.sh (get-ip.sh returns the IP of tamproto running in the same machine) 

example:

```sh
$ ./showtamurl.sh 
--tamurl 192.168.100.114/api/tam_cbor
```

You can simply copy the output of the showtamurl.sh and paste it to the end of the generated teep-broker-app binary.For ex:

```sh
$ ./teep-broker-app --tamurl http://192.168.100.114:8888/api/tam_cbor
```

*itc.sh*

Initiate teep-agent with tamproto. This script is for debugging the confirmative and handling of formats of TEEP Messages and SUIT Manifest in teep-agent and tamproto.
Make sure you have copied the below files into tamproto server
before running the ./teep-broker-app.

1) build/keystone/hello-tc/8d82573a-926d-4754-9353-32dc29997f74.ta  
2) build/keystone/hello-tc/signed-download-tc.suit as integrated-payload-manifest.cbor

```sh
# cat ita.sh
#!/bin/bash -x

./teep-broker-app --tamurl ${TAM_URL}/api/tam_cbor
# ./itc.sh
```

*rtc.sh*

Execute the downloaded TC from the tamproto. This script is for debugging the implementation of the TC.

```sh
# ./rtc.sh
+ echo Running downloaded TC from the TAM
Running downloaded TC from the TAM
+ ./hello-app 8d82573a-926d-4754-9353-32dc29997f74.ta eyrie-rt
[debug] UTM : 0xffffffff80000000-0xffffffff80100000 (1024 KB) (boot.c:127)
[debug] DRAM: 0x179800000-0x179c00000 (4096 KB) (boot.c:128)
[debug] FREE: 0x1799bd000-0x179c00000 (2316 KB), va 0xffffffff001bd000 (boot.c:133)
[debug] eyrie boot finished. drop to the user land ... (boot.c:172)
main start
Hello TEEP from TEE!
main end
```


To exit from qemu.

```sh
# poweroff
```

The log message of tamproto will be shown on the terminal of running tamproto.
```
tam_api_1  | POST /api/tam_cbor 200 2.816 ms - 399
tam_api_1  | Access from: ::ffff:172.18.0.3
tam_api_1  | {
tam_api_1  |   pragma: 'no-cache',
tam_api_1  |   'cache-control': 'no-cache',
tam_api_1  |   host: 'example.com',
tam_api_1  |   origin: 'http://192.168.11.3',
tam_api_1  |   connection: 'close',
tam_api_1  |   accept: 'application/teep+cbor',
tam_api_1  |   'content-length': '13',
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
tam_api_1  | POST /api/tam_cbor 204 1.357 ms - -
```

These are trimmed outputs of all procedures above on the terminal of the running container.

```sh
build-user@86417bb9c512:~/teep-device$ make run-qemu
make -C sample run-qemu TAM_URL=http://tamproto_tam_api_1:8888
make[1]: Entering directory '/home/user/teep-device/sample'
qemu-system-riscv64 \
	-m 4G \
	-bios /home/user/keystone/build/bootrom.build/bootrom.bin \
	-nographic \
	-machine virt \
	-kernel /home/user/keystone/build/sm.build/platform/generic/firmware/fw_payload.elf \
	-append "console=ttyS0 ro root=/dev/vda cma=256M@0x00000000C0000000" \
	-device virtio-blk-device,drive=hd0 -drive
	file=/home/user/teep-device/sample/../build/keystone/rootfs.ext2,format=raw,id=hd0 \
	-netdev user,id=net0,net=192.168.100.1/24,dhcpstart=192.168.100.128,hostfwd=tcp::10032-:22 \
	-device virtio-net-device,netdev=net0 \
	-device virtio-rng-pci
overriding secure boot ROM (file: /home/user/keystone/build/bootrom.build/bootrom.bin)
boot ROM size: 53869
fdt dumped at 57968

OpenSBI v0.8
   ____                    _____ ____ _____
  / __ \                  / ____|  _ \_   _|
 | |  | |_ __   ___ _ __ | (___ | |_) || |
 | |  | | '_ \ / _ \ '_ \ \___ \|  _ < | |
 | |__| | |_) |  __/ | | |____) | |_) || |_
  \____/| .__/ \___|_| |_|_____/|____/_____|
        | |
        |_|

Platform Name             : riscv-virtio,qemu
Platform Features         : timer,mfdeleg
Platform HART Count       : 1
Firmware Base             : 0x80000000
Firmware Size             : 204 KB
Runtime SBI Version       : 0.2

Domain0 Name              : root
Domain0 Boot HART         : 0
Domain0 HARTs             : 0*
Domain0 Region00          : 0x0000000080000000-0x000000008003ffff ()
Domain0 Region01          : 0x0000000000000000-0xffffffffffffffff (R,W,X)
Domain0 Next Address      : 0x0000000080200000
Domain0 Next Arg1         : 0x0000000082200000
Domain0 Next Mode         : S-mode
Domain0 SysReset          : yes
...
...
...
Starting syslogd: OK
Starting klogd: OK
Running sysctl: OK
Saving random seed: OK
Starting network: udhcpc: started, v1.32.0
udhcpc: sending discover
udhcpc: sending select for 192.168.100.128
udhcpc: lease of 192.168.100.128 obtained, lease time 86400
deleting routers
adding dns 192.168.100.3
OK
Starting dropbear sshd: OK

Welcome to Buildroot
buildroot login: root
Password: 
# 
# 
# ls
keystone-driver.ko  teep-broker         tests.ke
# cd teep-broker/
# source env.sh 
[   18.953988] keystone_driver: loading out-of-tree module taints kernel.
[   18.960803] keystone_enclave: keystone enclave v1.0.0
# 
# ./itc.sh  
+ ./teep-broker-app --tamurl http://tamproto_tam_api_1:8888/api/tam_cbor
teep-broker.c compiled at Nov 24 2022 05:19:23
uri = http://tamproto_tam_api_1:8888/api/tam_cbor, cose=0, talist=
[debug] UTM : 0xffffffff80000000-0xffffffff80100000 (1024 KB) (boot.c:127)
[debug] DRAM: 0x179800000-0x179c00000 (4096 KB) (boot.c:128)
[debug] FREE: 0x1799f2000-0x179c00000 (2104 KB), va 0xffffffff001f2000 (boot.c:133)
[debug] eyrie boot finished. drop to the user land ... (boot.c:172)
[1970/01/01 00:00:37:6062] N: POST: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:37:6073] N: (hexdump: zero length)
[1970/01/01 00:00:37:6117] N: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:37:6716] N: 
[1970/01/01 00:00:37:6725] N: 0000: 83 01 A5 01 81 01 03 81 00 04 43 01 02 05 14 48    ..........C....H
[1970/01/01 00:00:37:6731] N: 0010: 77 77 77 77 77 77 77 77 15 81 00 02                wwwwwwww....    
[1970/01/01 00:00:37:6738] N: 
[1970/01/01 00:00:37:6829] N: POST: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:37:6843] N: 
[1970/01/01 00:00:37:6849] N: 0000: 82 02 A4 14 48 77 77 77 77 77 77 77 77 08 80 0E    ....Hwwwwwwww...
[1970/01/01 00:00:37:6856] N: 0010: 80 0F 80                                           ...             
[1970/01/01 00:00:37:6862] N: 
[1970/01/01 00:00:37:6907] N: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:37:7111] N: 
[1970/01/01 00:00:37:7115] N: 0000: 82 03 A2 0A 81 59 01 66 D8 6B A2 02 58 73 82 58    .....Y.f.k..Xs.X
[1970/01/01 00:00:37:7123] N: 0010: 24 82 2F 58 20 63 70 90 82 1C BB B2 67 95 42 78    $./X cp.....g.Bx
[1970/01/01 00:00:37:7129] N: 0020: 7B 49 F4 5E 14 AF 0C BF AD 9E F4 A4 F0 B3 42 B9    {I.^..........B.
[1970/01/01 00:00:37:7135] N: 0030: 23 35 56 05 AF 58 4A D2 84 43 A1 01 26 A0 F6 58    #5V..XJ..C..&..X
[1970/01/01 00:00:37:7142] N: 0040: 40 BF B8 79 C8 E0 9B DE 52 2E 84 38 10 D7 7A 2E    @..y....R..8..z.
[1970/01/01 00:00:37:7149] N: 0050: AF 88 62 53 C1 C1 83 D6 A2 FD 9E A1 DA 6F 4A D1    ..bS.........oJ.
[1970/01/01 00:00:37:7156] N: 0060: DD 16 84 72 BC D6 10 81 F0 AE 30 A8 05 36 91 0E    ...r......0..6..
[1970/01/01 00:00:37:7163] N: 0070: C9 D7 63 90 B4 E9 C9 64 A1 C3 6C F0 FE 29 71 91    ..c....d..l..)q.
[1970/01/01 00:00:37:7170] N: 0080: 84 03 58 EA A5 01 01 02 01 03 58 86 A2 02 81 84    ..X.......X.....
[1970/01/01 00:00:37:7176] N: 0090: 4B 54 45 45 50 2D 44 65 76 69 63 65 48 53 65 63    KTEEP-DeviceHSec
[1970/01/01 00:00:37:7182] N: 00A0: 75 72 65 46 53 50 8D 82 57 3A 92 6D 47 54 93 53    ureFSP..W:.mGT.S
[1970/01/01 00:00:37:7188] N: 00B0: 32 DC 29 99 7F 74 42 74 61 04 58 56 86 14 A4 01    2.)..tBta.XV....
[1970/01/01 00:00:37:7194] N: 00C0: 50 FA 6B 4A 53 D5 AD 5F DF BE 9D E6 63 E4 D4 1F    P.kJS.._....c...
[1970/01/01 00:00:37:7201] N: 00D0: FE 02 50 14 92 AF 14 25 69 5E 48 BF 42 9B 2D 51    ..P....%i^H.B.-Q
[1970/01/01 00:00:37:7207] N: 00E0: F2 AB 45 03 58 24 82 2F 58 20 00 11 22 33 44 55    ..E.X$./X ..3DU
[1970/01/01 00:00:37:7213] N: 00F0: 66 77 88 99 AA BB CC DD EE FF 01 23 45 67 89 AB    fw.........#Eg..
[1970/01/01 00:00:37:7219] N: 0100: CD EF FE DC BA 98 76 54 32 10 0E 19 87 D0 01 0F    ......vT2.......
[1970/01/01 00:00:37:7226] N: 0110: 02 0F 09 58 54 86 13 A1 15 78 4A 68 74 74 70 3A    ...XT....xJhttp:
[1970/01/01 00:00:37:7233] N: 0120: 2F 2F 74 61 6D 70 72 6F 74 6F 5F 74 61 6D 5F 61    //tamproto_tam_a
[1970/01/01 00:00:37:7239] N: 0130: 70 69 5F 31 3A 38 38 38 38 2F 54 41 73 2F 38 64    pi_1:8888/TAs/8d
[1970/01/01 00:00:37:7246] N: 0140: 38 32 35 37 33 61 2D 39 32 36 64 2D 34 37 35 34    82573a-926d-4754
[1970/01/01 00:00:37:7253] N: 0150: 2D 39 33 35 33 2D 33 32 64 63 32 39 39 39 37 66    -9353-32dc29997f
[1970/01/01 00:00:37:7259] N: 0160: 37 34 2E 74 61 15 02 03 0F 0A 43 82 03 0F 14 48    74.ta.....C....H
[1970/01/01 00:00:37:7265] N: 0170: AB A1 A2 A3 A4 A5 A6 A7                            ........        
[1970/01/01 00:00:37:7272] N: 
TTRC:verifying signature of suit manifest
TTRC:verify OK
TTRC:command: 20
TTRC:execute suit-set-parameters
TTRC:command: 1
TTRC:execute suit-condition-vendor-identifier
TTRC:command: 2
TTRC:execute suit-condition-class-identifier
TTRC:command: 19
TTRC:execute suit-set-parameters
TTRC:command: 21
TTRC:execute suit-directive-fetch
TTRC:fetch_and_store component
[1970/01/01 00:00:38:3479] N: GET: http://tamproto_tam_api_1:8888/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
[1970/01/01 00:00:38:3497] N: http://tamproto_tam_api_1:8888/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
TTRC:component download 153128
TTRC:ta-store.c: store_component() store component
TTRC:  device   = TEEP-Device
TTRC:  storage  = SecureFS
TTRC:  filename = 8d82573a-926d-4754-9353-32dc29997f74.ta
TTRC:  image_len = 153128
TTRC:finish fetch
TTRC:command: 3
TTRC:execute suit-condition-image-match
TTRC:end of command seq
[1970/01/01 00:00:38:8690] N: POST: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:38:8695] N: 
[1970/01/01 00:00:38:8699] N: 0000: 82 05 A1 14 48 77 77 77 77 77 77 77 77             ....Hwwwwwwww   
[1970/01/01 00:00:38:8704] N: 
[1970/01/01 00:00:38:8714] N: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:38:8873] N: (hexdump: zero length)
# 
# ./rtc.sh 
+ echo Running downloaded TC from the TAM
Running downloaded TC from the TAM
+ ./hello-app 8d82573a-926d-4754-9353-32dc29997f74.ta eyrie-rt
[debug] UTM : 0xffffffff80000000-0xffffffff80100000 (1024 KB) (boot.c:127)
[debug] DRAM: 0x179800000-0x179c00000 (4096 KB) (boot.c:128)
[debug] FREE: 0x1799bd000-0x179c00000 (2316 KB), va 0xffffffff001bd000 (boot.c:133)
[debug] eyrie boot finished. drop to the user land ... (boot.c:172)
main start
Hello TEEP from TEE!
main end
# 
# poweroff
# Stopping dropbear sshd: OK
Stopping network: OK
Saving random seed: OK
Stopping klogd: OK
Stopping syslogd: OK
umount: devtmpfs busy - remounted read-only
[  177.941806] EXT4-fs (vda): re-mounted. Opts: (null)
The system is going down NOW!
logout
Sent SIGTERM to all processes
Sent SIGKILL to all processes
Requesting system poweroff
[  179.965743] reboot: Power down
make[1]: Leaving directory '/home/user/teep-device/sample'
```


**Run automatically**

This command will run all the previous manual procedures. It is mainly prepared for running TEEP-Device in CI.

```sh
$ make run-sample-session
```

Trimmed output printing 'Hello TEEP from TEE!'

```sh
Welcome to Buildroot
buildroot login: root
Password: sifive

# PS1='##''## '
#### insmod keystone-driver.ko || echo 'err''or'
[    5.247409] keystone_driver: loading out-of-tree module taints kernel.
[    5.254006] keystone_enclave: keystone enclave v1.0.0
#### cd /root/teep-broker
#### ls -l
total 3422
-rwxr-xr-x    1 1000     1000           567 Nov 24  2022 cp_ta_to_tamproto.sh
-rwxr-xr-x    1 1000     1000           156 Nov 24  2022 env.sh
-rwxr-xr-x    1 1000     1000         98088 Nov 21  2022 eyrie-rt
-rwxr-xr-x    1 1000     1000           290 Nov 24  2022 get-ip.sh
-rwxr-xr-x    1 1000     1000        437656 Nov 24  2022 hello-app
-rwxr-xr-x    1 1000     1000        153128 Nov 24  2022 hello-ta
-rwxr-xr-x    1 1000     1000            65 Nov 24  2022 itc.sh
-rwxr-xr-x    1 1000     1000           116 Nov 24  2022 rtc.sh
-rwxr-xr-x    1 1000     1000           134 Nov 24  2022 showtamurl.sh
-rwxr-xr-x    1 1000     1000        415280 Nov 24  2022 teep-agent-ta
-rwxr-xr-x    1 1000     1000       2372112 Nov 24  2022 teep-broker-app
#### ./hello-app hello-ta eyrie-rt
[debug] UTM : 0xffffffff80000000-0xffffffff80100000 (1024 KB) (boot.c:127)
[debug] DRAM: 0x179800000-0x179c00000 (4096 KB) (boot.c:128)
[debug] FREE: 0x1799bd000-0x179c00000 (2316 KB), va 0xffffffff001bd000 (boot.c:133)
[debug] eyrie boot finished. drop to the user land ... (boot.c:172)
main start
Hello TEEP from TEE!
main end
#### ./hello-app 8d82573a-926d-4754-9353-32dc29997f74.ta eyrie-rt
[Keystone SDK] /home/user/keystone/sdk/src/host/ElfFile.cpp:26 : file does not exist - 8d82573a-926d-4754-9353-32dc29997f74.ta
[Keystone SDK] /home/user/keystone/sdk/src/host/Enclave.cpp:209 : Invalid enclave ELF

./hello-app: Unable to start enclave
#### ./teep-broker-app --tamurl http://tamproto_tam_api_1:8888/api/tam_cbor
teep-broker.c compiled at Nov 24 2022 05:09:43
uri = http://tamproto_tam_api_1:8888/api/tam_cbor, cose=0, talist=
[debug] UTM : 0xffffffff80000000-0xffffffff80100000 (1024 KB) (boot.c:127)
[debug] DRAM: 0x179800000-0x179c00000 (4096 KB) (boot.c:128)
[debug] FREE: 0x1799f2000-0x179c00000 (2104 KB), va 0xffffffff001f2000 (boot.c:133)
[debug] eyrie boot finished. drop to the user land ... (boot.c:172)
[1970/01/01 00:00:08:4238] N: POST: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:08:4285] N: (hexdump: zero length)
[1970/01/01 00:00:08:4349] N: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:08:5193] N: 
[1970/01/01 00:00:08:5203] N: 0000: 83 01 A5 01 81 01 03 81 00 04 43 01 02 05 14 48    ..........C....H
[1970/01/01 00:00:08:5211] N: 0010: 77 77 77 77 77 77 77 77 15 81 00 02                wwwwwwww....    
[1970/01/01 00:00:08:5217] N: 
[1970/01/01 00:00:08:5347] N: POST: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:08:5354] N: 
[1970/01/01 00:00:08:5364] N: 0000: 82 02 A4 14 48 77 77 77 77 77 77 77 77 08 80 0E    ....Hwwwwwwww...
[1970/01/01 00:00:08:5373] N: 0010: 80 0F 80                                           ...             
[1970/01/01 00:00:08:5380] N: 
[1970/01/01 00:00:08:5427] N: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:08:5735] N: 
[1970/01/01 00:00:08:5741] N: 0000: 82 03 A2 0A 81 59 01 66 D8 6B A2 02 58 73 82 58    .....Y.f.k..Xs.X
[1970/01/01 00:00:08:5748] N: 0010: 24 82 2F 58 20 63 70 90 82 1C BB B2 67 95 42 78    $./X cp.....g.Bx
[1970/01/01 00:00:08:5756] N: 0020: 7B 49 F4 5E 14 AF 0C BF AD 9E F4 A4 F0 B3 42 B9    {I.^..........B.
[1970/01/01 00:00:08:5763] N: 0030: 23 35 56 05 AF 58 4A D2 84 43 A1 01 26 A0 F6 58    #5V..XJ..C..&..X
[1970/01/01 00:00:08:5770] N: 0040: 40 4C 09 82 3E 54 8D 4B 51 23 A7 68 34 2F 65 3F    @L..>T.KQ#.h4/e?
[1970/01/01 00:00:08:5778] N: 0050: CE 7E F1 8D 0A C0 24 19 2E AD D7 0C 67 6C 81 25    .~....$.....gl.%
[1970/01/01 00:00:08:5785] N: 0060: FF A3 37 23 17 2C FB B7 67 73 45 88 70 13 DF A1    ..7#.,..gsE.p...
[1970/01/01 00:00:08:5792] N: 0070: 1D 74 8C D3 14 03 B7 7C 84 40 46 D4 66 9E 37 44    .t.....|.@F.f.7D
[1970/01/01 00:00:08:5801] N: 0080: FE 03 58 EA A5 01 01 02 01 03 58 86 A2 02 81 84    ..X.......X.....
[1970/01/01 00:00:08:5808] N: 0090: 4B 54 45 45 50 2D 44 65 76 69 63 65 48 53 65 63    KTEEP-DeviceHSec
[1970/01/01 00:00:08:5815] N: 00A0: 75 72 65 46 53 50 8D 82 57 3A 92 6D 47 54 93 53    ureFSP..W:.mGT.S
[1970/01/01 00:00:08:5822] N: 00B0: 32 DC 29 99 7F 74 42 74 61 04 58 56 86 14 A4 01    2.)..tBta.XV....
[1970/01/01 00:00:08:5829] N: 00C0: 50 FA 6B 4A 53 D5 AD 5F DF BE 9D E6 63 E4 D4 1F    P.kJS.._....c...
[1970/01/01 00:00:08:5836] N: 00D0: FE 02 50 14 92 AF 14 25 69 5E 48 BF 42 9B 2D 51    ..P....%i^H.B.-Q
[1970/01/01 00:00:08:5845] N: 00E0: F2 AB 45 03 58 24 82 2F 58 20 00 11 22 33 44 55    ..E.X$./X .."3DU
[1970/01/01 00:00:08:5852] N: 00F0: 66 77 88 99 AA BB CC DD EE FF 01 23 45 67 89 AB    fw.........#Eg..
[1970/01/01 00:00:08:5860] N: 0100: CD EF FE DC BA 98 76 54 32 10 0E 19 87 D0 01 0F    ......vT2.......
[1970/01/01 00:00:08:5868] N: 0110: 02 0F 09 58 54 86 13 A1 15 78 4A 68 74 74 70 3A    ...XT....xJhttp:
[1970/01/01 00:00:08:5876] N: 0120: 2F 2F 74 61 6D 70 72 6F 74 6F 5F 74 61 6D 5F 61    //tamproto_tam_a
[1970/01/01 00:00:08:5885] N: 0130: 70 69 5F 31 3A 38 38 38 38 2F 54 41 73 2F 38 64    pi_1:8888/TAs/8d
[1970/01/01 00:00:08:5893] N: 0140: 38 32 35 37 33 61 2D 39 32 36 64 2D 34 37 35 34    82573a-926d-4754
[1970/01/01 00:00:08:5900] N: 0150: 2D 39 33 35 33 2D 33 32 64 63 32 39 39 39 37 66    -9353-32dc29997f
[1970/01/01 00:00:08:5907] N: 0160: 37 34 2E 74 61 15 02 03 0F 0A 43 82 03 0F 14 48    74.ta.....C....H
[1970/01/01 00:00:08:5914] N: 0170: AB A1 A2 A3 A4 A5 A6 A7                            ........        
[1970/01/01 00:00:08:5923] N: 
TTRC:verifying signature of suit manifest
TTRC:verify OK
TTRC:command: 20
TTRC:execute suit-set-parameters
TTRC:command: 1
TTRC:execute suit-condition-vendor-identifier
TTRC:command: 2
TTRC:execute suit-condition-class-identifier
TTRC:command: 19
TTRC:execute suit-set-parameters
TTRC:command: 21
TTRC:execute suit-directive-fetch
TTRC:fetch_and_store component
[1970/01/01 00:00:09:3454] N: GET: http://tamproto_tam_api_1:8888/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
[1970/01/01 00:00:09:3475] N: http://tamproto_tam_api_1:8888/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
TTRC:component download 153128
TTRC:ta-store.c: store_component() store component
TTRC:  device   = TEEP-Device
TTRC:  storage  = SecureFS
TTRC:  filename = 8d82573a-926d-4754-9353-32dc29997f74.ta
TTRC:  image_len = 153128
TTRC:finish fetch
TTRC:command: 3
TTRC:execute suit-condition-image-match
TTRC:end of command seq
[1970/01/01 00:00:09:9560] N: POST: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:09:9565] N: 
[1970/01/01 00:00:09:9569] N: 0000: 82 05 A1 14 48 77 77 77 77 77 77 77 77             ....Hwwwwwwww   
[1970/01/01 00:00:09:9574] N: 
[1970/01/01 00:00:09:9583] N: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:09:9754] N: (hexdump: zero length)
#### ls -l
total 3573
-rw-------    1 root     root        153128 Jan  1 00:00 8d82573a-926d-4754-9353-32dc29997f74.ta
-rwxr-xr-x    1 1000     1000           567 Nov 24  2022 cp_ta_to_tamproto.sh
-rwxr-xr-x    1 1000     1000           156 Nov 24  2022 env.sh
-rwxr-xr-x    1 1000     1000         98088 Nov 21  2022 eyrie-rt
-rwxr-xr-x    1 1000     1000           290 Nov 24  2022 get-ip.sh
-rwxr-xr-x    1 1000     1000        437656 Nov 24  2022 hello-app
-rwxr-xr-x    1 1000     1000        153128 Nov 24  2022 hello-ta
-rwxr-xr-x    1 1000     1000            65 Nov 24  2022 itc.sh
-rwxr-xr-x    1 1000     1000           116 Nov 24  2022 rtc.sh
-rwxr-xr-x    1 1000     1000           134 Nov 24  2022 showtamurl.sh
-rwxr-xr-x    1 1000     1000        415280 Nov 24  2022 teep-agent-ta
-rwxr-xr-x    1 1000     1000       2372112 Nov 24  2022 teep-broker-app
#### ./hello-app 8d82573a-926d-4754-9353-32dc29997f74.ta eyrie-rt
[debug] UTM : 0xffffffff80000000-0xffffffff80100000 (1024 KB) (boot.c:127)
[debug] DRAM: 0x179800000-0x179c00000 (4096 KB) (boot.c:128)
[debug] FREE: 0x1799bd000-0x179c00000 (2316 KB), va 0xffffffff001bd000 (boot.c:133)
[debug] eyrie boot finished. drop to the user land ... (boot.c:172)
main start
Hello TEEP from TEE!
main end
####  done
```

Cleaning built binaries. Deleting the built binaries are required when starting to build TEEP-Device on other CPU architectures otherwise will generate errors.

```sh
$ make clean
```


### Build TEEP-Device for OP-TEE with Docker

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

**Start the Docker**

```sh
# Start the Docker
$ docker run --network tamproto_default -w /home/user/teep-device -it --rm -v $(pwd):/home/user/teep-device aistcpsec/taref-dev:optee
```

After you start the Docker command, you will be logged-in inside the Docker container.
Following are the commands to be executed inside the Docker.

```sh
# [Inside docker image]

# Change to teep-device
$ cd ~/teep-device/

# Build the teep device
$ make
```

After the successful build, run the sample TEEP session with tamproto.

```sh
# After the successful build
# Run the TEEP-Device
$ make run-sample-session
```

Trimmed output of the TEEP-Device

```console
M/TA: command: 20
M/TA: execute suit-set-parameters
M/TA: command: 1
M/TA: execute suit-condition-vendor-identifier
M/TA: command: 2
M/TA: execute suit-condition-class-identifier
M/TA: command: 19
M/TA: execute suit-set-parameters
M/TA: command: 21
M/TA: execute suit-directive-fetch
M/TA: fetch_and_store component
M/TA: component download 55976
M/TA: store component
M/TA:   device   = TEEP-Device
M/TA:   storage  = SecureFS
M/TA:   filename = 8d82573a-926d-4754-9353-32dc29997f74.ta
D/TC:? 0 tee_ta_init_pseudo_ta_session:283 Lookup pseudo TA 6e256cba-fc4d-4941-ad09-2ca1860342dd
D/TC:? 0 tee_ta_init_pseudo_ta_session:296 Open secstor_ta_mgmt
D/TC:? 0 tee_ta_init_pseudo_ta_session:310 secstor_ta_mgmt : 6e256cba-fc4d-4941-ad09-2ca1860342dd
D/TC:? 0 install_ta:99 Installing 8d82573a-926d-4754-9353-32dc29997f74
D/TC:? 0 tee_ta_close_session:499 csess 0xc09491c0 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session
M/TA: finish fetch
M/TA: command: 3
M/TA: execute suit-condition-image-match
M/TA: end of command seq
D/TC:? 0 tee_ta_close_session:499 csess 0xc094ab40 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session
D/TC:? 0 destroy_context:298 Destroy TA ctx (0xc094aae0)
D/TC:? 0 tee_ta_init_pseudo_ta_session:283 Lookup pseudo TA 8d82573a-926d-4754-9353-32dc29997f74
D/TC:? 0 load_ldelf:704 ldelf load address 0x40006000
D/LD:  ldelf:134 Loading TA 8d82573a-926d-4754-9353-32dc29997f74
D/TC:? 0 tee_ta_init_session_with_context:573 Re-open TA 3a2f8978-5dc0-11e8-9c2d-fa7ae01bbebc
D/TC:? 0 system_open_ta_binary:257 Lookup user TA ELF 8d82573a-926d-4754-9353-32dc29997f74 (Secure Storage TA)
D/TC:? 0 system_open_ta_binary:260 res=0x0
D/LD:  ldelf:169 ELF (8d82573a-926d-4754-9353-32dc29997f74) at 0x40066000
D/TC:? 0 tee_ta_close_session:499 csess 0xc0948820 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session

Hello TEEP from TEE!

D/TC:? 0 tee_ta_close_session:499 csess 0xc0949020 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session
D/TC:? 0 destroy_context:298 Destroy TA ctx (0xc0948fc0)
make[1]: Leaving directory '/home/user/teep-device/platform/op-tee'
```

Cleaning built binaries. Deleting the built binaries are required when starting to build TEEP-Device on other CPU architectures otherwise will generate errors.

```sh
$ make clean
```


### Build TEEP-Device for SGX with Docker

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

**Start the Docker**

```sh
# Start the Docker
$ docker run --network tamproto_default -w /home/user/teep-device -it --rm -v $(pwd):/home/user/teep-device aistcpsec/taref-dev:sgx
```

After you start the Docker command, you will be logged-in inside the Docker container.
Following are the commands to be executed inside the Docker

```sh
# [Inside docker image]

# Change to teep-device
$ cd ~/teep-device/

# set the TEE environments for SGX
# The MACHINE=SIM specifies running SGX in simulation mode which
# will allow running SGX on all Intel and AMD cpu regardless of SGX support.
$ export MACHINE=SIM

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

```console
-rw-r--r-- 1 build-user build-user 326986 Sep  8 07:01 embed-tc.suit
-rw-r--r-- 1 build-user build-user    209 Sep  8 07:01 embed-tc.suit.tmp
-rw-r--r-- 1 build-user build-user    690 Sep  8 07:01 embed.json
-rw-rw-rw- 1 root       root         1901 Sep  8 07:00 enclave.mk
-rw-r--r-- 1 build-user build-user 326768 Sep  8 07:01 enclave.signed.so
-rwxr-xr-x 1 build-user build-user 326768 Sep  8 07:01 enclave.so
-rw-r--r-- 1 build-user build-user    351 Sep  8 07:01 signed-download-tc.suit
-rw-r--r-- 1 build-user build-user 327062 Sep  8 07:01 signed-embed-tc.suit
cd /builds/rinkai/teep-device/sample/../build/sgx/agent && \
    ../broker/teep-broker-app --tamurl http://172.17.0.26:8888/api/tam_cbor | \
    tee /builds/rinkai/teep-device/sample/../build/sgx/sgx.log
[CEnclavePool /home/user/linux-sgx/psw/urts/enclave.cpp:627] enter CEnclavePool constructor
[build_secs /home/user/linux-sgx/psw/urts/loader.cpp:516] Enclave start addr. = 0x7f011e616000, Size = 0x8000000, 131072 KB
verifying signature of suit manifest
verify OK
command: 20
execute suit-set-parameters
command: 1
execute suit-condition-vendor-identifier
command: 2
execute suit-condition-class-identifier
command: 19
execute suit-set-parameters
command: 21
execute suit-directive-fetch
fetch_and_store component
component download 326768
store component
  device   = TEEP-Device
  storage  = SecureFS
  filename = 8d82573a-926d-4754-9353-32dc29997f74.ta
finish fetch
command: 3
execute suit-condition-image-match
end of command seq
cd /builds/rinkai/teep-device/sample/../build/sgx/../../hello-tc/build-sgx/ && \
    ./App_sgx | \
    tee -a /builds/rinkai/teep-device/sample/../build/sgx/sgx.log
[CEnclavePool /home/user/linux-sgx/psw/urts/enclave.cpp:627] enter CEnclavePool constructor
[build_secs /home/user/linux-sgx/psw/urts/loader.cpp:516] Enclave start addr. = 0x7f8173294000, Size = 0x8000000, 131072 KB
main start
Hello TEEP from TEE!
main end
Info: Enclave successfully returned.
```

Cleaning built binaries. Deleting the built binaries are required when starting to build TEEP-Device on other CPU architectures otherwise will generate errors.

```sh
$ make clean
```

## Generate Documentation

These TEEP-Device documentations in pdf and html format are generated by using Doxygen.

### Start the container

```sh
docker run -it --rm -v $(pwd):/home/user/teep-device aistcpsec/teep-dev:doxygen
```

### Generate pdf and html documentation

```sh
$ make docs
```

Location of created documentation.

```
docs/teep-device.pdf
docs/teep-device_readme_html.tar.gz
```
