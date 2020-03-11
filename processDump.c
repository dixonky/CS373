/*******************************************************************************
* ** Author: Kyle Dixon
* ** Date: 11/3/2019-11/10/19 <- As close to finished as I could get
* ** Descriptions: Homework 373: C program for generating processes info
* ** 	Enumerates all the running processes, then user chooses a process for info on
* **	Running threads, loaded modules, and executable pages within the chosed process
* **	Capable of reading memory
* ** Sources: CS 344 Block 3 (processes & smallsh assignment)
	https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
	http://man7.org/linux/man-pages/man5/proc.5.html
	https://stackoverflow.com/questions/33266678/how-to-extract-info-in-linux-with-a-c-code-from-proc
	https://unix.stackexchange.com/questions/6301/how-do-i-read-from-proc-pid-mem-under-linux
	https://nullprogram.com/blog/2016/09/03/
		 *Program referenced in blog https://github.com/skeeto/memdig/blob/master/memdig.c
	http://man7.org/linux/man-pages/man2/process_vm_readv.2.html
	https://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html
	https://stackoverflow.com/questions/36988645/weird-packing-in-iov-writev
*****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <errno.h>

//Global Constraints
#define _GNU_SOURCE
#define CHAR_BUFFER 255
#define MAX_LENGTH 1024
#define MAX_ARG 512

//Prototypes
void userInput(char*[], int*, char[], char[], int);
void procList();
void procInfo();
int procGetMods();
void procGetMem();


//Main Function
int main() {
	int i;
	int pid = getpid();
	int flagContinue = 0;
	int exitStatus = 0;
	int flagBackground = 0;
	char inputFile[CHAR_BUFFER] = "";		//set up holders
	char outputFile[CHAR_BUFFER] = "";
	char* input[MAX_ARG];
	for (i=0; i<MAX_ARG; i++) 
	{
		input[i] = NULL;
	}
	printf("Welcome to Kyle's Processes Program\n");			//Present options to the user
	printf("\ttype 'list' to list all active processes\n");
	printf("\ttype 'pid' to get info on a specific process\n");
	printf("\ttype 'mod' to get the loaded modules\n");
	printf("\ttype 'mem' to print the contents of a specific memory address \n");
	printf("\ttype 'exit' to exit the program\n");
	while (flagContinue == 0) 
	{
		userInput(input, &flagBackground, inputFile, outputFile, pid);	//Get user input
		if (input[0][0] == '#' || input[0][0] == '\0') 		//check user input against saved commands
		{
			continue;		//do nothing if the first character is blank or a comment symbol
		}
		else if (strcmp(input[0], "exit") == 0) 
		{
			flagContinue = 1;	//set the continue flag and exit the while loop
		}
		else if (strcmp(input[0], "list") == 0) 
		{
			procList();	 //call function to list all processes
		}
		else if (strcmp(input[0], "pid") == 0) 
		{
			procInfo(); //call function to get process info
		}
		else if (strcmp(input[0], "mod") == 0) 
		{
			procGetMods();  //call function to get modules currently running
		}
		else if (strcmp(input[0], "mem") == 0)
		{
			procGetMem();  //call function to get contents at memory address
		}
		else 
		{
			printf("Unrecognized command ('exit' to leave program)\n\n");
		}
		for (i=0; input[i]; i++) //clear the input files before continuing to get the user input again
		{
			input[i] = NULL;
		}
		flagBackground = 0;		//clear holders
		inputFile[0] = '\0';
		outputFile[0] = '\0';
	} 
	return 0;
}


//User input Function
	//gets the user input and checks contents for commands
void userInput(char* correctedInput[], int* flagBackground, char input[], char output[], int pid){
	int i, j;
	char userInput[MAX_LENGTH];
	printf(": ");			//prompt to get user input
	fflush(stdout);			//clear stdout
	fgets(userInput, MAX_LENGTH, stdin); //store the char stream into user input
	int flagNewline = 0;			//remove the newline character via checking each char until one is found
	for (i=0; !flagNewline && i<MAX_LENGTH; i++) 
	{
		if (userInput[i] == '\n') 
		{
			userInput[i] = '\0';
			flagNewline = 1;
		}
	}
	const char delim[2] = " ";
	char *symbol = strtok(userInput, delim); //break apart input into series of tokens, check tokens for symbols matching commands !!Symbol checks are a relic of CS 344!!
	for (i=0; symbol; i++) 
	{
		correctedInput[i] = strdup(symbol);
		for (j=0; correctedInput[i][j]; j++) //search for the $$ symbol and replace with the pid 
			{
				if (correctedInput[i][j] == '$' && correctedInput[i][j+1] == '$') 
				{
					correctedInput[i][j] = '\0';
					snprintf(correctedInput[i], CHAR_BUFFER, "%s%d", correctedInput[i], pid);
				}
			}
		symbol = strtok(NULL, delim);
	}
}


//Check Process Function
	//Pass in the process id and the type of process
	//Reads the name of the process, used for creating the pid list
int procGetName( const char* proc_id, int type ){
    char strFile[CHAR_BUFFER];			//set up holders
    char strBuffer[MAX_ARG];
    if(type)							//print the file name in the strFile holder
	{
        sprintf( strFile, "/proc/%s/cmdline", proc_id );
    } 
	else 
	{
        sprintf( strFile, "/proc/%s/comm", proc_id );
    }
    FILE* f = fopen(strFile, "r" );		//open the file
    if(!f)								//validate openning
	{
        sprintf( strFile, "Error: proc %s", proc_id );
        perror( strFile );
        return 0;
    }
    if(!fgets(strBuffer, MAX_ARG, f ))	//save the process name into the holder
	{
        fclose(f);
        if(!type)	
		{
            sprintf(strFile, "Error: cmdline from proc %s", proc_id);
            perror(strFile);
            return 0;
        } 
		else 	//change level and repeat
		{
            return procGetName(proc_id, 0);
        }
    }
    fclose(f);
    int len = strlen(strBuffer);	//used to print the process name to the screen
    if( len > 0 && strBuffer[len-1] == '\n' ) strBuffer[len-1] = 0;		//validate the length and remove the final (newline) char
    if(type)	//print process name according to its type
	{
        printf("%s %s\n", proc_id, strBuffer);
    } 
	else 
	{
        printf("%s [%s]\n", proc_id, strBuffer);
    }
    return 1;
}


//Get process threads function
	//Read everything in the proc/pid/task folder
int procGetTask( const char* proc_id){
	printf("Threads: \n");
    char strFile[CHAR_BUFFER];			//set up holders
    char strBuffer[MAX_ARG];
    DIR *dirp;	//Pointer for reading directories
    struct dirent *dp;
    sprintf(strFile, "/proc/%s/task", proc_id);
  	dirp = opendir(strFile);	//open the thread directory
    if (dirp) //condition to close loop when at the end of the directory
    {
	    while( (dp = readdir(dirp)) != NULL)	//read directory process by process
		{
	        if( dp->d_type == DT_DIR && isdigit( dp->d_name[0] ) )	//check that the file is a process
			{
	            if( !procGetName( dp->d_name, 0 ) )	//call check process function to list process info
				{
	                exit( EXIT_FAILURE );	//if the reading fails, exit
	                closedir(dirp);
	            }
	        }
	    }
        closedir(dirp);
    }
    return 1;
}


//Get process threads function
	//Read everything under the modules folder
int procGetMods(){
	printf("Modules: \n");
    char strFile[CHAR_BUFFER];			//set up holders
    char strBuffer[MAX_ARG];  	
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    FILE* f = fopen("/proc/modules", "r" );		//open the file
   	while ((read = getline(&line, &len, f)) != -1) {
        printf("%s", line);
    }
  	fclose(f);
    return 1;
}


//Get process exe function
	//Read everything in the proc/pid/exe folder
int procGetExe(const char* proc_id){
	char * line = NULL;
    ssize_t read;
	char strFile[CHAR_BUFFER];			//set up holders
	char strBuffer[MAX_ARG];  
    sprintf(strFile, "/proc/%s/exe", proc_id);
	ssize_t len = readlink(strFile, strBuffer, MAX_ARG);
	strBuffer[len]='\0';
	printf("Executable page path: %s\n", strBuffer);
    return 1;
}


//Get process memory function
	//Read everything in the proc/pid/maps folder
	//Use to get memory address for procGetMem(){
int procGetMap( const char* proc_id){
	printf("Memory Map: \n");
	char * line = NULL;
    size_t len = 0;
    ssize_t read;
	char strFile[CHAR_BUFFER];			//set up holders
    sprintf(strFile, "/proc/%s/maps", proc_id);
    FILE* f = fopen(strFile, "r" );		//open the file
   	while ((read = getline(&line, &len, f)) != -1) {
        printf("%s", line);
    }
  	fclose(f);
    return 1;
}

//Get more specific process memory function
	//
void procGetMem(){
	printf("Memory: \n");
	int i;					//set up getting the user input for the pid (same as previous user input)
	int pid = getpid();
	int flagBackground = 0;
	char inputFile[CHAR_BUFFER] = "";
	char outputFile[CHAR_BUFFER] = "";
	char* input[MAX_ARG];
	for (i=0; i<MAX_ARG; i++) 	//clear input
	{
		input[i] = NULL;
	}
	printf("\tEnter the pid\n");
	userInput(input, &flagBackground, inputFile, outputFile, pid);	//Get the pid
	pid_t proc_id = strtol(input[0], NULL, 10);
	inputFile[0] = '\0';
	outputFile[0] = '\0';
	for (i=0; i<MAX_ARG; i++) 	//clear input
	{
		input[i] = NULL;
	}
	printf("\tEnter the full target address\n\t (example: 0x00007efbf7fd7000)\n");
	userInput(input, &flagBackground, inputFile, outputFile, pid);	//Get the memory address
	void *remotePtr = (void *)strtol(input[0], NULL, 0);
	printf("Target address: 0x%llx\n", remotePtr);
	size_t bufferL = 100;
	char buf1[100];
	for (i=0; i<100; i++) //setup buffer
	{
		buf1[i] = NULL;
	}

    struct iovec local[1]; 	//create necessary structures //http://man7.org/linux/man-pages/man2/process_vm_readv.2.html
    local[0].iov_base = buf1;
    local[0].iov_len = bufferL;

    struct iovec remote[1];
    remote[0].iov_base = remotePtr;
    remote[0].iov_len = bufferL;

    ssize_t nread = process_vm_readv(proc_id, local, 2, remote, 1, 0); //http://man7.org/linux/man-pages/man2/process_vm_readv.2.html
    printf("Executed and read %zd bytes.\n", nread);
	if (nread < 0) {
		switch (errno) { 	//https://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html
		    case EINVAL:
		      printf("ERROR: arguments\n");
		      break;
		    case EPERM:
		      printf("ERROR: privilege\n");
		      break;
		    case ESRCH:
		      printf("ERROR: process id\n");
		      break;
		    case EFAULT:
		      printf("ERROR: accessing target memory address\n");
		      break;
		    case ENOMEM:
		      printf("ERROR: memory allocation\n");
		      break;
		    default:
		      printf("ERROR: unknown\n");
		}
	}
	//https://stackoverflow.com/questions/36988645/weird-packing-in-iov-writev
	printf("At memory location: Content: %d, IOV_BASE &: %p, IOV_BASE: %p, IOV_LEN &: %p, IOV_LEN: %#zx\n", (void *) &local[0].iov_base,(void *) &local[0].iov_base, local[0].iov_base,(void *) &local[0].iov_len, local[0].iov_len);
}


//List Process List Function
	//prints all of the processes (calls procGetName)
void procList() {
    DIR *dirp;	//Pointer for reading directories
    struct dirent *dp;
  	dirp = opendir("/proc");	//open the proc directory
    if (dirp) //condition to close loop when at the end of the directory
    {
	    while( (dp = readdir(dirp)) != NULL)	//read directory process by process
		{
	        if( dp->d_type == DT_DIR && isdigit( dp->d_name[0] ) )	//check that the file is a process
			{
	            if( !procGetName( dp->d_name, 1 ) )	//call check process function to list process info
				{
	                exit( EXIT_FAILURE );	//if the reading fails, exit
	                closedir(dirp);
	            }
	        }
	    }
        closedir(dirp);
    }
    return;
}


//Process Info Function
	//get pid from user and call functions getting info on pid
void procInfo() {
	int i;					//set up getting the user input for the pid (same as previous user input)
	int pid = getpid();
	int flagContinue = 0;
	int exitStatus = 0;
	int flagBackground = 0;
	char inputFile[CHAR_BUFFER] = "";
	char outputFile[CHAR_BUFFER] = "";
	char* input[MAX_ARG];
	for (i=0; i<MAX_ARG; i++) 	//clear input
	{
		input[i] = NULL;
	}
	printf("\tEnter the number of the wanted process\n");
	printf("\tYou will receive active threads, executables, and memory locations\n");
	printf("\tUse the memory address for the mem command later!\n");
	printf(": ");
	while (flagContinue == 0) 
	{
		userInput(input, &flagBackground, inputFile, outputFile, pid);	//Get user input (same as previous user input)
		int inputNum = atoi(input[0]);
		printf("Process: %d\n", inputNum);
		DIR *dirp;	//Pointer for reading directories
    	struct dirent *dp;
  		dirp = opendir("/proc");	//open the proc directory
	    if (dirp) //condition to close loop when at the end of the directory
	    {
		    while( (dp = readdir(dirp)) != NULL)	//read directory process by process
			{
		        if( dp->d_type == DT_DIR && strcmp(input[0], dp->d_name)==0 )	//check for the wanted file
				{
		            if( !procGetTask( dp->d_name) )	//get the process active threads
					{
		                exit( EXIT_FAILURE );	//if the reading fails, exit
		                closedir(dirp);
		            }
		            if( !procGetExe( dp->d_name) )	//get the executable page path
					{
		                exit( EXIT_FAILURE );	//if the reading fails, exit
		                closedir(dirp);
		            }
		            if( !procGetMap( dp->d_name) )	//get the memory dump
					{
		                exit( EXIT_FAILURE );	//if the reading fails, exit
		                closedir(dirp);
		            }
		        }
		    }
		    closedir(dirp);
	    }
		flagContinue = 1;	//For now loop is set up for only one pid
	} 
	return;
}
