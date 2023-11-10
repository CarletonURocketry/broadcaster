# Broadcaster

A QNX process for broadcasting data over an RN2483 LoRa radio module using a UART connection.

## Usage

See the help documentation with the command: `use broadcaster`.

This program expects input through the form of a file or via `stdin`. The input should be packet data written in ASCII
using only valid hexadecimal characters. A newline character is used to separate packets for transmission.

All radio parameters are configurable via command line arguments. The default values are listed in the help docs
available by calling `use broadcaster`.

**WARNING:**
LoRa radio parameters can be set using command-line to the program. You should not assume that the defaults match the
settings for the CUInSpace ground station. For instance, the ground station assumes cyclic redundancy check to be
enabled, but it is disabled by default in `broadcaster`. You will need to pass the `-c` flag to enable it.

## Development

Please visit the GitHub wiki for developer resources.
