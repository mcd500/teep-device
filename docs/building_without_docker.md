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
$ git clone https://github.com/ko-isobe/tamproto.git
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
$ git clone https://github.com/mcd500/teep-device.git
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
$ git clone https://github.com/mcd500/teep-device.git
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
make -C sample run-session TAM_URL=http://172.17.0.32:8888
make[1]: Entering directory '/builds/rinkai/teep-device/sample'
make -C /builds/rinkai/teep-device/sample/../hello-tc/build-optee
 SOURCE=/builds/rinkai/teep-device/sample/../hello-tc upload-download-manifest
make[2]: Entering directory '/builds/rinkai/teep-device/hello-tc/build-optee'
curl http://172.17.0.32:8888/panel/upload \
	-F "file=@/builds/rinkai/teep-device/hello-tc/build-optee/../../build/optee/hello-tc/signed-download-tc.suit;f
	ilename=integrated-payload-manifest.cbor"
...
...
...
cd /home/user/optee/out/bin && \
	QEMU=/home/user/optee/qemu/aarch64-softmmu/qemu-system-aarch64 \
	QEMU_SMP=2 \
	TAM_URL=http://172.17.0.32:8888 \
	ROOTFS=/builds/rinkai/teep-device/sample/../build/optee/rootfs.cpio.gz \
	expect /builds/rinkai/teep-device/sample/session/test-optee.expect
Starting QEMU...
..
..
FI stub: Booting Linux Kernel...
EFI stub: EFI_RNG_PROTOCOL unavailable, no randomness supplied
EFI stub: Using DTB from configuration table
EFI stub: Exiting boot services and installing virtual address map...
Starting syslogd: OK
Starting klogd: OK
Initializing random number generator... [    3.248115] random: dd: uninitialized urandom read (512 bytes read)
done.
Set permissions on /dev/tee*: OK
Set permissions on /dev/ion: OK
Create/set permissions on /data/tee: OK
Starting tee-supplicant: OK
Starting network: OK
Starting network (udhcpc): OK
Welcome to Buildroot, type root or test to login
buildroot login: root
#  done, guest is booted.
export LD_LIBRARY_PATH=/lib:/lib/arm-linux-gnueabihf:/lib/optee_armtz:/usr/lib
# cd teep-broker
# ls -l
total 4228
-rwxr-xr-x    1 root     root           567 Nov 24 07:27 cp_ta_to_tamproto.sh
-rwxr-xr-x    1 root     root           146 Nov 24 07:27 env.sh
-rwxr-xr-x    1 root     root           290 Nov 24 07:27 get-ip.sh
-rwxr-xr-x    1 root     root         14112 Nov 24 07:27 hello-app
-rwxr-xr-x    1 root     root            65 Nov 24 07:27 itc.sh
-rwxr-xr-x    1 root     root           116 Nov 24 07:27 rtc.sh
-rwxr-xr-x    1 root     root           134 Nov 24 07:27 showtamurl.sh
-rwxr-xr-x    1 root     root       4285528 Nov 24 07:27 teep-broker-app
# ./hello-app
hello-app: TEEC_Opensession failed with code 0xffff0008 origin 0x3
# ./teep-broker-app --tamurl http://172.17.0.32:8888/api/tam_cbor
teep-broker.c compiled at Nov 24 2022 07:27:53
uri = http://172.17.0.32:8888/api/tam_cbor, cose=0, talist=
[2022/11/24 07:28:26:9408] N: POST: http://172.17.0.32:8888/api/tam_cbor
[2022/11/24 07:28:26:9425] N: (hexdump: zero length)
[2022/11/24 07:28:26:9506] N: http://172.17.0.32:8888/api/tam_cbor
[2022/11/24 07:28:26:0224] N: 
[2022/11/24 07:28:26:0234] N: 0000: 83 01 A5 01 81 01 03 81 00 04 43 01 02 05 14 48    ..........C....H
[2022/11/24 07:28:26:0243] N: 0010: 77 77 77 77 77 77 77 77 15 81 00 02                wwwwwwww....    
[2022/11/24 07:28:26:0250] N: 
[2022/11/24 07:28:26:0374] N: POST: http://172.17.0.32:8888/api/tam_cbor
[2022/11/24 07:28:26:0379] N: 
[2022/11/24 07:28:26:0382] N: 0000: 82 02 A4 14 48 77 77 77 77 77 77 77 77 08 80 0E    ....Hwwwwwwww...
[2022/11/24 07:28:26:0389] N: 0010: 80 0F 80                                           ...             
[2022/11/24 07:28:26:0395] N: 
[2022/11/24 07:28:26:0417] N: http://172.17.0.32:8888/api/tam_cbor
[2022/11/24 07:28:27:0742] N: 
[2022/11/24 07:28:27:0746] N: 0000: 82 03 A2 0A 81 59 01 5F D8 6B A2 02 58 73 82 58    .....Y._.k..Xs.X
[2022/11/24 07:28:27:0751] N: 0010: 24 82 2F 58 20 D7 FC F7 75 1E CB 77 96 39 A4 0F    $./X ...u..w.9..
[2022/11/24 07:28:27:0758] N: 0020: 58 66 56 EF D3 08 7D 31 ED C3 C4 5B EE DD 9E 95    XfV...}1...[....
[2022/11/24 07:28:27:0767] N: 0030: 38 CE 0D 3E 8A 58 4A D2 84 43 A1 01 26 A0 F6 58    8..>.XJ..C..&..X
[2022/11/24 07:28:27:0774] N: 0040: 40 6F D3 76 AE AE CF F3 BC E7 7E 60 E1 22 0A 20    @o.v......~`.. 
[2022/11/24 07:28:27:0780] N: 0050: 1C 3C 10 3F 85 BE 71 A7 10 E5 6D C1 C5 0A A6 C6    .<.?..q...m.....
[2022/11/24 07:28:27:0786] N: 0060: 47 D4 D4 EE DD 20 2D 08 EA 4F 74 6F 48 FF 3D D3    G.... -..OtoH.=.
[2022/11/24 07:28:27:0793] N: 0070: 32 B4 60 18 95 15 6A 5D 25 12 EF E8 8B 35 CE CD    2.`...j]%....5..
[2022/11/24 07:28:27:0799] N: 0080: 1C 03 58 E3 A5 01 01 02 01 03 58 86 A2 02 81 84    ..X.......X.....
[2022/11/24 07:28:27:0808] N: 0090: 4B 54 45 45 50 2D 44 65 76 69 63 65 48 53 65 63    KTEEP-DeviceHSec
[2022/11/24 07:28:27:0815] N: 00A0: 75 72 65 46 53 50 8D 82 57 3A 92 6D 47 54 93 53    ureFSP..W:.mGT.S
[2022/11/24 07:28:27:0821] N: 00B0: 32 DC 29 99 7F 74 42 74 61 04 58 56 86 14 A4 01    2.)..tBta.XV....
[2022/11/24 07:28:27:0827] N: 00C0: 50 FA 6B 4A 53 D5 AD 5F DF BE 9D E6 63 E4 D4 1F    P.kJS.._....c...
[2022/11/24 07:28:27:0833] N: 00D0: FE 02 50 14 92 AF 14 25 69 5E 48 BF 42 9B 2D 51    ..P....%i^H.B.-Q
[2022/11/24 07:28:27:0840] N: 00E0: F2 AB 45 03 58 24 82 2F 58 20 00 11 22 33 44 55    ..E.X$./X ..3DU
[2022/11/24 07:28:27:0847] N: 00F0: 66 77 88 99 AA BB CC DD EE FF 01 23 45 67 89 AB    fw.........#Eg..
[2022/11/24 07:28:27:0853] N: 0100: CD EF FE DC BA 98 76 54 32 10 0E 19 87 D0 01 0F    ......vT2.......
[2022/11/24 07:28:27:0859] N: 0110: 02 0F 09 58 4D 86 13 A1 15 78 43 68 74 74 70 3A    ...XM....xChttp:
[2022/11/24 07:28:27:0865] N: 0120: 2F 2F 31 37 32 2E 31 37 2E 30 2E 33 32 3A 38 38    //172.17.0.32:88
[2022/11/24 07:28:27:0871] N: 0130: 38 38 2F 54 41 73 2F 38 64 38 32 35 37 33 61 2D    88/TAs/8d82573a-
[2022/11/24 07:28:27:0877] N: 0140: 39 32 36 64 2D 34 37 35 34 2D 39 33 35 33 2D 33    926d-4754-9353-3
[2022/11/24 07:28:27:0884] N: 0150: 32 64 63 32 39 39 39 37 66 37 34 2E 74 61 15 02    2dc29997f74.ta..
[2022/11/24 07:28:27:0890] N: 0160: 03 0F 0A 43 82 03 0F 14 48 AB A1 A2 A3 A4 A5 A6    ...C....H.......
[2022/11/24 07:28:27:0896] N: 0170: A7                                                 .               
[2022/11/24 07:28:27:0901] N: 
[2022/11/24 07:28:27:6609] N: GET: http://172.17.0.32:8888/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
[2022/11/24 07:28:27:6632] N: http://172.17.0.32:8888/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
[2022/11/24 07:28:27:7968] N: POST: http://172.17.0.32:8888/api/tam_cbor
[2022/11/24 07:28:27:7973] N: 
[2022/11/24 07:28:27:7977] N: 0000: 82 05 A1 14 48 77 77 77 77 77 77 77 77             ....Hwwwwwwww   
[2022/11/24 07:28:27:7982] N: 
[2022/11/24 07:28:27:7993] N: http://172.17.0.32:8888/api/tam_cbor
[2022/11/24 07:28:27:8228] N: (hexdump: zero length)
# ls -l
total 4228
-rwxr-xr-x    1 root     root           567 Nov 24 07:27 cp_ta_to_tamproto.sh
-rwxr-xr-x    1 root     root           146 Nov 24 07:27 env.sh
-rwxr-xr-x    1 root     root           290 Nov 24 07:27 get-ip.sh
-rwxr-xr-x    1 root     root         14112 Nov 24 07:27 hello-app
-rwxr-xr-x    1 root     root            65 Nov 24 07:27 itc.sh
-rwxr-xr-x    1 root     root           116 Nov 24 07:27 rtc.sh
-rwxr-xr-x    1 root     root           134 Nov 24 07:27 showtamurl.sh
-rwxr-xr-x    1 root     root       4285528 Nov 24 07:27 teep-broker-app
# ./hello-app
#  done
..

M/TA: TTRC:verifying signature of suit manifest
M/TA: TTRC:verify OK
M/TA: TTRC:command: 20
M/TA: TTRC:execute suit-set-parameters
M/TA: TTRC:command: 1
M/TA: TTRC:execute suit-condition-vendor-identifier
M/TA: TTRC:command: 2
M/TA: TTRC:execute suit-condition-class-identifier
M/TA: TTRC:command: 19
M/TA: TTRC:execute suit-set-parameters
M/TA: TTRC:command: 21
M/TA: TTRC:execute suit-directive-fetch
M/TA: TTRC:fetch_and_store component
M/TA: TTRC:component download 55976
M/TA: TTRC:ta-store.c: store_component() store component
M/TA: TTRC:  device   = TEEP-Device
M/TA: TTRC:  storage  = SecureFS
M/TA: TTRC:  filename = 8d82573a-926d-4754-9353-32dc29997f74.ta
M/TA: TTRC:  image_len = 55976
D/TC:? 0 tee_ta_init_pseudo_ta_session:283 Lookup pseudo TA 6e256cba-fc4d-4941-ad09-2ca1860342dd
D/TC:? 0 tee_ta_init_pseudo_ta_session:296 Open secstor_ta_mgmt
D/TC:? 0 tee_ta_init_pseudo_ta_session:310 secstor_ta_mgmt : 6e256cba-fc4d-4941-ad09-2ca1860342dd
D/TC:? 0 install_ta:99 Installing 8d82573a-926d-4754-9353-32dc29997f74
D/TC:? 0 tee_ta_close_session:499 csess 0xc37ecfb0 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session
M/TA: TTRC:finish fetch
M/TA: TTRC:command: 3
M/TA: TTRC:execute suit-condition-image-match
M/TA: TTRC:end of command seq
D/TC:? 0 tee_ta_close_session:499 csess 0xc37eeb40 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session
D/TC:? 0 destroy_context:298 Destroy TA ctx (0xc37eeae0)
D/TC:? 0 tee_ta_init_pseudo_ta_session:283 Lookup pseudo TA 8d82573a-926d-4754-9353-32dc29997f74
D/TC:? 0 load_ldelf:704 ldelf load address 0x40006000
D/LD:  ldelf:134 Loading TA 8d82573a-926d-4754-9353-32dc29997f74
D/TC:? 0 tee_ta_init_session_with_context:573 Re-open TA 3a2f8978-5dc0-11e8-9c2d-fa7ae01bbebc
D/TC:? 0 system_open_ta_binary:257 Lookup user TA ELF 8d82573a-926d-4754-9353-32dc29997f74 (Secure Storage TA)
D/TC:? 0 system_open_ta_binary:260 res=0x0
D/LD:  ldelf:169 ELF (8d82573a-926d-4754-9353-32dc29997f74) at 0x4004b000
D/TC:? 0 tee_ta_close_session:499 csess 0xc37ec610 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session
Hello TEEP from TEE!
D/TC:? 0 tee_ta_close_session:499 csess 0xc37ece10 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session
D/TC:? 0 destroy_context:298 Destroy TA ctx (0xc37ecdb0)
! fgrep 'ERR:' /home/user/optee/out/bin/serial1.log
fgrep 'Hello TEEP from TEE!' /home/user/optee/out/bin/serial1.log
Hello TEEP from TEE!
make[1]: Leaving directory '/home/user/teep-device/sample'
```


### Run hello-app and teep-broker-app on Raspberry PI 3

The RPI3 board needs the following to boot.
1) Kernal Image - kernel8.img
2) DTB file for RPI3 - bcm2710-rpi-3-b-plus.dtb
3) Rootfs file system - arm64-20.04-rootfs-teep-device.tar.xz

You can copy the Kernal Image and DTB file from the **aistcpsec/tee-dev:optee-3.10.0_rpi3** image.
It will located inside /home/user/optee/out-br/target/boot folder.

The rootfs file system can be downloaded from the docker image **aistcpsec/teep-dev:rootfs_teep-device_rpi3_netboot_nfs**
It will be available in the / folder. Tar file name is arm64-20.04-rootfs-teep-device.tar.xz.

Please copy the above files from docker to your local using docker cp command.

**Partition SD Card**

Partition the SD card into two partitions 
Partition 1 - Boot Partition - Place the Kernal Image file and DTB File 
Partition 2 - Rootfs file system - Copy the extracted arm64-20.04-rootfs-teep-device.tar.xz contents.


**Write to SD card**
<br />
Please follow below steps to write the TEEP-Device binaries to SD-card
- Insert SD card to your PC for Unleashed
- Copy the binaries to SD card
- Move the SD card to Raspberry PI 3 board and boot it

#### Run hello-app and teep-broker-app on Raspberry PI 3

There are two methods to connect to Raspberry PI 3.
- Serial Port using minicom (/dev/ttyUSB0)
- Over SSH: `ssh root@<rpi3_ip_address>` 

In the below steps, let us consider the IP address of RPI3 connected
system is 192.168.100.118 and the IP address of RPI3 is 192.168.100.114

Also, Tamproto Server is required to test the TEEP-Device.
We can start the tamproto in the PC or we can start inside RPI3 itself.
In our case, we will start the tamproto server inside the RPI3.

**Access the RPI3 Terminal - Using Minicom**

When RPI3 is booting, we can access the RPI3 using the minicom.
This is access from the PC to which the RPI3 is connected.
Also /dev/ttyUSB0 is not fixed, it may be /dev/ttyUSB1 or 2 etc.

```sh
$ minicom -D /dev/ttyUSB0

root@arm64-ubuntu:~# NOTICE:  Booting Trusted Firmware
NOTICE:  BL1: v2.2(debug):v2.2
NOTICE:  BL1: Built : 03:31:10, Nov 15 2022
INFO:    BL1: RAM 0x100ee000 - 0x100f7000
INFO:    BL1: cortex_a53: CPU workaround for 843419 was applied
INFO:    BL1: cortex_a53: CPU workaround for 855873 was applied
NOTICE:  rpi3: Detected: Raspberry Pi 3 Model B+ (1GB, Sony, UK) [0x00a020d3]
INFO:    BL1: Loading BL2
INFO:    Loading image id=1 at address 0x100b4000
INFO:    Image id=1 loaded: 0x100b4000 - 0x100bc410
NOTICE:  BL1: Booting BL2
INFO:    Entry point address = 0x100b4000
INFO:    SPSR = 0x3c5
NOTICE:  BL2: v2.2(debug):v2.2
NOTICE:  BL2: Built : 03:31:10, Nov 15 2022
INFO:    BL2: Doing platform setup
INFO:    BL2: Loading image id 3
INFO:    Loading image id=3 at address 0x100e0000
INFO:    Image id=3 loaded: 0x100e0000 - 0x100ea078
INFO:    BL2: Loading image id 4
INFO:    Loading image id=4 at address 0x10100000
INFO:    Image id=4 loaded: 0x10100000 - 0x1010001c
INFO:    OPTEE ep=0x10100000
INFO:    OPTEE header info:
INFO:          magic=0x4554504f
INFO:          version=0x2
INFO:          arch=0x1

arm64-ubuntu login: root
Password: 
Welcome to Ubuntu 20.04.5 LTS (GNU/Linux 4.14.56-v8 aarch64)

```


**Access the RPI3 Terminal - Using SSH**
After RPI3 is booted, we can access the RPI3 terminal using SSH

```sh
$ ssh root@192.168.100.114
root@192.168.100.114's password: 
Welcome to Ubuntu 20.04.5 LTS (GNU/Linux 4.14.56-v8 aarch64)

 * Documentation:  https://help.ubuntu.com
 * Management:     https://landscape.canonical.com
 * Support:        https://ubuntu.com/advantage
Last login: Wed Aug 31 15:28:32 2022
root@arm64-ubuntu:~# 
```


**Starting the Tamproto Server**

First, lets install npm and node server in RPI3.
This is done by executing the ./install_node.sh file.
```sh
$ # cd /home/user/./install_node.sh 
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100 15916  100 15916    0     0  42329      0 --:--:-- --:--:-- --:--:-- 42217
=> Downloading nvm as script to '/root/.nvm'

=> Appending nvm source string to /root/.bashrc
=> Appending bash_completion source string to /root/.bashrc
=> Close and reopen your terminal to start using nvm or run the following to use it now:

export NVM_DIR="$HOME/.nvm"
[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"  # This loads nvm
[ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion"  # This loads nvm bash_completion
Installing latest LTS version.
Downloading and installing node v18.12.1...
Downloading https://nodejs.org/dist/v18.12.1/node-v18.12.1-linux-arm64.tar.xz...
########################################### 100.0%
Computing checksum with sha256sum
Checksums matched!
Now using node v18.12.1 (npm v8.19.2)
Creating default alias: default -> lts/* (-> v18.12.1)
Now using node v18.12.1 (npm v8.19.2)
root@arm64-ubuntu:/home/user# 

```


Then install the node modules and start the tamproto server.
```sh
root@arm64-ubuntu:/home/user# cd tamproto/
root@arm64-ubuntu:/home/user/tamproto# ls
Dockerfile  README.md  app.js       docker-compose.yml  keymanager.js      package.json        query_response.cbor  suit.cbor        teep-p.js
LICENSE     TAs        config.json  key                 package-lock.json  query_request.cbor  routes               ta_install.cbor  views
root@arm64-ubuntu:/home/user/tamproto# npm install
added 154 packages, and audited 155 packages in 23s

13 packages are looking for funding
  run `npm fund` for details

found 0 vulnerabilities
npm notice 
npm notice New major version of npm available! 8.19.2 -> 9.1.2
npm notice Changelog: https://github.com/npm/cli/releases/tag/v9.1.2
npm notice Run npm install -g npm@9.1.2 to update!
npm notice 
root@arm64-ubuntu:/home/user/tamproto# 
root@arm64-ubuntu:/home/user/tamproto# node app.js
{
  'supported-cipher-suites': 1,
  challenge: 2,
  versions: 3,
  'ocsp-data': 4,
  'selected-cipher-suite': 5,
  'selected-version': 6,
  evidence: 7,
  'tc-list': 8,
  'ext-list': 9,
  'manifest-list': 10,
  msg: 11,
  'err-msg': 12,
  'evidence-format': 13,
  'requested-tc-list': 14,
  'unneeded-tc-list': 15,
  'component-id': 16,
  'tc-manifest-sequence-number': 17,
  'have-binary': 18,
  'suit-reports': 19,
  token: 20,
  'supported-freshness-mechanisms': 21
}
Loading KeyConfig
{
  TAM_priv: 'teep_ecP256.jwk',
  TAM_pub: 'teep_ecP256.jwk',
  TEE_priv: 'teep_ecP256.jwk',
  TEE_pub: 'teep_agent_prime256v1_pub.pem'
}
Load key TAM_priv
Load key TAM_pub
Load key TEE_priv
Load key TEE_pub
Key binary loaded
(node:1098) Warning: Accessing non-existent property 'request' of module exports inside circular dependency
(Use `node --trace-warnings ...` to show where the warning was created)
192.168.100.114
Express HTTP  server listening on port 8888
Express HTTPS server listening on port 8443

```


**Copy the TA's into Tamproto server**

Open another terminal using ssh

```sh
$ cd /home/user/teep-broker
$ ./cp_ta_to_tamproto.sh
```

**Run the teep-broker-app**

(Inside teep-broker folder)
```sh
$ ./showtamurl.sh
   --tamurl http://192.168.100.114:8888/api/tam_cbor
$ ./teep-broker-app --tamurl http://192.168.100.114:8888/api/tam_cbor
teep-broker.c compiled at Nov 24 2022 08:14:22
uri = http://192.168.100.114:8888/api/tam_cbor, cose=0, talist=
[2022/11/24 08:54:57:7358] N: POST: http://192.168.100.114:8888/api/tam_cbor
[2022/11/24 08:54:57:7358] N: (hexdump: zero length)
[2022/11/24 08:54:57:7369] N: http://192.168.100.114:8888/api/tam_cbor
[2022/11/24 08:54:57:8365] N: 
[2022/11/24 08:54:57:8366] N: 0000: 83 01 A5 01 81 01 03 81 00 04 43 01 02 05 14 48    ..........C....H
[2022/11/24 08:54:57:8366] N: 0010: 77 77 77 77 77 77 77 77 15 81 00 02                wwwwwwww....    
[2022/11/24 08:54:57:8366] N: 
[2022/11/24 08:54:57:8375] N: POST: http://192.168.100.114:8888/api/tam_cbor
[2022/11/24 08:54:57:8375] N: 
[2022/11/24 08:54:57:8376] N: 0000: 82 02 A4 14 48 77 77 77 77 77 77 77 77 08 80 0E    ....Hwwwwwwww...
[2022/11/24 08:54:57:8376] N: 0010: 80 0F 80                                           ...             
[2022/11/24 08:54:57:8376] N: 
[2022/11/24 08:54:57:8380] N: http://192.168.100.114:8888/api/tam_cbor
[2022/11/24 08:54:57:9134] N: 
[2022/11/24 08:54:57:9135] N: 0000: 82 03 A2 0A 81 59 01 5D D8 6B A2 02 58 73 82 58    .....Y.].k..Xs.X
[2022/11/24 08:54:57:9136] N: 0010: 24 82 2F 58 20 E5 2A E9 E8 AC 01 49 41 2E 3C EB    $./X .*....IA.<.
[2022/11/24 08:54:57:9136] N: 0020: E8 8D 6C B7 27 A9 DE D6 42 24 1A FD 39 D5 ED 0E    ..l.'...B$..9...
[2022/11/24 08:54:57:9136] N: 0030: 51 E8 9A 95 BF 58 4A D2 84 43 A1 01 26 A0 F6 58    Q....XJ..C..&..X
[2022/11/24 08:54:57:9136] N: 0040: 40 97 C2 E8 79 81 C5 23 6B 63 C5 AF 51 41 6C 43    @...y..#kc..QAlC
[2022/11/24 08:54:57:9137] N: 0050: F6 9D E8 91 D7 EB AB 73 7A 30 52 A7 74 02 73 01    .......sz0R.t.s.
[2022/11/24 08:54:57:9137] N: 0060: 0F B6 70 E2 0B 70 D8 3B CF C3 31 8A 26 39 D0 4D    ..p..p.;..1.&9.M
[2022/11/24 08:54:57:9137] N: 0070: 1F CF 4B 77 83 F3 24 7F 43 2E 9D 77 00 37 6B CC    ..Kw..$.C..w.7k.
[2022/11/24 08:54:57:9137] N: 0080: E0 03 58 E1 A5 01 01 02 01 03 58 86 A2 02 81 84    ..X.......X.....
[2022/11/24 08:54:57:9138] N: 0090: 4B 54 45 45 50 2D 44 65 76 69 63 65 48 53 65 63    KTEEP-DeviceHSec
[2022/11/24 08:54:57:9138] N: 00A0: 75 72 65 46 53 50 8D 82 57 3A 92 6D 47 54 93 53    ureFSP..W:.mGT.S
[2022/11/24 08:54:57:9138] N: 00B0: 32 DC 29 99 7F 74 42 74 61 04 58 56 86 14 A4 01    2.)..tBta.XV....
[2022/11/24 08:54:57:9138] N: 00C0: 50 FA 6B 4A 53 D5 AD 5F DF BE 9D E6 63 E4 D4 1F    P.kJS.._....c...
[2022/11/24 08:54:57:9139] N: 00D0: FE 02 50 14 92 AF 14 25 69 5E 48 BF 42 9B 2D 51    ..P....%i^H.B.-Q
[2022/11/24 08:54:57:9139] N: 00E0: F2 AB 45 03 58 24 82 2F 58 20 00 11 22 33 44 55    ..E.X$./X .."3DU
[2022/11/24 08:54:57:9139] N: 00F0: 66 77 88 99 AA BB CC DD EE FF 01 23 45 67 89 AB    fw.........#Eg..
[2022/11/24 08:54:57:9140] N: 0100: CD EF FE DC BA 98 76 54 32 10 0E 19 87 D0 01 0F    ......vT2.......
[2022/11/24 08:54:57:9140] N: 0110: 02 0F 09 58 4B 86 13 A1 15 78 41 68 74 74 70 3A    ...XK....xAhttp:
[2022/11/24 08:54:57:9141] N: 0120: 2F 2F 31 32 37 2E 30 2E 30 2E 31 3A 38 38 38 38    //127.0.0.1:8888
[2022/11/24 08:54:57:9141] N: 0130: 2F 54 41 73 2F 38 64 38 32 35 37 33 61 2D 39 32    /TAs/8d82573a-92
[2022/11/24 08:54:57:9141] N: 0140: 36 64 2D 34 37 35 34 2D 39 33 35 33 2D 33 32 64    6d-4754-9353-32d
[2022/11/24 08:54:57:9142] N: 0150: 63 32 39 39 39 37 66 37 34 2E 74 61 15 02 03 0F    c29997f74.ta....
[2022/11/24 08:54:57:9142] N: 0160: 0A 43 82 03 0F 14 48 AB A1 A2 A3 A4 A5 A6 A7       .C....H........ 
[2022/11/24 08:54:57:9142] N: 
[2022/11/24 08:54:58:1626] N: GET: http://127.0.0.1:8888/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
[2022/11/24 08:54:58:1630] N: http://127.0.0.1:8888/TAs/8d82573a-926d-4754-9353-32dc29997f74.ta
[2022/11/24 08:54:58:3440] N: POST: http://192.168.100.114:8888/api/tam_cbor
[2022/11/24 08:54:58:3441] N: 
[2022/11/24 08:54:58:3441] N: 0000: 82 05 A1 14 48 77 77 77 77 77 77 77 77             ....Hwwwwwwww   
[2022/11/24 08:54:58:3441] N: 
[2022/11/24 08:54:58:3444] N: http://192.168.100.114:8888/api/tam_cbor
[2022/11/24 08:54:58:3646] N: (hexdump: zero length)
root@arm64-ubuntu:/home/user/teep-broker# 
```


**Execute the ./hello-app**

```sh
$ ./hello-app

# Check for the "Hello from TEE!" output on Minicom window
D/LD:  ldelf:134 Loading TA 68373894-5bb3-403c-9eec-3114a1f5d3fc
D/TC:? 0 tee_ta_init_session_with_context:573 Re-open TA 3a2f8978-5dc0-11e8-9c2d-fa7ae01bbebc
D/TC:? 0 system_open_ta_binary:257 Lookup user TA ELF 68373894-5bb3-403c-9eec-3114a1f5d3fc (Secure Storage TA)
D/TC:? 0 system_open_ta_binary:260 res=0xffff0008
D/TC:? 0 system_open_ta_binary:257 Lookup user TA ELF 68373894-5bb3-403c-9eec-3114a1f5d3fc (REE)
D/TC:? 0 system_open_ta_binary:260 res=0x0
D/LD:  ldelf:169 ELF (68373894-5bb3-403c-9eec-3114a1f5d3fc) at 0x4008c000
D/TC:? 0 tee_ta_close_session:499 csess 0x101776c0 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session
M/TA: TTRC:verifying signature of suit manifest
M/TA: TTRC:verify OK
M/TA: TTRC:command: 20
M/TA: TTRC:execute suit-set-parameters
M/TA: TTRC:command: 1
M/TA: TTRC:execute suit-condition-vendor-identifier
M/TA: TTRC:command: 2
M/TA: TTRC:execute suit-condition-class-identifier
M/TA: TTRC:command: 19
M/TA: TTRC:execute suit-set-parameters
M/TA: TTRC:command: 21
M/TA: TTRC:execute suit-directive-fetch
M/TA: TTRC:fetch_and_store component
M/TA: TTRC:component download 55976
M/TA: TTRC:ta-store.c: store_component() store component
M/TA: TTRC:  device   = TEEP-Device
M/TA: TTRC:  storage  = SecureFS
M/TA: TTRC:  filename = 8d82573a-926d-4754-9353-32dc29997f74.ta
M/TA: TTRC:  image_len = 55976
D/TC:? 0 tee_ta_init_pseudo_ta_session:283 Lookup pseudo TA 6e256cba-fc4d-4941-ad09-2ca1860342dd
D/TC:? 0 tee_ta_init_pseudo_ta_session:296 Open secstor_ta_mgmt
D/TC:? 0 tee_ta_init_pseudo_ta_session:310 secstor_ta_mgmt : 6e256cba-fc4d-4941-ad09-2ca1860342dd
D/TC:? 0 install_ta:99 Installing 8d82573a-926d-4754-9353-32dc29997f74
D/TC:? 0 tee_ta_close_session:499 csess 0x101769c0 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session
M/TA: TTRC:finish fetch
M/TA: TTRC:command: 3
M/TA: TTRC:execute suit-condition-image-match
M/TA: TTRC:end of command seq
D/TC:? 0 tee_ta_close_session:499 csess 0x10177ec0 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session
D/TC:? 0 destroy_context:298 Destroy TA ctx (0x10177e60)
D/TC:? 0 tee_ta_init_pseudo_ta_session:283 Lookup pseudo TA 8d82573a-926d-4754-9353-32dc29997f74
D/TC:? 0 load_ldelf:704 ldelf load address 0x40006000
D/LD:  ldelf:134 Loading TA 8d82573a-926d-4754-9353-32dc29997f74
D/TC:? 0 tee_ta_init_session_with_context:573 Re-open TA 3a2f8978-5dc0-11e8-9c2d-fa7ae01bbebc
D/TC:? 0 system_open_ta_binary:257 Lookup user TA ELF 8d82573a-926d-4754-9353-32dc29997f74 (Secure Storage TA)
D/TC:? 0 system_open_ta_binary:260 res=0x0
D/LD:  ldelf:169 ELF (8d82573a-926d-4754-9353-32dc29997f74) at 0x40070000
D/TC:? 0 tee_ta_close_session:499 csess 0x10176070 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session
Hello TEEP from TEE!
D/TC:? 0 tee_ta_close_session:499 csess 0x10176870 id 1
D/TC:? 0 tee_ta_close_session:518 Destroy session
D/TC:? 0 destroy_context:298 Destroy TA ctx (0x10176810) 
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
$ git clone https://github.com/mcd500/teep-device.git
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
$ sudo apt -y install doxygen-latex graphviz texlive-full texlive-latex-base latex-cjk-all flex bison
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
