# Overview of TEEP-Device

There is a wiki page describing Introduction, objective and use cases of TEEP Protocol.
[https://github.com/ietf-teep/teep-protocol/wiki](https://github.com/ietf-teep/teep-protocol/wiki)

The TEEP Protocol provides the protocol on a wide range of devices for install, update, and delete Trusted Applications and Personalization Data by Trusted Component Signer or Device Administrators who host Trusted Application Managers (TAMs).

@image html docs/images/overview_of_teep.png
@image latex docs/images/overview_of_teep.png width=\textwidth

The TEEP-Device is an implementation for defining the draft of Trusted Execution Environment Provisioning (TEEP) Protocol at the Internet Engineering Task Force (IETF). The chart above is a simplified diagram of components described in the TEEP Protocol and TEEP Architecture drafts. The TEEP Protocol on the TEEP-Device uses HTTP packets defined by HTTP Transport for Trusted Execution Environment Provisioning.

Following are the explanations of each component on the above diagram.

Trusted Application (TA): An application that runs in a TEE.

Trusted Application Manager (TAM): An entity that manages Trusted
      Applications and other Trusted Components running in TEEs of
      various devices.

TEEP Broker: A TEEP Broker is an application component running in
      a Rich Execution Environment (REE) that enables the message
      protocol exchange between a TAM and a TEE in a device.  A TEEP
      Broker does not process messages on behalf of a TEE, but merely is
      responsible for relaying messages from the TAM to the TEE, and for
      returning the TEE's responses to the TAM.

TEEP Agent: The TEEP Agent is a processing module running inside a
      TEE that receives TAM requests (typically relayed via a TEEP
      Broker that runs in an REE).  A TEEP Agent in the TEE may parse
      requests or forward requests to other processing modules in a TEE,
      which is up to a TEE provider's implementation.

The terminology of Trusted Application (TA) in the old draft was revised to Trusted Component (TC) to express the files installed from TAM to devices that could have both binaries of trusted applications and data files of personalization data. The TA and TC are interchangeable in this documentation.

The TEEP Protocol relies on the Software Updates for Internet of Things (SUIT) Manifest defined at IETF which expresses metadata of the TC. When the TAM sends one of the TEEP Messages called Update Message, it will include SUIT Manifest for installing TC to the Device.

The TEEP Protocol and SUIT Manifest are both in binary formats of Concise Binary Object Representation (CBOR). The Objective of CBOR is designed to reduce the message size as much as possible in the way of light way parsing which is appropriate for Constraint Hardware such as IoT, Edge and Embedded devices with limited CPU processing power and memory size. The CBOR has a similar representation of JSON that is widely used on the Internet and makes constructing CBOR easier to use.

The authentication feature of CBOR binaries in TEEP Messages and SUIT Manifests is proved by CBOR Object Signing and Encryption (COSE) required by TAM, Device, Developer and Owner of TC and etc. The COSE defined the method of Signing and Encryption of CBOR formats.

More details can be found in the URLs at iETF.
- TEEP Protocol
  * https://datatracker.ietf.org/doc/html/draft-ietf-teep-protocol
- HTTP Transport for Trusted Execution Environment Provisioning
  * https://datatracker.ietf.org/doc/html/draft-ietf-teep-otrp-over-http
- TEEP Architecture:
  * https://datatracker.ietf.org/doc/draft-ietf-teep-architecture/
- SUIT Manifest:
  * https://datatracker.ietf.org/doc/draft-ietf-suit-manifest/
- CBOR
  * https://datatracker.ietf.org/doc/rfc8949/
- COSE
  * https://datatracker.ietf.org/doc/rfc8152/

## Use Cases of TEEP

Typical use cases for TEEP Protocol is a firmware update Over The Air (OTA) which TC containing a firmware binary, installing security sensitive applications used for payment, playing video with DRM, insurance software, enabling hardware feature with license keys, telemetry software, and softwares handles personal identification data, such as Social Security Number, and vaccination status.

## Features of TEEP-Device

- The TEEP Protocol defines the protocol format and interaction between the server called Trusted Application Managers (TAM) and the IoT/Edge devices. The TEEP-Device is an implementation of Trusted Execution Environment Provisioning (TEEP) Protocol on the IoT/Edge devices.

- The TEEP-Device provides the requirements of the TEEP-Broker and TEEP-Agent in IETF drafts.

- Uses the tamproto as an implementation of the TAM server.
  * https://github.com/ko-isobe/tamproto

- Provides initiating the protocol from TEEP-Device, downloading a Trusted Components (TC) called Hello-TEEP-TA as a sample of TC from the TAM, installing it inside TEE, and executing the Hello-TEEP-TA.

- Implemented on top of TA-Ref which provides a portable TEE programming environment among different TEEs on Intel CPU, ARM Cortex-A and RISC-V 64G to provide uniform source codes over OP-TEE on ARM-TrustZone for Cortex-A series and Keystone on RISC-V.

- The required features of TEEP-Agent in the draft is implemented as a application in user application privilege level inside TEE in this TEEP-Device to simplify the implementation which ideally should be combined with implementation in higher privilege levels, such as, the runtime in S-Mode and Secure Monitor in M-mode on RISC-V. Therefore, some of the assumed requirements on the draft are not fulfilled with the TEEP-Device. In the product, the features of TEEP-Agent must be enabled through root-of-trust from the boot up of the CPUs, the TCs must be saved in a secure manner and have protection of installed TCs.

- Supports Concise Binary Object Representation (CBOR) for current four TEEP Messages.
  * https://datatracker.ietf.org/doc/html/rfc7049

- Supports SUIT Manifest inside the Update message of TEEP Protocol.
  * https://datatracker.ietf.org/doc/draft-ietf-suit-manifest/

## Components of TEEP-Device and TA-Ref

The Trusted Application Reference (TA-Ref) is a different software stack from this TEEP-Devie. The TA-Ref provides a portable API and SDK among Intel SGX, ARM TrustZone-A and RISC-V Keystone and enables portability for source codes of Trusted Applications among different CPUs.

The API of TA-Ref is a subset of TEE Internal Core API Specification defined by Global Platform.
 - https://globalplatform.org/specs-library/tee-internal-core-api-specification/

### TEEP-Device and TA-Ref Components on Keystone

@image html docs/images/teep-and-taref-on-keystone.png
@image latex docs/images/teep-and-taref-on-keystone.png width=\textwidth

The TEEP-Device is implemented on top of the TA-Ref with TEE provided by the Keystone project on RISC-V RV64GC CPU. Each TA in the Trusted Area is protected with Physical memory protection (PMP) which is enabled by RISC-V hardware.

- Keystone project
  * https://keystone-enclave.org/

### TEEP-Device and TA-Ref Components on OP-TEE

@image html docs/images/teep-and-taref-on-optee.png
@image latex docs/images/teep-and-taref-on-optee.png width=\textwidth

It is on OP-TEE but highly utilizes the programming environment provided by TA-Ref to simplify the TEEP-Device to be able to build and function on other CPUs with the single source code of TEEP-Agent and Hello-TEEP-TA. They both are using the subset of Global Platform API.

### TEEP-Device and TA-Ref Components on SGX

@image html docs/images/teep-and-taref-on-sgx.png
@image latex docs/images/teep-and-taref-on-sgx.png width=\textwidth

The diagram is the ideal implementation of TEEP-Device on SGX. The current TEEP-Device is not utilizing SGX libraries and the SGX enabled CPU which provides SGX capability with SGX SDK. The TEEP-Device is built and executed as a regular user space application at the moment, and enabling the SGX capability is a future activity.

## The design of TEEP Agent  and sample TA of HELLO-TEEP-TA

All three TEEs (Kestone/OP-TEE/SGX) share the same design which requires
  - Binaries of TA
  - Client Application to execute the TA.

The features of TEEP-Agent described in the TEEP Architecture draft are realized as a TA (TEEP-Agent-TA, filename: teep-agent-ta) in this TEEP-Device implementation. The Client App which starts executing the TEEP-Agent-TA is the TEEP-Broker-App (filename: teep-broker-app).

The features of TEEP-Agent do not have to be implemented as a TA in TEEP Architecture draft, ideally it is recommended the features of TEEP-Agent to be implemented inside TEE-os or underlying privileged mode (SMC in OP-TEE and SM in Keystone) to improve preventing from malicious softwares to stop the feature of the TEEP-Agent. The reason for realizing the TEEP-Agent as a TA in the TEEP-Device design is to simplify the implementation only.

| Purpose | Client App| TA Binary|
| ------ | ------ | ------ |
| TEEP | teep-broker-app | teep-agent-ta (teep-agent-ta.so for only sgx) |

The HELLO-TEEP-TA is a sample TA application which prints "Hello TEEP from TEE!". This sample TA is executed after the successful build of target client applications (App-keystone / App-optee / App-sgx).
The HELLO-TEEP-TA itself is not part of the TEEP Architecture design. It is for purely demonstration and debugging purposes of the TEE-Broker-App, TEEP-Agent-TA and the TAM.

In the real scenario, the HELLO-TEEP-TA will have features of Payment Application, DRM for video playback, OTA capability, serial code of enhancing features of the hardware or stopping the entire activity of the device when the drones or power plants fall into the unintended parties.

Following are the pairs needed for execution on all three TEEs.

| Purpose | Client App| TA Binary|
| ------ | ------ | ------ |
| Keystone | App-keystone  | 8d82573a-926d-4754-9353-32dc29997f74.ta |
| OP-TEE | App-optee  | 8d82573a-926d-4754-9353-32dc29997f74.ta |
| Keystone | App-sgx  | 8d82573a-926d-4754-9353-32dc29997f74.ta |


Executing the ./teep-broker-app with tamproto URL will allow the teep-agent-ta to talk with the tamproto server and download the TA Binary.
Once the TA Binary is downloaded, the Client App (App-keystone / App-optee / App-sgx) will run the
downloaded TA Binary.

The filename of downloaded TA Binary is: 8d82573a-926d-4754-9353-32dc29997f74.ta
for all the targets.
