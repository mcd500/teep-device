# Directory structure of source files

TODO: Update to current structure.

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

