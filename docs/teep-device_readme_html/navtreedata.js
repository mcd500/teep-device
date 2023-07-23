/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "TEEP-DEVICE", "index.html", [
    [ "Overview of TEEP-Device", "index.html", [
      [ "Use Cases of TEEP", "index.html#autotoc_md1", null ],
      [ "Features of TEEP-Device", "index.html#autotoc_md2", null ],
      [ "Components of TEEP-Device and TA-Ref", "index.html#autotoc_md3", [
        [ "TEEP-Device and TA-Ref Components on Keystone", "index.html#autotoc_md4", null ],
        [ "TEEP-Device and TA-Ref Components on OP-TEE", "index.html#autotoc_md5", null ],
        [ "TEEP-Device and TA-Ref Components on SGX", "index.html#autotoc_md6", null ]
      ] ],
      [ "The design of TEEP Agent  and sample TA of HELLO-TEEP-TA", "index.html#autotoc_md7", null ]
    ] ],
    [ "Operation of TAM and device", "a00089.html", null ],
    [ "Concise Binary Object Representation (CBOR) in TEEP-Device", "a00090.html", [
      [ "Three format representations in TEEP and SUIT", "a00090.html#autotoc_md10", null ],
      [ "TEEP Message format", "a00090.html#autotoc_md11", null ],
      [ "SUIT Manifest format", "a00090.html#autotoc_md12", null ]
    ] ],
    [ "Directory structure of source files", "a00091.html", null ],
    [ "Consideration of build machine and development environment", "a00092.html", [
      [ "How to select the build machine", "a00092.html#autotoc_md15", null ],
      [ "How to setup an efficient development environment", "a00092.html#autotoc_md16", null ]
    ] ],
    [ "Build TEEP-Device with Docker", "a00093.html", [
      [ "Preparation for Docker", "a00093.html#autotoc_md18", [
        [ "Install Docker", "a00093.html#autotoc_md19", null ],
        [ "Executing Docker without sudo", "a00093.html#autotoc_md20", null ],
        [ "Create a Docker network tamproto", "a00093.html#autotoc_md21", null ]
      ] ],
      [ "Docker Images with pre-built TEEP-Device", "a00093.html#autotoc_md22", null ],
      [ "Preparation to build TEEP-Device on Docker", "a00093.html#autotoc_md23", [
        [ "List of Docker images to build TEEP-Device", "a00093.html#autotoc_md24", null ]
      ] ],
      [ "Run tamproto (TAM Server) - Required by all Keystone/OP-TEE/SGX", "a00093.html#autotoc_md25", [
        [ "Build TEEP-Device for Keystone with Docker", "a00093.html#autotoc_md26", null ],
        [ "Build TEEP-Device for OP-TEE with Docker", "a00093.html#autotoc_md27", null ],
        [ "Build TEEP-Device for SGX with Docker", "a00093.html#autotoc_md28", null ]
      ] ],
      [ "Generate Documentation", "a00093.html#autotoc_md29", [
        [ "Start the container", "a00093.html#autotoc_md30", null ],
        [ "Generate pdf and html documentation", "a00093.html#autotoc_md31", null ]
      ] ]
    ] ],
    [ "Build TEEP-Device without Docker and for Development boards", "a00094.html", [
      [ "Prerequisite", "a00094.html#autotoc_md33", null ],
      [ "Run tamproto (TAM Server) - Required by all Kestone/OP-TEE/SGX", "a00094.html#autotoc_md34", null ],
      [ "Keystone", "a00094.html#autotoc_md35", [
        [ "Clone and Build", "a00094.html#autotoc_md36", null ],
        [ "Run hello-app & teep-broker-app on QEMU Environment", "a00094.html#autotoc_md37", null ],
        [ "Run hello-app and teep-broker-app on RISC-V Unleashed", "a00094.html#autotoc_md38", [
          [ "Copy the hello-app and teep-broker-app binaries to Unleashed", "a00094.html#autotoc_md39", null ],
          [ "Run hello-app and teep-broker-app on Unleased", "a00094.html#autotoc_md40", null ]
        ] ]
      ] ],
      [ "OP-TEE", "a00094.html#autotoc_md41", [
        [ "Clone and Build", "a00094.html#autotoc_md42", null ],
        [ "Run hello-app & teep-broker-app on QEMU Environment", "a00094.html#autotoc_md43", null ],
        [ "Run hello-app and teep-broker-app on Raspberry PI 3", "a00094.html#autotoc_md44", [
          [ "Run hello-app and teep-broker-app on Raspberry PI 3", "a00094.html#autotoc_md45", null ]
        ] ]
      ] ],
      [ "SGX", "a00094.html#autotoc_md46", [
        [ "Clone and Build", "a00094.html#autotoc_md47", null ],
        [ "Run hello-app & teep-broker-app on Intel SGX", "a00094.html#autotoc_md48", null ]
      ] ],
      [ "Generate Documentation", "a00094.html#autotoc_md49", [
        [ "Required Packages", "a00094.html#autotoc_md50", null ],
        [ "Build and Install Doxygen", "a00094.html#autotoc_md51", null ],
        [ "Generate pdf and html documentation", "a00094.html#autotoc_md52", null ]
      ] ]
    ] ],
    [ "Build TEEP-Device without having TEE installed", "a00095.html", null ]
  ] ]
];

var NAVTREEINDEX =
[
"a00089.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';