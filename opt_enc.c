/* 
 * Client for encoding text using one time pad
 * by: Allen Garvey
 * usage: opt_enc <plaintext_file> <key_file> <port>
 * based on: http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpserver.c
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//for character functions
#include <ctype.h>
//for error checking
#include <assert.h>

//maximum number of characters used for the buffer for messages sent to/from the client
#define MESSAGE_BUFFER_SIZE 131071

//maximum number of requests that allowed to queue up waiting for server to become available
#define REQUEST_QUEUE_SIZE 5

//string used as ok message to client
//so client knows it is ok to send
#define OK_MESSAGE "@OK\n"

//character used to terminate cipher key and message strings
#define DATA_TERMINATING_CHAR '\n'


/*
* Constants specific to encoding and decoding
*/
//string used to identify client to server
//should be sent as first message to server
#ifndef CLIENT_IDENTIFICATION_HEADER
#define CLIENT_IDENTIFICATION_HEADER "ENCODE\n"
//number of chars including null char in header
#define CLIENT_IDENTIFICATION_HEADER_LENGTH 8
#endif

/*
 * Error functions
 */
//prints program usage
void printUsage(char *programName){
	fprintf(stderr, "usage: %s <plaintext_file> <key_file> <port>\n", programName);
}

//prints error message and exits program with error code
//based on: http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpserver.c
void error(char *msg){
	perror(msg);
	exit(1);
}

/*
 * Validate command-line arguments
 */
//validates argument is valid port number
//returns 1 (true) if it is valid, otherwise 0 (false)
int isPortNumValid(int portNum){
	if(portNum > 0 && portNum <= 65535){
		return 1;
	}
	return 0;
}

//Validates command-line arguments for correct number
void validateCommandLineArgumentsLength(int argc, char **argv){
	if(argc != 4){
		printUsage(argv[0]);
		exit(1);
	}
  
}

//validates port num argument is valid and returns it if it is
int getPortNum(int argc, char **argv){
	//get port number from command line arguments
	int portNum = atoi(argv[3]);
  	//check that portNum is valid - if atoi fails, 0 is returned
  	if(!isPortNumValid(portNum)){
    	fprintf(stderr, "Port given is out of valid range");
    	exit(2);
  	}
  	return portNum;
}


int main(int argc, char const *argv[]){
	//validate command line arguments, and get values from arguments
	validateCommandLineArgumentsLength(argc, argv);
	int portNum = getPortNum(argc, argv);
	char *messageFileName = argv[1];
	char *keyFileName = argv[2];



	return 0;
}