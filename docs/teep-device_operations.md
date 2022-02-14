# TEEP-DEVICE Operations

@image html docs/images/teep-operations.png
@image latex docs/images/teep-operations.png width=\textwidth

TEEP Protocol defines four messages, QueryRequest, QueryResponse, Update, and Success-Error.

The device initiates the first packet by sending an empty HTTP POST to the TAM server, and then the TAM will be sending the QueryRequest to ask the capability of the device. The return contains the supported cryptographic algorithm, installed TCs, and etc.

If the TAM decides the TC must be installed to the device or update the previously installed TC, then the TAM will send the Update message and the device will process it. The Update message could contain the TC in the body of the Update message or have URI pointing to the location of the TC hosted elsewhere. The result of processing the Update message in the device is reported to the TAM with a Success-Error message.

