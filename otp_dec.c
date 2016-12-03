/* 
 * Client for decoding text using one time pad
 * by: Allen Garvey
 * usage: opt_dec <plaintext_file> <key_file> <port>
 */

//the only difference between the encode and decode clients
//is the identification message initially sent to the server
//so we just have to define the constant and include the code
//for the encode client

//used in initial message to server to identify client
#define CLIENT_IDENTIFICATION_HEADER "DECODE\n"
#include "otp_enc.c"