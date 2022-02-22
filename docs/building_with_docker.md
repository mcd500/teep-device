# Building TEEP-Device with docker


## Preparation for Docker

For building TEEP-Device with docker, it is required to install docker on Ubuntu.

For the first time users of docker, please have a look on https://docs.docker.com/engine/

The following installation steps is for Ubuntu 20.04

### Installing Docker

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
$ sudo apt install docker-ce
```

### Executing Docker without sudo

By default, the docker command can only be run the root user or by a user in the docker group, which is automatically created during Docker’s installation process. If you attempt to run the docker command without prefixing it with sudo or without being in the docker group, you’ll get an output like this:

```console
docker: Cannot connect to the Docker daemon. Is the docker daemon running on this host?.
```

To avoid typing sudo whenever we run the docker command, add your username to the docker group.

```sh
$ sudo groupadd docker

$ sudo gpasswd -a $USER docker

# Logout and then log-in again to apply the changes to the group
 ```

After you logout and login, you can probably run the docker command without `sudo`

```sh
$ docker run hello-world
```

### Create a docker network tamproto

A docker network named tamproto is required when we run teep device with ta-ref for all targets.
The local network is required to connect with tamproto service running locally.

```sh
$ docker network create tamproto_default 
```


## Pre-built Docker Image details

The following are the docker images that has pre-built and tested binaries of TEEP-Device with ta-ref.
Since this images are already prepared and built already, you can start using it directly without
building the TEEP-Device again.
Make sure you have account on docker-hub. If not please create one on `dockerhub.com`

| Target | docker image |
| ------ | ------ |
| Keystone | trasioteam/teep-dev:keystone |
| OP-TEE | trasioteam/teep-dev:optee |
| Intel SGX | trasioteam/teep-dev:sgx |
| Tamproto | trasioteam/teep-dev:tamproto |
| Doxygen | trasioteam/teep-dev:doxygen |


## Prepartion for building TEEP-Device on docker

### Docker images details for building

If we need to build the TEEP-Device,
docker images with all necessary packages for building TEEP-Device for all three targets are already available.
The details are mentioned below.


| Target | docker image |
| ------ | ------ |
| Keystone | trasioteam/taref-dev:keystone |
| OP-TEE | trasioteam/taref-dev:optee |
| Intel SGX | trasioteam/taref-dev:sgx |
| Doxygen | trasioteam/taref-dev:doxygen |


## Building TEEP-Device with Docker

### Building TEEP-Device for Keystone with docker

Following commands are to be executed on Ubuntu 20.04.

To run TEEP-Device, first we need to run tamproto inside the same
host. Lets clone the tamproto and start it.

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

**Start the docker**

```sh	
# Start the docker
$ docker run --network tamproto_default -it --rm -v $(pwd):/home/user/teep-device trasioteam/taref-dev:keystone 
```

After you start the docker command, you will be logged-in inside the docker container.
Following are the  commands to be executed inside the docker

```sh
# [Inside docker image]
    
# Change to teep-device
$ cd ~/teep-device/
    
# make the teep-device
$ make
    
# After the successful build
# Run the TEEP-Device
$ make test
	
```

Trimmed output printing 'Hello TEEP from TEE!'

```sh
Welcome to Buildroot
buildroot login: root
Password: sifive

#### insmod keystone-driver.ko || echo 'err''or'
[    5.350967] keystone_driver: loading out-of-tree module taints kernel.
[    5.358283] keystone_enclave: keystone enclave v1.0.0
#### cd /root/teep-device
#### ls -l
total 1367
-rwxr-xr-x    1 root     root         98088 Feb 15  2022 eyrie-rt
-rwxr-xr-x    1 root     root        437480 Feb 15  2022 hello-app
-rwxr-xr-x    1 root     root        152016 Feb 15  2022 hello-ta
-rwxr-xr-x    1 root     root        247416 Feb 15  2022 teep-agent-ta
-rwxr-xr-x    1 root     root        470568 Feb 15  2022 teep-broker-app
#### ./hello-app hello-ta eyrie-rt
[debug] UTM : 0xffffffff80000000-0xffffffff80100000 (1024 KB) (boot.c:127)
[debug] DRAM: 0x179800000-0x179c00000 (4096 KB) (boot.c:128)
[debug] FREE: 0x1799bd000-0x179c00000 (2316 KB), va 0xffffffff001bd000 (boot.c:133)
[debug] eyrie boot finished. drop to the user land ... (boot.c:172)

Hello TEEP from TEE!
	
#### ./hello-app 8d82573a-926d-4754-9353-32dc29997f74.ta eyrie-rt
[Keystone SDK] /home/user/keystone/sdk/src/host/ElfFile.cpp:26 : file does not exist - 8d82573a-926d-4754-9353-32dc29997f74.ta
[Keystone SDK] /home/user/keystone/sdk/src/host/Enclave.cpp:209 : Invalid enclave ELF
./hello-app: Unable to start enclave

#### ./teep-broker-app --tamurl http://tamproto_tam_api_1:8888/api/tam_cbor
teep-broker.c compiled at Feb 15 2022 10:07:55
uri = http://tamproto_tam_api_1:8888/api/tam_cbor, cose=0, talist=
[debug] UTM : 0xffffffff80000000-0xffffffff80100000 (1024 KB) (boot.c:127)
[debug] DRAM: 0x179800000-0x179c00000 (4096 KB) (boot.c:128)
[debug] FREE: 0x1799c6000-0x179c00000 (2280 KB), va 0xffffffff001c6000 (boot.c:133)
[debug] eyrie boot finished. drop to the user land ... (boot.c:172)
<output trimmed>
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
[1970/01/01 00:00:07:8189] NOTICE: GET: 
http://tamproto_tam_api_1:8888/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
[1970/01/01 00:00:07:8204] NOTICE: created client ssl context for default
[1970/01/01 00:00:07:8211] NOTICE: 
http://tamproto_tam_api_1:8888/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
component download 152016
store component
  device   = TEEP-Device
  storage  = SecureFS
  filename = 8d82573a-926d-4754-9353-32dc29997f74.ta
finish fetch
command: 3
execute suit-condition-image-match
end of command seq
[1970/01/01 00:00:08:4271] NOTICE: POST: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:08:4276] NOTICE: 
[1970/01/01 00:00:08:4280] NOTICE: 0000: 82 05 A1 14 48 77 77 77 77 77 77 77 77             ....Hwwwwwwww   
[1970/01/01 00:00:08:4287] NOTICE: 
[1970/01/01 00:00:08:4297] NOTICE: created client ssl context for default
[1970/01/01 00:00:08:4303] NOTICE: http://tamproto_tam_api_1:8888/api/tam_cbor
[1970/01/01 00:00:08:4563] NOTICE: (hexdump: zero length)
#### ls -l
total 1517
-rw-------    1 root     root        152016 Jan  1 00:00 8d82573a-926d-4754-9353-32dc29997f74.ta
-rwxr-xr-x    1 root     root         98088 Feb 15  2022 eyrie-rt
-rwxr-xr-x    1 root     root        437480 Feb 15  2022 hello-app
-rwxr-xr-x    1 root     root        152016 Feb 15  2022 hello-ta
-rwxr-xr-x    1 root     root        247416 Feb 15  2022 teep-agent-ta
-rwxr-xr-x    1 root     root        470568 Feb 15  2022 teep-broker-app
#### ./hello-app 8d82573a-926d-4754-9353-32dc29997f74.ta eyrie-rt
[debug] UTM : 0xffffffff80000000-0xffffffff80100000 (1024 KB) (boot.c:127)
[debug] DRAM: 0x179800000-0x179c00000 (4096 KB) (boot.c:128)
[debug] FREE: 0x1799bd000-0x179c00000 (2316 KB), va 0xffffffff001bd000 (boot.c:133)
[debug] eyrie boot finished. drop to the user land ... (boot.c:172)
	
Hello TEEP from TEE!
	
97f74.ta.secstor.plain-4754-9353-32dc29997f74.ta 8d82573a-926d-4754-9353-32dc2999
cmp: 8d82573a-926d-4754-9353-32dc29997f74.ta.secstor.plain: No such file or directory
####  done
```

### Building TEEP-Device for OPTEE with docker

To run TEEP-Device, first we need to run tamproto inside the same
host. Lets clone the tamproto and start it.

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

Copy the IP address of the tamproto which will be passed in the 
next section.

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

**Start the docker**

```sh	
# Start the docker
$ docker run --network tamproto_default -it --rm -v $(pwd):/home/user/teep-device trasioteam/taref-dev:optee
```  

After you start the docker command, you will be logged-in inside the docker container.
Following are the commands to be executed inside the docker

```sh
# [Inside docker image]
	
# Change to teep-device
$ cd ~/teep-device/
    
# Build the teep device
$ make
    
# Install the TA on qemu
$ make optee_install_qemu
    
# After the successful build
# Run the TEEP-Device
$ make test
    
```

Trimmed output of the test 

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

### Building TEEP-Device for PC with docker


To run TEEP-Device, first we need to run tamproto inside the same
host. Lets clone the tamproto and start it.

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

Copy the IP address of the tamproto which will be passed in the 
next section.

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

**Start the docker**

```sh	
# Start the docker
$ docker run --network tamproto_default -it --rm -v $(pwd):/home/user/teep-device trasioteam/taref-dev:sgx
```  

After you start the docker command, you will be logged-in inside the docker container.
Following are the commands to be executed inside the docker

```sh
# [Inside docker image]
	
# Change to teep-device
$ cd ~/teep-device/
    
# set the TEE environment to pc
$ export TEE=pc
	
# Build the teep device
$ make
	    
# After the successful build
# Run the TEEP-Device
$ make test
    
```

Trimmed output of the run.
The output can be found in /home/user/teep-device/platform/pc/build/8d82573a-926d-4754-9353-32dc29997f74.ta


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