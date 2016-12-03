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
//for file opening errors
#include <errno.h>

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
#endif

/*
 * Error functions
 */
//prints program usage
void printUsage(char *programName){
	fprintf(stderr, "usage: %s <plaintext_file> <key_file> <port>\n", programName);
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
    	fprintf(stderr, "Port given is out of valid range\n");
    	exit(2);
  	}
  	return portNum;
}

/*
* Get data from server functions
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




/*
 * File opening and validation functions
 */

//print relevant error message when file
//can't be opened
void printFileOpenError(int errorNum, char *fileName){
	switch(errorNum){
		//permissions error
		case EACCES:
			fprintf(stderr, "Permission denied to open %s\n", fileName);
			break;
		//file doesn't exist
		case ENOENT:
			fprintf(stderr, "%s doesn't exist\n", fileName);
			break;
		//unspecified error
		default:
			fprintf(stderr, "Could not open %s\n", fileName);
		break;
		}
}

//opens a file and returns file pointer
//exits with error if file can't be opened
FILE * openFileByName(char *fileName){
	//attempt to open to specified file for reading
	FILE *filePointer = fopen(fileName, "r");
	//check if it succeed
	if(filePointer == NULL){
		//print error, since we couldn't open file
		printFileOpenError(errno, fileName);
		exit(1);
	}


	return filePointer;
}

//check line to see if it contains invalid characters 
//(anything except uppercase characters or spaces)
//returns 1 if it doesn't, 0 if it does
int isValidLine(char *line){
	//subtract 1 from length, since last character will be newline
	int length = strlen(line) - 1;
	int i;
	for(i=0;i<length;i++){
		char currentChar = line[i];
		//make sure character is either uppercase letter or space
		if(!isupper(currentChar) && currentChar != ' '){
			return 0;
		}
	}
	return 1;

}

//reads file line by line
//returns number of characters in first line if file contains only valid characters
//or 0 otherwise
//note that file should only contain exactly one line (text file automatically contain extra newline at end of file)
int isFileContentsValid(char *fileName){
	FILE *filePointer = openFileByName(fileName);
	//read file line by line and check that it contains valid characters
	//based on: http://stackoverflow.com/questions/3501338/c-read-file-line-by-line
	//initialize line reading variables
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int returnValue = 0;
	int linesRead = 0;
	while((read = getline(&line, &len, filePointer)) != -1){
		//only one line allowed, so check for last line
		if(linesRead == 1 && strlen(line) == 1 && line[0] == '\n'){
			break;
		}
		//check to see if line contains invalid characters
		else if(linesRead > 0 || !isValidLine(line)){
			returnValue = 0;
			break;
		}
		else{
			returnValue = strlen(line);
		}
		linesRead++;
	}
	//free space allocated for line
	free(line);
	//close file
	fclose(filePointer);
	return returnValue;
}

//checks that file contains valid characters and prints
//message and exits if it doesn't
int checkFileContents(char *fileName){
	int length = isFileContentsValid(fileName);
	if(!isFileContentsValid(fileName)){
		fprintf(stderr, "%s contains characters other than uppercase letters and spaces or is empty\n", fileName);
		exit(1);
	}
	return length;
}


/*
 * Socket/Network functions
 */
//builds server address we will use to connect
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

//connects to server using tcp socket and returns file descriptor
//for socket connection
//exits and prints error message if there is an error
//based on: http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpserver.c
int connectToServer(int portNum){
	//create the tcp socket 
	int serverSocketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocketFileDescriptor < 0){
		fprintf(stderr, "Could not open TCP socket\n");
		exit(2);
	}
	
	//build server's internet address using port number
	struct sockaddr_in serverAddress;
  	buildServerAddress(&serverAddress, portNum);
  	
  	//create a connection with the server
    if(connect(serverSocketFileDescriptor, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0){
        fprintf(stderr, "Could not connect to server on port %d\n", portNum);
        exit(2);
    }
    return serverSocketFileDescriptor;
}

/*
* Helper functions for reading/writing to sockets
*/
//sends message to server identified by file descriptor
void sendToSocket(int serverSocketFileDescriptor, char *message){
  //send message to server
  int charCountTransferred = write(serverSocketFileDescriptor, message, strlen(message));
  //check for errors writing
  if(charCountTransferred < 0){
    fprintf(stderr, "Could not send message to server\n");
    	exit(1);
  }
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


//gets data from client using socket, and saves in data argument
//data should end in \n char, and since that should be the only
//newline char in the string, we will know that receiving from the client is 
//done
void getDataFromServer(int serverSocketFileDescriptor, char *data){
  //use while loop to receive data, since it might take multiple requests
  //specifically do-while, since we need it to run at least one time
  
  //create variables so new data gets added to end of buffer
  //subtract one from buffer size to allow for null char at end
  int bufferSize = MESSAGE_BUFFER_SIZE - 1;
  char *dataCurrentPointer = data;
  do{
    int charCountTransferred = read(serverSocketFileDescriptor, dataCurrentPointer, bufferSize);
    //check that read succeeded
    if(charCountTransferred < 0){
    	fprintf(stderr, "There was a problem receiving data from server\n");
    	exit(1);
    }
    //modify variables so new data gets added on to the end
    bufferSize -= charCountTransferred;
    dataCurrentPointer += charCountTransferred;

  }while(!isDataComplete(data) && bufferSize > 0);

}


//sends first line of file to server (file should only have one line in it)
void sendFileToServer(int serverSocketFileDescriptor, char *fileName){
	FILE *filePointer = openFileByName(fileName);
	//read file line by line and check that it contains valid characters
	//based on: http://stackoverflow.com/questions/3501338/c-read-file-line-by-line
	//initialize line reading variables
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int linesRead = 0;
	//read first line and send to server
	while((read = getline(&line, &len, filePointer)) != -1){
		//only one line allowed, see if this is the first line
		if(linesRead > 0){
			break;
		}
		sendToSocket(serverSocketFileDescriptor, line);

		linesRead++;
	}
	//free space allocated for line
	free(line);
	//close file
	fclose(filePointer);
}




/*
 * Main program
 */
int main(int argc, char *argv[]){
	//validate command line arguments, and get values from arguments
	validateCommandLineArgumentsLength(argc, argv);
	int portNum = getPortNum(argc, argv);
	char *messageFileName = argv[1];
	char *keyFileName = argv[2];

	//check message and key to make sure they contain valid characters
	int messageLength = checkFileContents(messageFileName);
	int keyLength = checkFileContents(keyFileName);
	//check that key is at least as long as message
	if(keyLength < messageLength){
		fprintf(stderr, "Number of characters in key file must be greater than or equal number of characters in message file\n");
		exit(1);
	}

	//initialize buffer for messages to server
	char *messageBuffer = createBuffer(MESSAGE_BUFFER_SIZE);

	//connect to server
	int serverSocketFileDescriptor = connectToServer(portNum);

	//send identification message
	sendToSocket(serverSocketFileDescriptor, CLIENT_IDENTIFICATION_HEADER);

	//check for server confirmation
	getDataFromServer(serverSocketFileDescriptor, messageBuffer);
	if(strcmp(messageBuffer, OK_MESSAGE) != 0){
		fprintf(stderr, "This program is not authorized to access that server\n");
		exit(1);
	}

	//send key file
	sendFileToServer(serverSocketFileDescriptor, keyFileName);

	//check for server confirmation
	getDataFromServer(serverSocketFileDescriptor, messageBuffer);
	if(strcmp(messageBuffer, OK_MESSAGE) != 0){
		fprintf(stderr, "The server had problems receiving the key file\n");
		exit(1);
	}

	//send message file
	sendFileToServer(serverSocketFileDescriptor, messageFileName);


	//get results of combining key and message file from server and print result
	getDataFromServer(serverSocketFileDescriptor, messageBuffer);
	printf("%s", messageBuffer);

	//free message buffer
	//program might exit due to error before we reach this, which isn't ideal
	//but since the program isn't doing anything after that, the operating system
	//will just reclaim the memory
	//http://stackoverflow.com/questions/654754/what-really-happens-when-you-dont-free-after-malloc
	free(messageBuffer);

	return 0;
}