/* 
 * Server for decoding text using one time pad
 * by: Allen Garvey
 * usage: opt_dec_d <port> &
 */


/*
* Constants specific to encoding and decoding
*/
//string used to represent client is the correct one
//should be sent as first message from client
#ifndef ACCEPTED_MESSAGE_HEADER
#define ACCEPTED_MESSAGE_HEADER "DECODE\n"
//number of chars including null char in header
#define ACCEPTED_MESSAGE_HEADER_LENGTH 8
#endif

#ifndef CHARACTER_TRANSFORMATION_FUNCTION_POINTER
#define CHARACTER_TRANSFORMATION_FUNCTION_POINTER &decodeCharacter 
#endif

//with the exception of the above constants, encoding and decoding server should work the same
//so just include the code for the encoding server
#include "opt_enc_d.c"