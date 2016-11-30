#include <stdlib.h>
#include <stdio.h>
//for INT_MAX
#include <limits.h>
//for seeding random number generator
#include <sys/time.h>

//minimum and maximum values for key length
#define KEY_LENGTH_MIN 1
#define KEY_LENGTH_MAX INT_MAX

//key base is 27, because there are 27 different characters (which means digits should be between 0-26)
#define KEY_BASE 27


//print program usage
//based on: http://forum.codecall.net/topic/61791-writing-to-stderr-in-c/
//return value should be used as program exit code
int printUsage(const char *programName){
	fprintf(stderr, "usage: %s <key_length>\n", programName);
	return 1;
}

//returns 1 (true) if keyLength 
//is in the valid range, 0 (false) otherwise
int isValidKeyLength(int keyLength){
	return keyLength >= KEY_LENGTH_MIN && keyLength <= KEY_LENGTH_MAX;
}

//returns a random integer using key base
//based on: https://www.tutorialspoint.com/c_standard_library/c_function_rand.htm
int getRandInt(){
	return rand() % KEY_BASE;
}

//converts random integer to a character
//random numbers are 0-26 (because base is 27, that means 26 should be the highest number)
//function returns character either A-Z (capital) or space char
char randIntToChar(int num){
	//highest number, or numbers out of range return ' ' (space character)
	if(num >= 26 || num < 0){
		return ' ';
	}
	//uppercase A is ASCII char 65
	int offset = 65;
	return num + offset;
}


int main(int argc, char const *argv[]){
	//check for required number of arguments
	if(argc != 2){
		return printUsage(argv[0]);
	}
	//atoi will return 0, if keylength is non-numeric, which is fine because 0
	//is invalid anyway
	int keyLength = atoi(argv[1]);
	//check that keyLength is in the valid range
	if(!isValidKeyLength(keyLength)){
		return printUsage(argv[0]);
	}

	//initialize random number generator
	//based on: http://stackoverflow.com/questions/322938/recommended-way-to-initialize-srand
	struct timeval time; 
    gettimeofday(&time, NULL);
    //seed will only repeat once every 24 days
    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

	//output random characters (A-Z and spaces) for the length of keyLength
	int i;
	for(i = 0; i < keyLength; ++i){
		printf("%c", randIntToChar(getRandInt()));
	}

	//last char in file must be newline
	printf("\n");


	return 0;
}