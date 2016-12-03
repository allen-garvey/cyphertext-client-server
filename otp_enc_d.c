/* 
 * Server for encoding text using one time pad
 * by: Allen Garvey
 * usage: opt_enc_d <port> &
 * server based on: http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpserver.c
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
//string used to represent client is the correct one
//should be sent as first message from client
#ifndef ACCEPTED_MESSAGE_HEADER
#define ACCEPTED_MESSAGE_HEADER "ENCODE\n"
//number of chars including null char in header
#define ACCEPTED_MESSAGE_HEADER_LENGTH 8
#endif

#ifndef CHARACTER_TRANSFORMATION_FUNCTION_POINTER
#define CHARACTER_TRANSFORMATION_FUNCTION_POINTER &encodeCharacter 
#endif

/*
 * Error functions
 */
//prints program usage
void printUsage(char *programName){
  fprintf(stderr, "usage: %s <port> &\n", programName);
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

//Validates command-line arguments for port number to listen on
//returns port number to listen on
//based on: http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpserver.c
int validateCommandLineArguments(int argc, char **argv){
  if(argc != 2){
    printUsage(argv[0]);
    exit(1);
  }
  //get port number from command line arguments
  int portNum = atoi(argv[1]);
  //check that portNum is valid - if atoi fails, 0 is returned
  if(!isPortNumValid(portNum)){
    printUsage(argv[0]);
    exit(1);
  }
  return portNum;
}

/*
 * Server setup functions
 */
//builds server address we will use to accept connections
//based on: http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpserver.c
void buildServerAddress(struct sockaddr_in *serverAddress, int portNum){
  //initialize memory by setting it to all 0s
  bzero((char *) serverAddress, sizeof(*serverAddress));
  //set internet address
  serverAddress->sin_family = AF_INET;
  //use system's ip address
  serverAddress->sin_addr.s_addr = htonl(INADDR_ANY);
  //set listen port
  serverAddress->sin_port = htons((unsigned short)portNum);
}

//starts the server listening on portNum on IP address determined by operating system
//returns fileDescriptor for the listening socket
//based on: http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpserver.c
int initializeServer(int portNum){
   //create the server socket 
  int serverSocketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocketFileDescriptor < 0){
    error("ERROR opening TCP socket");
  }

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  int optval = 1; //doesn't really do anything, but required for setsockopt
  setsockopt(serverSocketFileDescriptor, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  //build and initialize server address for ip address and port
  struct sockaddr_in serverAddress;
  buildServerAddress(&serverAddress, portNum);

  //bind server to port
  if(bind(serverSocketFileDescriptor, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0){
    fprintf(stderr, "Could not bind to port %d\n", portNum);
    exit(1);
  }

  //start server listening - set queue length to size of REQUEST_QUEUE_SIZE
  if(listen(serverSocketFileDescriptor, REQUEST_QUEUE_SIZE) < 0){
    error("ERROR on listen");
  }
  return serverSocketFileDescriptor;
}

/*
* Helper functions for reading/writing to sockets
*/
//sends message to client identified by file descriptor
void sendToSocket(int clientSocketFileDescriptor, char *message){
  //send message to client
  int charCountTransferred = write(clientSocketFileDescriptor, message, strlen(message));
  //check for errors writing
  if(charCountTransferred < 0){
    error("ERROR writing to socket");
  }
}

//reads message from client identified by file descriptor and puts message into 
//buffer supplied as second argument
//based on: http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpserver.c
void readFromSocketIntoBuffer(int clientSocketFileDescriptor, char *messageBuffer, int bufferSize){
    //zero the buffer so that old messages do not remain
    bzero(messageBuffer, bufferSize);
    //store message from client in buffer
    int charCountTransferred = read(clientSocketFileDescriptor, messageBuffer, bufferSize);
    //check for errors reading
    assert(charCountTransferred >= 0);
}


/*
* Get data from client functions
*/

//allocates memory for string buffer to store one time pad or message
char * createBuffer(int bufferSize){
  //allocate memory
  char *buffer = malloc(sizeof(char) * bufferSize);
  //check that memory allocation succeeded
  assert(buffer != NULL);
  //initialize memory with null chars
  bzero(buffer, bufferSize);
  return buffer;
}

//removes trailing '\n' char from string
void chompString(char* message){
  int length = strlen(message);
  //check to make sure buffer has length first
  if(length == 0){
    return;
  }
  //remove trailing '\n' by changing it to null char
  if(message[length - 1] == '\n'){
    message[length - 1] = '\0';
  }
}

//receive message from sender and determine if it has the correct header
//used so encode and decode clients do not connect to wrong servers
//returns 1 if client is authorized, and 0 if not
//will send error message to client if it is unauthorized
int isClientAuthorized(int clientSocketFileDescriptor){
  char *message = createBuffer(ACCEPTED_MESSAGE_HEADER_LENGTH);
  //read message sent from client
  readFromSocketIntoBuffer(clientSocketFileDescriptor, message, ACCEPTED_MESSAGE_HEADER_LENGTH);
  //compare message with correct message header
  int compareValue = strcmp(message, ACCEPTED_MESSAGE_HEADER);
  //free memory from message before returning
  free(message);
  //check if client is authorized and return message if not
  if(compareValue != 0){
    sendToSocket(clientSocketFileDescriptor, "ERROR: Client not authorized to connect to this server\n");
    return 0;
  }
  return 1;
}

//returns 1 if data has finished being sent, and 0 if not
//uses presence of control character at the end of the string to know if
//data is finished
int isDataComplete(char *data){
  //sanity check first
  assert(data != NULL);
  //get length, since we need to check last character
  int length = strlen(data);
  //check to see if data is empty, since data must have length
  if(length <= 0){
    return 0;
  }
  //check last char
  return data[length - 1] == DATA_TERMINATING_CHAR;
}

//checks that key is the same length or longer than message
//returns 1 if true, 0 if false
int isValidKeyLength(char *key, char *message){
  int keyLength = strlen(key);
  int messageLength = strlen(message);

  return keyLength >= messageLength;
}

//gets data from client using socket, and saves in data argument
//data should end in \n char, and since that should be the only
//newline char in the string, we will know that receiving from the client is 
//done
void getDataFromClient(int clientSocketFileDescriptor, char *data){
  //use while loop to receive data, since it might take multiple requests
  //specifically do-while, since we need it to run at least one time
  
  //create variables so new data gets added to end of buffer
  //subtract one from buffer size to allow for null char at end
  int bufferSize = MESSAGE_BUFFER_SIZE - 1;
  char *dataCurrentPointer = data;
  do{
    int charCountTransferred = read(clientSocketFileDescriptor, dataCurrentPointer, bufferSize);
    //check that read succeeded
    assert(charCountTransferred >= 0);
    //modify variables so new data gets added on to the end
    bufferSize -= charCountTransferred;
    dataCurrentPointer += charCountTransferred;

  }while(!isDataComplete(data) && bufferSize > 0);

}


/*
* Character encoding/decoding functions
*/

//normalizes ASCII A-Z and space to int from
//0-26 (base 27), with 0 being A and 26 being space
int charToBase27(char c){
  //sanity check for character
  assert(c == ' ' || isupper(c));
  //check for space character
  if(c == ' '){
    return 26;
  }
  //A is ASCII character 65
  int offset = 65;
  return c - offset;
}

//reverse of charToBase27
//converts base 27 number to ASCII char
//A-Z or space (26 is space char, 0 is A)
char base27ToChar(int d){
  //sanity check for range
  assert(d >= 0 && d <= 26);
  if(d == 26){
    return ' ';
  }
  int offset = 65;
  return d + offset;
}

//encodes a character A-Z or space with key char
//normalizes offset on ASCII character codes first
//by adding them together and performing mod % 27 on result
//returns encoded character
char encodeCharacter(char messageChar, char keyChar){
  //convert to base 27 versions
  int base27MessageChar = charToBase27(messageChar);
  int base27keyChar = charToBase27(keyChar);
  //add together, perform modulo so still base 27, and convert to character
  return base27ToChar((base27MessageChar + base27keyChar) % 27);
}

//decodes a character A-Z or space with key char
//by subtracting key from encoded message and performing mod % 27 on result
//returns encoded character
//with normalized offset for ASCII character codes
char decodeCharacter(char messageChar, char keyChar){
  //convert to base 27 versions
  int base27MessageChar = charToBase27(messageChar);
  int base27keyChar = charToBase27(keyChar);
  //subtract key from message, perform modulo so still base 27, and convert to character
  int difference = (base27MessageChar - base27keyChar) % 27;
  //need add encoding base if difference is less than 0
  difference = difference >= 0 ? difference : difference + 27;
  return base27ToChar(difference);
}


//modify message in place, character by character using key
//whether this encodes or decodes message depends on constant definition
//at top of file, as both encoding and decoding files use the same code
void modifyMessage(char *message, char *key, char (*characterTransformationFunction)(char, char)){
  int messageLength = strlen(message);
  int i;
  //modify every character in message using key and transformation function
  for(i = 0; i < messageLength; ++i){
    message[i] = characterTransformationFunction(message[i], key[i]);
  }
  //add terminating character to end of message
  message[messageLength] = DATA_TERMINATING_CHAR;

}

/*
* Main server action
* used by child process
* either encodes plaintext using one time pad or decodes ciphertext into message
* using one time pad, depending on program
* This program encodes message
*/

void mainServerAction(int clientSocketFileDescriptor){
  //check if client is authorized
  if(!isClientAuthorized(clientSocketFileDescriptor)){
    return;
  }
  //allocate memory to store message to be encoded
  char *message = createBuffer(MESSAGE_BUFFER_SIZE);
  //allocate memory to store cipher key
  char *key = createBuffer(MESSAGE_BUFFER_SIZE);
  //send ok message to let client know to send key
  sendToSocket(clientSocketFileDescriptor, OK_MESSAGE);

  //client should now send key, so read that, and save in key variable
  getDataFromClient(clientSocketFileDescriptor, key);
  //remove trailing newline
  chompString(key);

  //send ok message to let client know to send message
  sendToSocket(clientSocketFileDescriptor, OK_MESSAGE);

  //get message from client
  getDataFromClient(clientSocketFileDescriptor, message);
  //remove trailing newline
  chompString(message);

  //check that key is the same length or longer than message
  //send error message to client and exit if not
  if(!isValidKeyLength(key, message)){
    sendToSocket(clientSocketFileDescriptor, "@ERROR: Key is shorter than message\n");
    //free memory from buffers
    free(message);
    free(key);
    return;
  } 

  //modify message to either be encoded or decoded as appropriate, based on constant defined in header
  modifyMessage(message, key, CHARACTER_TRANSFORMATION_FUNCTION_POINTER);

  sendToSocket(clientSocketFileDescriptor, message);

  //free memory from buffers
  free(message);
  free(key);
}



int main(int argc, char **argv){
  //get port number from command-line arguments
  //will print usage and exit if command-line arguments are invalid
  int portNum = validateCommandLineArguments(argc, argv);

  //do server setup and start server listing on portNum
  int serverSocketFileDescriptor = initializeServer(portNum);

  //we aren't using the following 2 variables, but we need them for the accept() function
  //set aside memory for client connection address
  struct sockaddr_in clientAddress;
  //initialize address length, since it is used to accept connection
  uint clientAddressLength = sizeof(clientAddress);

  //used to keep track of process after forking
  int pid;
  //main server listen loop
  while(1){
    //accept connection
    //don't need to close connection, because client should do this
    int clientSocketFileDescriptor = accept(serverSocketFileDescriptor, (struct sockaddr *) &clientAddress, &clientAddressLength);
    if(clientSocketFileDescriptor < 0){
      error("ERROR while trying to accept connection");
    }
    //fork process, since only child process should handle connection
    pid = fork();

    //check for error with forking
    if(pid < 0){
      error("Could not create child process to handle connection");
    }
    //parent should just continue while loop, as it doesn't process connection
    else if(pid > 0){
      continue;
    }

    //only the child process should be here now
    mainServerAction(clientSocketFileDescriptor);
    //close client connection
    close(clientSocketFileDescriptor);
    //child exits loop after performing action
    break;
    
  }

  //we shouldn't ever reach here (in the parent process), as the only way to quit
  //is to manually interrupt or kill process, but in case we do
  //stop server listening if process is parent
  if(pid > 0){
    close(serverSocketFileDescriptor);
  }

  //for child process
  return 0;
}