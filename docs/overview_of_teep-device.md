# Overview of TEEP-Device

The TEEP Protocol provides the protocol on a wide range of devices for install, update, and delete Trusted Applications and Personalization Data by Trusted Component Signer or Device Administrators who host Trusted Application Managers (TAMs).

@image html docs/images/overview_of_teep.png
@image latex docs/images/overview_of_teep.png width=\textwidth

The TEEP-Device is an implementation for defining the draft of Trusted Execution Environment Provisioning (TEEP) Protocol at the Internet Engineering Task Force (IETF). The chart above is a simplified diagram of components described in the TEEP Protocol and TEEP Architecture drafts. The TEEP Protocol on the TEEP-Device uses HTTP packets defined by HTTP Transport for Trusted Execution Environment Provisioning.

- TEEP Protocol
  * https://datatracker.ietf.org/doc/html/draft-ietf-teep-protocol
- HTTP Transport for Trusted Execution Environment Provisioning
  * https://datatracker.ietf.org/doc/html/draft-ietf-teep-otrp-over-http
- TEEP Architecutre:
  * https://datatracker.ietf.org/doc/draft-ietf-teep-architecture/

The terminology of Trusted Application (TA) in the old draft was changed to Trusted Component (TC) to express the files installed from TAM to devices that could have both binaries of trusted applications and data files of personalization data. The TA and TC are interchangeable in this documentation.

## Features of TEEP-Device

- The TEEP Protocol defines the protocol format and interaction between the server called Trusted Application Managers (TAM) and the IoT/Edge devices. The TEEP-Device is an implementation of Trusted Execution Environment Provisioning (TEEP) Protocol on the IoT/Edge devices.

- The TEEP-Device provides the requirements of the TEEP-Agent in IETF drafts.

- Uses the tamproto as an implementation of the TAM server.
  * https://github.com/ko-isobe/tamproto

- Provides initiating the protocol from TEEP-Device, downloading a sample Trusted Components (TC) called Hello-TEEP-TA from the TAM, installing it inside TEE, and executing the Hello-TEEP-TA.

- Implemented on top of TA-Ref which provides a portable TEE programming environment among different TEEs on Intel CPU, ARM Cortex-A and RISC-V 64G to provide uniform source codes over OP-TEE on ARM-TrustZone for Cortex-A series and Keystone on RISC-V.

## Components of TEEP-device and TA-Ref

### TEEP-device and TA-Ref Components on Keystone

@image html docs/images/teep-and-taref-on-keystone.png
@image latex docs/images/teep-and-taref-on-keystone.png width=\textwidth

The TEEP-Device is on the TA-Ref withTEE provided by the Keystone project on RISC-V RV64GC CPU. Each TA in the Trusted Aria is protected with Physical memory protection (PMP) which is enabled by RISC-V hardware.

- Keystone project
  * https://keystone-enclave.org/

### TEEP-device and TA-Ref Components on OP-TEE

@image html docs/images/teep-and-taref-on-optee.png
@image latex docs/images/teep-and-taref-on-optee.png width=\textwidth

It is on OP-TEE but highly utilizing the programming environment provided by TA-Ref to simplify the TEEP-Device to be able to build and function on other CPUs with the single source code of TEEP-Agent and Hello-TEEP-TA. The both are using the subset of Global Platform API.

### TEEP-device and TA-Ref Components on SGX

@image html docs/images/teep-and-taref-on-sgx.png
@image latex docs/images/teep-and-taref-on-sgx.png width=\textwidth

The diagram is the ideal implementation of TEEP-Device on SGX. The current TEEP-Device is not utilizing SGX libraries and the SGX enabled CPU which provides SGX capability with SGX SDK. The TEEP-Device is built and executed as a regular user space application at the moment, and enabling the SGX capability is a future activity.

## Directory structure

```
.
+-- README.md
+-- docs                --- Files for generating documentations
+-- hello-app           --- Sample Trusted Application on Linux side for TEEP Protocol
+-- hello-ta            --- Sample Trusted Application on TEE side for TEEP Protocol
+-- include             --- Header files to build hello-app/ta and teep-broker-app/teep-agent-ta
+-- key                 --- Cryptographic keys for TEEP Protocol
+-- libteep             --- Contains libraries used on TEEP-Device
|   +-- libwebsockets   --- HTTP/HTTPS library  https://github.com/warmcat/libwebsockets
|   +-- mbedtls         --- Cryptographic library  https://github.com/ARMmbed/mbed-crypto
|   +-- QCBOR           --- CBOR library  https://github.com/laurencelundblade/QCBOR.git
|   +-- t_cose          --- COSE library  https://github.com/laurencelundblade/t_cose.git
+-- pctest              --- TEEP-Device runs only on PC with Linux for development purpose
+-- platform            --- Build files for supported CPUs
+-- scripts             --- Scripts used for build and running TEEP-Device
+-- teep-agent-ta       --- Main body of handling TEEP Protocol on TEE side
+-- teep-broker-app     --- Main body of handling TEEP Protocol on Linux side
+-- tiny-tam            --- Small TAM implementation
```
