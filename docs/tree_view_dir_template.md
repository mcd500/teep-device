# Directory structure of source files

Following are the important directories in the source code along with its description

| Directory| Description|
| ------ | ------ |
| docs | Files for generating documentaions |
| hello-tc | Sample Trusted Application for TEEP Protocol |
| include | Header files to build hello-app/ta and teep-broker-app/teep-agent-ta | 
| key | cryptograpic keys for TEEP protocol |
| lib | contains libraries used on TEEP-Device |
| submodules |Contains the submodules used for TEEP-Device |
| submodules/libwebsockets | HTTP/HTTP library https://github.com/warmcat/libwebsockets |
| submodules/mbedtls | Cryptographic library  https://github.com/ARMmbed/mbed-crypto |
| submodules/QCBOR | CBOR library  https://github.com/laurencelundblade/QCBOR.git |
| submodules/t_cose | COSE library  https://github.com/laurencelundblade/t_cose.git |
| teep-agent-ta | Main body of handling TEEP Protocol on TEE side |
| teep-broker-app | Main body of handling TEEP Protocol on Linux side |

**TEEP-Device Source Code**

The below is the current TEEP-Device source code listing only the directories.

DYNAMIC_SOURCE_UPDATE_TAG