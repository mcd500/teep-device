# Operation of TAM and device

@image html docs/images/teep-operations.png
@image latex docs/images/teep-operations.png width=\textwidth

TEEP Protocol defines four messages, QueryRequest, QueryResponse, Update, and Success-Error.

The device initiates the first message by sending an empty HTTP POST Method macket to the TAM server, and then the TAM will be sending the QueryRequest to ask the capability of the device. All the consequent TEEP messages after the first empty HTTP POST are carried over HTTP Request/Response. The return message of the QueryResponse contains the information of supported cryptographic algorithms, installed TCs, etc on the device.

If the TAM decides the TC must be installed to the device or update the previously installed TC, then the TAM will send the Update message and the device will process it. The Update message includes software Updates for Internet of Things (SUIT) Manifest which could contain the TC in the body of the Update message or have URI pointing to the location of the TC hosted elsewhere. The result of processing the Update message in the device is reported to the TAM with a Success-Error message.

The SUIT Manifest provides metadata of TC including dependency of the TC, procedure of installing, method of verifying signature and information of invoking it. The SUIT Manifest is designed to meet the requirements of constrained devices with limited capacity of CPU and memory size of IoT and/or embedded devices.

All the messages are transmitted over HTTP packets in the current implementation. The type of transport layer in the drafts is not limited to HTTP, may use HTTPS or any other method.

