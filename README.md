# Broadcaster

A QNX process for broadcasting data over an RN2483 LoRa radio module using a UART connection.

## Usage

See the help documentation with the command: `use broadcaster`.

This program expects input through the form of a file or via `stdin`. The input should be packet data written in ASCII
using only valid hexadecimal characters. A newline character is used to separate packets for transmission.

All radio parameters are configurable via command line arguments. The default values are listed in the help docs
available by calling `use broadcaster`.
