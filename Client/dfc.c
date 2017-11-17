#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

#define PASS 1
#define FAIL 0
#define TRUE 1
#define FALSE 0

#define LIST 1
#define GET 2
#define PUT 3
#define EXIT 4

#define MAX_CONN 4
struct sockaddr_in serverAddress[MAX_CONN];
socklen_t serverLength[MAX_CONN];
int sockfd[MAX_CONN];
int serverSize = sizeof(serverAddress);
struct hostent *server;


//--------------------------------------USER MENU------------------------------------
int userMenu(){
	char command[6];
	int choice = 0;
	
	while(1){
		printf("\n----------------------------------------------\n");
		printf("LIST: get a list of files on the server\n");
		printf("PUT: upload a file to the server\n");
		printf("GET: get a file from the server\n");
		printf("EXIT: close the connection\n");
		printf("\n----------------------------------------------\n\n");
		
		printf("Please enter a command:  ");
		scanf("%s", command);
		
		if(strcmp(command, "list") == 0){
			choice = LIST;
			printf("\nCommand entered: %s", command);
			break;
		}
		else if(strcmp(command, "get") == 0){
			choice = GET;
			printf("\nCommand entered: %s", command);
			break;
		}
		else if(strcmp(command, "put") == 0){
			choice = PUT;
			printf("\nCommand entered: %s", command);
			break;
		}
		else if(strcmp(command, "exit") == 0){
			choice = EXIT;
			printf("\nCommand entered: %s", command);
			break;
		}
		else
			printf("\nInvalid command!\n");
	}
	return choice;
}

int main(int argc, char **argv){
	int choice = 0;
	int i;
	int sendBytes;
	int arrayOfFailedSend[MAX_CONN];
	signal(SIGPIPE, SIG_IGN);
	
	
	while(1){
		choice = userMenu();
		printf("\nUser choice: %d\n", choice);
		
		#if 1
		for(i=0; i<MAX_CONN;i++){
			int sendBytes = send(sockfd[i], (void*)&choice, sizeof(int), 0);
			if(sendBytes < 0){
				arrayOfFailedSend[i] = TRUE;
				perror("Error in sending User Choice\n");
			}
			else{
				arrayOfFailedSend[i] = FALSE;
				printf("\nOption sent!");
			}
		}
		
		#else
		int sendBytes = send(sockfd[0], (void*)&choice, sizeof(int), 0);
		if(sendBytes < 0)
			perror("Error in sending User Choice\n");
		
		#endif
		
		int j;
		int dummy = 100;
		
		if(choice == LIST){
			
		}//end of choice == LIST
		
		else if(choice == GET){
			
		}//end of choice == GET
		
		else if(choice == PUT){
			
		}//end of choice == PUT
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
	}//end of while(1)
}