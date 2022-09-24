# Concise Binary Object Representation (CBOR) in TEEP-Device

## Three format representations in TEEP and SUIT

TEEP messages use CBOR binary format contrary to text based JavaScript Object Notation (JSON). The JSON format is widely used on the modern Internet as an easy of use message exchanging format between servers and clients.

The CBOR provides binary format which enables smaller packet size to carry over the Internet as well as light weight parsing for the IoT and embedded devices.

The CBOR has three representations. The Concise Data Definition Language (CDDL), Diagnostic Notation and Binary Representation.

The CDDL is used for defining CBOR syntax of the TEEP messages. The CBOR Diagnostic Notation is a way of expression of actuarial packet format of TEEP messages in CBOR syntax. The CBOR Diagnostic Notation is similar text description to JSON format which are almost interchangeable.

The CBOR Binary Representation is the result of converting the CBOR Diagnostic Notation to the binary. The TAM server and TEEP-Device parse the Binary Representation and handles the TEEP protocol operation.

@image html docs/images/teep-cbor-representation.png
@image latex docs/images/teep-cbor-representation.png width=\textwidth

## TEEP message format

Example of the TEEP message of QueryRequest in both CBOR Diagnostic Notation and Binary Representation. The Diagnostic Notation is expressed in similar text style with JSON when implementing the Query Request by reading the CDDL format.

The TEEP-Device and TAM will exchange the Binary Representation only. When the TEEP-Device receives the QueryRequest message from the TAM, the TEEP-Device will parse the  Binary Representation to the Diagnostic Notation for understanding the contents of the message.

@image html docs/images/teep-message-format-cddl.png
@image latex docs/images/teep-message-format-cddl.png width=0.75\textwidth

@image html docs/images/teep-message-format-diag-notation.png
@image latex docs/images/teep-message-format-diag-notation.png width=0.7\textwidth

@image html docs/images/teep-message-format-bin-rep.png
@image latex docs/images/teep-message-format-bin-rep.png width=0.7\textwidth

## SUIT manifest format

The Update message of TEEP protocol includes the SUIT Manifests for the information of TC and/or carrying the TC itself in the SUIT Manifest. These are the examples of SUIT Manifest in the Update message.

@image html docs/images/suit-message-format-diag-notation.png
@image latex docs/images/suit-message-format-diag-notation.png width=0.85\textwidth

@image html docs/images/suit-message-format-bin-rep.png
@image latex docs/images/suit-message-format-bin-rep.png width=\textwidth
