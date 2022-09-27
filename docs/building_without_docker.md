# Build TEEP-Device without Docker and for Development boards

Clone the TEEP-Device's source code and build it for Keystone, OP-TEE and SGX.

To build TEEP-Device for any TEE, the preparation of the TA-Ref sdk has to be done in advance. The detailed steps of building TA-Ref can be found in the TA-Ref documentation. Export the path of TA-Ref in the following environment variable.

```sh
$ export TAREF_DIR=<ta-ref dir>
```

## Prerequisite

Have tested on Ubuntu 20.04.

Install packages for compiling.

```sh
sudo apt-get -y install build-essential git autoconf automake cmake git wget curl expect python3-pip libcap-dev debian-ports-archive-keyring locales
openssh-client openssh-server file libcurl4-gnutls-dev libjansson-dev rsync ntpdate usbutils pciutils net-tools iproute2 vim e2fsprogs
```

**Install suit-tool**

The TEEP Messages use SUIT Manifest format for acquiring TCs. The suit-tools is used in TEEP-Device for parsing and handling SUIT Manifests.

```sh
# Cloning suit-tool
git clone https://git.gitlab.arm.com/research/ietf-suit/suit-tool.git

# Checkout the version of suit-tools compatible with current TEEP-Device
cd suit-tool
git checkout ca66a97bac153864617e7868e44f4b409e3e6ed4 -b for-teep-device
python3 -m pip install --upgrade .
```

## Run tamproto (TAM Server) - Required by all Kestone/OP-TEE/SGX

Running a tamproto on a separate terminal is required as when the
TEEP_Device is executed which communicates with
the tamproto server to install the TC's.


```sh
# Clone the tamproto repo and checkout master branch
$ git clone https://192.168.100.100/rinkai/tamproto.git
$ cd tamproto
$ git checkout master
$ docker-compose build
$ docker-compose up
```

Once the TAM server is up, it will wait for incoming packets from TEEP-Device.

```console
naga@smartie:~/Aist_Dev/test/tamproto$ docker-compose up
Starting tamproto_tam_api_1 ... done
Attaching to tamproto_tam_api_1
tam_api_1  | { 'supported-cipher-suites': 1,
tam_api_1  |   challenge: 2,
tam_api_1  |   versions: 3,
tam_api_1  |   'ocsp-data': 4,
tam_api_1  |   'selected-cipher-suite': 5,
tam_api_1  |   'selected-version': 6,
tam_api_1  |   evidence: 7,
tam_api_1  |   'tc-list': 8,
tam_api_1  |   'ext-list': 9,
tam_api_1  |   'manifest-list': 10,
tam_api_1  |   msg: 11,
tam_api_1  |   'err-msg': 12,
tam_api_1  |   'evidence-format': 13,
tam_api_1  |   'requested-tc-list': 14,
tam_api_1  |   'unneeded-tc-list': 15,
tam_api_1  |   'component-id': 16,
tam_api_1  |   'tc-manifest-sequence-number': 17,
tam_api_1  |   'have-binary': 18,
tam_api_1  |   'suit-reports': 19,
tam_api_1  |   token: 20,
tam_api_1  |   'supported-freshness-mechanisms': 21 }
tam_api_1  | Loading KeyConfig
tam_api_1  | { TAM_priv: 'test-jw_tsm_identity_private_tam-mytam-private.jwk',
tam_api_1  |   TAM_pub: 'test-jw_tsm_identity_tam-mytam-public.jwk',
tam_api_1  |   TEE_priv: 'teep.jwk',
tam_api_1  |   TEE_pub: 'teep.jwk' }
tam_api_1  | Load key TAM_priv
tam_api_1  | Load key TAM_pub
tam_api_1  | Load key TEE_priv
tam_api_1  | Load key TEE_pub
tam_api_1  | Key binary loaded
tam_api_1  | 192.168.11.4
tam_api_1  | Express HTTP  server listening on port 8888
tam_api_1  | Express HTTPS server listening on port 8443
tam_api_1  | GET /api/ 200 5.239 ms - 24
```

Please keep opening this terminal running tamproto. Cloning and building the TEEP-Device  will be done on separate terminals.

## Keystone

Instruction to build `TEEP-Device` with Keystone. The Keystone and its supporting sources must be built and installed on the build environment beforehand. Refer to the Keystone section of the "Preparation before building TA-Ref without Docker" in the TA-Ref document.

### Clone and Build

Prepare the environment setup.

```sh
$ export TEE=keystone
$ export KEYSTONE_DIR=<path to keystone dir>
$ export PATH=$PATH:$KEYSTONE_DIR/riscv/bin
$ export KEYEDGE_DIR=<path to keyedge dir>
```

Clone and Build

```sh
# Clone the TEEP-Device
$ git clone https://192.168.100.100/rinkai/teep-device.git
$ cd teep-device
$ git checkout master

# Sync and update the submodules
$ git submodule sync --recursive
$ git submodule update --init --recursive

# Build the TEEP-Device
$ make
```

### Run hello-app & teep-broker-app on QEMU Environment

To check TEEP-Device on Keystone, we need to run TAM server on PC.

```sh
# After the successful build
$ make run-sample-session

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

### Run hello-app and teep-broker-app on RISC-V Unleashed

To check TEEP-Device on Unleashed, we need to run TAM server (refer above
to run tamproto) and networking with Unleashed dev board

#### Copy the hello-app and teep-broker-app binaries to Unleashed

**Manual Copy**

- Connect to Unleased over serial console then assign IP address `ifconfig eth0 192.168.0.6`
- Copy the binaries from build PC over SSH (user:root, password: sifive)

Here `192.168.0.6` is IP Address of Unleased board

```
$ scp platform/keystone/build/hello-ta/hello-ta root@192.168.0.6:/root/teep-device
$ scp platform/keystone/build/hello-app/hello-app root@192.168.0.6:/root/teep-device
$ scp platform/keystone/build/teep-agent-ta/teep-agent-ta root@192.168.0.6:/root/teep-device
$ scp platform/keystone/build/teep-broker-app/teep-broker-app root@192.168.0.6:/root/teep-device

$ scp $KEYSTONE_DIR/sdk/rts/eyrie/eyrie-rt root@192.168.0.6:/root/teep-device
$ scp platform/keystone/build/libteep/ree/mbedtls/library/lib* root@192.168.0.6:/usr/lib/
$ scp platform/keystone/build/libteep/ree/libwebsockets/lib/lib* root@192.168.0.6:/usr/lib/
```

**Write to SD card**

Please follow below steps to write the TEEP-Device binaries to SD-card

- Insert SD card to your PC for Unleashed
- Edit `platform/keystone/script/sktinst.sh`
- Check SD-card device name detected on yor PC and fix `prefix=?`
- `export prefix=/dev/mmcblk0`
- execute `script/sktinst.sh` as follows
- `cd platform/keystone; script/sktinst.sh`
- Move the sd to unleashed board and boot it

#### Run hello-app and teep-broker-app on Unleased

There are two methods to connect to Unleased.
- Serial Port using minicom (/dev/ttyUSB0)
- Over SSH: `ssh root@192.168.0.6`; password is `sifive`

Setup environment in Unleashed (create /root/env.sh file and add following lines)

```
$ export PATH=$PATH:/root/teep-device
$ export TAM_IP=tamproto_tam_api_1
$ export TAM_PORT=8888
$ insmod keystone-driver.ko
```

**Run hello-app**

```sh
$ source env.sh
[ 2380.618514] keystone_driver: loading out-of-tree module taints kernel.
[ 2380.625305] keystone_enclave: keystone enclave v0.2

$ cd teep-broker/

$ ./hello-app hello-ta eyrie-rt
Hello TEEP from TEE!
$
```

**Run teep-broker-app**

Use the TAM server IP address (i.e 192.168.11.4)

```sh
$ ./teep-broker-app --tamurl http://192.168.11.4:8888/api/tam_cbor
```

Upon execution, you see following log

```console
teep-bro[ 2932.269897] ------------[ cut here ]------------
[ 2932.274191] WARNING: CPU: 4 PID: 164
[ 2932.287053] Modules linked in: keystone_driver(O)
[ 2932.291716] CPU: 4 PID: 164 Comm: teep-broker-app Tainted: G
[ 2932.301867] Call Trace:
[ 2932.304314] [<0000000036e46dc0>] walk_stackframe+0x0/0xa2
[ 2932.309686] [<00000000893dfe1c>] show_stack+0x26/0x34
[ 2932.314725] [<00000000c57ed7ce>] dump_stack+0x5e/0x7c
[ 2932.319759] [<00000000a68ce031>] __warn+0xca/0xe0
[ 2932.324445] [<00000000bec1f8a6>] warn_slowpath_null+0x2c/0x3e
[ 2932.330176] [<00000000e8c56bf2>] __alloc_pages_nodemask+0x14c/0x8da
[ 2932.336426] [<00000000ec1f9596>] __get_free_pages+0xc/0x52
[ 2932.341920] [<000000003e8cccc8>] epm_init+0x158/0x1a0 [keystone_driver]
[ 2932.348502] [<0000000032e4188b>] create_enclave+0x56/0xb0 [keystone_driver]
[ 2932.355447] [<000000008a656a96>] keystone_create_enclave+0x16/0x40 [keystone_driver]
[ 2932.363174] [<000000003bbf2147>] keystone_ioctl+0x132/0x164 [keystone_driver]
[ 2932.370288] [<00000000755f7993>] do_vfs_ioctl+0x76/0x4f4
[ 2932.375582] [<00000000b88b9c1d>] SyS_ioctl+0x36/0x60
[ 2932.380533] [<00000000aae667a5>] check_syscall_nr+0x1e/0x22
[ 2932.386132] ---[ end trace 66814e3a8c80ec12 ]---
ker.c compiled at Feb 16 2021 11:17:21
uri = http://192.168.11.4:8888/api/tam_cbor, cose=0, talist=
[1970/01/01 00:48:56:0796] NOTICE: POST: http://192.168.11.4:8888/api/tam_cbor
[1970/01/01 00:48:56:0798] NOTICE: (hexdump: zero length)
[1970/01/01 00:48:56:0801] NOTICE: created client ssl context for default
[1970/01/01 00:48:56:0802] NOTICE: http://192.168.11.4:8888/api/tam_cbor
[1970/01/01 00:48:56:0861] NOTICE:
[1970/01/01 00:48:56:0862] NOTICE: 0000: 83 01 A4 01 81 01 03 81 00 14 1A 77 77 77 77 04    ...........wwww.
[1970/01/01 00:48:56:0862] NOTICE: 0010: 43 01 02 03 02                                     C....
[1970/01/01 00:48:56:0862] NOTICE:
[1970/01/01 00:48:56:0871] NOTICE: POST: http://192.168.11.4:8888/api/tam_cbor
[1970/01/01 00:48:56:0871] NOTICE:
[1970/01/01 00:48:56:0871] NOTICE: 0000: 82 02 A4 14 1A 77 77 77 77 08 80 0E 80 0F 80       .....wwww......
[1970/01/01 00:48:56:0872] NOTICE:
[1970/01/01 00:48:56:0873] NOTICE: created client ssl context for default
[1970/01/01 00:48:56:0874] NOTICE: http://192.168.11.4:8888/api/tam_cbor
[1970/01/01 00:48:56:0962] NOTICE:
[1970/01/01 00:48:56:0962] NOTICE: 0000: 82 03 A2 0A 81 59 01 37 A2 02 58 72 81 58 6F D2    .....Y.7..Xr.Xo.
[1970/01/01 00:48:56:0963] NOTICE: 0010: 84 43 A1 01 26 A0 58 24 82 02 58 20 75 80 7C 54    .C..&.X$..X u.|T
[1970/01/01 00:48:56:0963] NOTICE: 0020: 62 40 D2 14 E5 7B D5 C4 6A 7C E5 2D ED B0 3D 0E    b@...{..j|.-..=.
[1970/01/01 00:48:56:0964] NOTICE: 0030: CC 80 75 F3 F7 E0 65 B3 60 CE AD 85 58 40 54 81    ..u...e.`...X@T.
[1970/01/01 00:48:56:0964] NOTICE: 0040: 49 CD CA D8 17 72 CC EA 61 4A 19 99 05 AB 97 33    I....r..aJ.....3
[1970/01/01 00:48:56:0965] NOTICE: 0050: EA 48 D7 1F 13 AE 33 0D 47 FF F5 B8 6C 5C 9B 7A    .H....3.G...l\.z
[1970/01/01 00:48:56:0965] NOTICE: 0060: BB 12 BC 2D FE 9C 20 6A C8 7F E2 28 58 74 E0 74    ...-.. j...(Xt.t
[1970/01/01 00:48:56:0965] NOTICE: 0070: A3 BD C4 DA B9 20 C4 37 35 8F 67 46 90 76 03 58    ..... .75.gF.v.X
[1970/01/01 00:48:56:0966] NOTICE: 0080: BE A5 01 01 02 01 03 58 60 A2 02 44 81 81 41 00    .......X`..D..A.
[1970/01/01 00:48:56:0966] NOTICE: 0090: 04 58 56 86 14 A4 01 50 FA 6B 4A 53 D5 AD 5F DF    .XV....P.kJS.._.
[1970/01/01 00:48:56:0967] NOTICE: 00A0: BE 9D E6 63 E4 D4 1F FE 02 50 14 92 AF 14 25 69    ...c.....P....%i
[1970/01/01 00:48:56:0967] NOTICE: 00B0: 5E 48 BF 42 9B 2D 51 F2 AB 45 03 58 24 82 02 58    ^H.B.-Q..E.X$..X
[1970/01/01 00:48:56:0968] NOTICE: 00C0: 20 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE     .."3DUfw.......
[1970/01/01 00:48:56:0968] NOTICE: 00D0: FF 01 23 45 67 89 AB CD EF FE DC BA 98 76 54 32    ..#Eg........vT2
[1970/01/01 00:48:56:0969] NOTICE: 00E0: 10 0E 19 87 D0 01 F6 02 F6 09 58 4E 86 13 A1 15    ..........XN....
[1970/01/01 00:48:56:0969] NOTICE: 00F0: 78 44 68 74 74 70 3A 2F 2F 31 39 32 2E 31 36 38    xDhttp://192.168
[1970/01/01 00:48:56:0970] NOTICE: 0100: 2E 31 31 2E 33 3A 38 38 38 38 2F 54 41 73 2F 38    .0.5:8888/TAs/8
[1970/01/01 00:48:56:0970] NOTICE: 0110: 64 38 32 35 37 33 61 2D 39 32 36 64 2D 34 37 35    d82573a-926d-475
[1970/01/01 00:48:56:0971] NOTICE: 0120: 34 2D 39 33 35 33 2D 33 32 64 63 32 39 39 39 37    4-9353-32dc29997
[1970/01/01 00:48:56:0971] NOTICE: 0130: 66 37 34 2E 74 61 15 F6 03 F6 0A 43 82 03 F6 14    f74.ta.....C....
[1970/01/01 00:48:56:0972] NOTICE: 0140: 1A 77 77 77 78                                     .wwwx
[1970/01/01 00:48:56:0972] NOTICE:
[1970/01/01 00:48:56:0983] NOTICE:
GET: http://192.168.11.4:8888/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
[1970/01/01 00:48:56:0984] NOTICE: created client ssl context for default
[1970/01/01 00:48:56:0985] NOTICE:
http://192.168.11.4:8888/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
teep_message_unwrap_ta_image: msg len 234110
Decrypt
Decrypt OK: length 174887
Verify
Signature OK 0 130552
ta_store_install: ta_image_len = 130552 ta_name=8d82573a-926d-4754-9353-32dc29997f74
[1970/01/01 00:49:01:9453] NOTICE: POST: http://192.168.11.4:8888/api/tam_cbor
[1970/01/01 00:49:01:9454] NOTICE:
[1970/01/01 00:49:01:9454] NOTICE: 0000: 82 05 A1 14 1A 77 77 77 77                         .....wwww
[1970/01/01 00:49:01:9454] NOTICE:
[1970/01/01 00:49:01:9456] NOTICE: created client ssl context for default
[1970/01/01 00:49:01:9457] NOTICE: http://192.168.11.4:8888/api/tam_cbor
[1970/01/01 00:49:01:9505] NOTICE: (hexdump: zero length)

```

## OP-TEE

Instruction to build `TEEP-Device` with OP-TEE. The OP-TEE and its supporting sources must be built and installed on the build environment beforehand. Refer to the OP-TEE section of the "Preparation before building TA-Ref without Docker" in the TA-Ref document.

### Clone and Build

Prepare the environment setup

```
$ export TEE=optee
$ export OPTEE_DIR=<optee_dir>
$ export PATH=$PATH:$OPTEE_DIR/toolchains/aarch64/bin:$OPTEE_DIR/toolchains/aarch32/bin
```

Clone and Build

```sh
# Clone the TEEP-Device
$ git clone https://192.168.100.100/rinkai/teep-device.git
$ cd teep-device
$ git checkout master

# Sync and update the submodules
$ git submodule sync --recursive
$ git submodule update --init --recursive

# Build the TEEP-Device
$ make
```

### Run hello-app & teep-broker-app on QEMU Environment

To check TEEP-Device on OP-TEE, we need to run TAM server on PC.


```sh
# Install the TA on qemu
$ make optee_install_qemu

# After the successful build
# Test the TEEP-Device
$ make run-sample-session
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


### Run hello-app and teep-broker-app on Raspberry PI 3

To check TEEP-Device on Raspberry PI 3, we need to run the TAM server on PC and network with Raspberry PI 3 board.

#### Copy the hello-app and teep-broker-app binaries to Raspberry PI 3
<br/>

**Copy binaries over SSH to Raspberry PI 3**

- Connect to Raspberry PI 3 over serial console(typically at /dev/ttryUSB0) then acquire IP address. One of the commands to obtain an ip address.

```sh
ip address
```
- Copy the binaries from build PC over SSH (user:root) to Raspberry PI 3

```
TODO - Further update required
```

**Write to SD card**
<br />
Please follow below steps to write the TEEP-Device binaries to SD-card
- Insert SD card to your PC for Unleashed
- Copy the binaries to SD card
- Move the sd to Raspberry PI 3 board and boot it

```
TODO - Further update required
```

#### Run hello-app and teep-broker-app on Raspberry PI 3
<br/>
There are two methods to connect to Raspberry PI 3.
- Serial Port using minicom (/dev/ttyUSB0)
- Over SSH: `ssh root@192.168.0.7`

```
TODO - Further update required
```

**Run hello-app**

```sh
TODO - Further update required
```

**Run teep-broker-app**

Use the TAM server IP address (i.e 192.168.11.4)

```
./teep-broker-app --tamurl http://192.168.11.4:8888/api/tam_cbor
```

Execution logs

```sh
TODO - Further update required
```

## SGX

Instruction to build `TEEP-Device` with SGX. The SGX and its supporting sources must be built and installed on the build environment beforehand. Refer to the SGX section of the "Preparation before building TA-Ref without Docker" in the TA-Ref document.

### Clone and Build

As a preparation step, it is required to set up the Intel SGX SDK.
Please refer to the preparation steps for building without Docker for SGX in TA-Ref documentation.


```sh
$ source /opt/intel/sgxsdk/environment
$ export TAREF_DIR=<ta-ref dir>
```

Clone and Build

```sh
# Clone the TEEP-Device
$ git clone https://192.168.100.100/rinkai/teep-device.git
$ cd teep-device
$ git checkout master

# Sync and update the submodules
$ git submodule sync --recursive
$ git submodule update --init --recursive

# Build the TEEP-Device
$ export TEE=sgx
$ make
```

### Run hello-app & teep-broker-app on Intel SGX

To run TEEP-Device on SGX, confirm that the make is completed first and the tamproto is executed in another terminal.

Unlide Keystone and OP-TEE, it is directly executed on PC without using QEMU.

```sh
$ cd ~/teep-device
$ make run-sample-session
```

The output would contain the string `Hello TEEP from TEE!`
The messages between TEEP-Device and tamproto are printed out as the log on the terminals.

```console
main start
Hello TEEP from TEE!
main end
Info: Enclave successfully returned.
```


## Generate Documentation

This documentation (teep-device.pdf) is generated by using Doxygen. To install Doxygen the following procedure is necessary.

### Required Packages

Install the following packages on Ubuntu.

```sh
$ sudo apt install doxygen-latex graphviz texlive-full texlive-latex-base latex-cjk-all
```

Above packages required to generate PDF using doxygen.

### Build and Install Doxygen

It is using a specific commit since we had a compatibility issue.

```sh
$ git clone https://github.com/doxygen/doxygen.git
$ cd doxygen
$ git checkout 227952da7562a6f13da2a9d19c3cdc93812bc2de -b for-teep-device
$ mkdir build
$ cd build
$ cmake -G "Unix Makefiles" ..
$ make
$ sudo make install
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
