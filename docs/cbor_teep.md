# Concise Binary Object Representation (CBOR) in TEEP-Device

## Three format representations in TEEP and SUIT

TEEP messages use CBOR binary format contrary to text based JavaScript Object Notation (JSON). The JSON format is widely used on the modern Internet as an easy of use message exchanging format between servers and clients.

The CBOR provides binary format which enables smaller packet size to carry over the Internet as well as light weight parsing for the IoT and embedded devices.

The CBOR has three representaions. The Concise Data Definition Language (CDDL), Diagnostic Notation and Binary Representation.

The CDDL is used for defining CBOR syntax of the TEEP messages. The CBOR Diagnostic Notation is a way of expression of actuarial packet format of TEEP messages in CBOR syntax. The CBOR Diagnostic Notation is similar text description to JSON format which are almost interchangeable.

The CBOR Binary Representation is the result of converting the CBOR Diagnostic Notation to the binary. The TAM server and TEEP-Device parse the Binary Representation and handles the TEEP protocol operation.

@image html docs/images/teep-suit-representation.png
@image latex docs/images/teep-suit-representation.png width=\textwidth

## TEEP message format examples

This is the example of the TEEP message of Query Request in both CBOR Diagnostic Representation and Binary Representation.

TODO: update the example. The example here is obsolete.

@image html docs/images/teep-message-format-example.png
@image latex docs/images/teep-message-format-example.png width=\textwidth

## SUIT manifest format examples

The Update message of TEEP protocol includes the SUIT Manifest for the information of TC and/or carrying the TC itself in the SUIT Manifest. This is the example of SUIT Manifest used inside the Update message.

TODO: update the example. The example here is obsolete.

@image html docs/images/suit-message-format-example.png
@image latex docs/images/suit-message-format-example.png width=\textwidth
