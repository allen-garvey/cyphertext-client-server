# Cyphertext client/server

Encode and decode strings using one-time-pad. `keygen.c` generating a random key for a given string length. `otp_enc_d.c` is a server that listens on a given port for a message and random string and returns an encoded string, `opt_enc.c` is a client that connects with `opt_enc_d.c` to get an encoded string from a message and key. `opt_dec_d.c` and `opt_dec.c` work the same way, but decode an encoded string into a message, given an encoded string and key.

## Dependencies

* gcc 4.8.5 or higher
* POSIX compatible operating system
* Bash (for compile script)

## Getting Started

* Clone or download this repository and `cd` into the project directory
* Make the compile script executable by typing `chmod u+x ./compileall`
* Compile with `./compileall`

## License

Cyphertext client/server is released under the MIT License. See license.txt for more details.