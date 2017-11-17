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

char dfcConfigFilename[50];
char USERNAME[20];
char PASSWORD[20];


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



//--------------------------------GET FILE SIZE------------------------------------
int getFileSize(FILE *fp){
	int fileSize;
	fseek(fp, 0L, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	return fileSize;
}



//-----------------------------READ DFC CONFIG FILE---------------------------------
void readConfFile(int lineLimit){
	FILE *fp;
	char *value;
	char readBuffer[200];
	
	fp = fopen(dfcConfigFilename, "r");
	if(fp == NULL){
		perror("Error in opening dfc.conf file\n");
		exit(1);
	}
	else{
		int dfcConfigFileSize = getFileSize(fp);
		while(fgets(readBuffer, dfcConfigFileSize, fp) != NULL){
			if(check){
				
			}
			else{
				//Fetch USERNAME
				if(strncmp(readBuffer, "Username", 8) == 0){
					printf("readBuffer: %s\n", readBuffer);
					value = strtok(readBuffer, " \t\n");
					value = strtok(NULL, " \t\n");
					strcpy(USERNAME, value);
					printf("USERNAME: %s\n", USERNAME);
					bzero(readBuffer, sizeof(readBuffer));
				}
				
				//Fetch PASSWORD
				if(strncmp(readBuffer, "Password", 8) == 0){
					printf("readBuffer: %s\n", readBuffer);
					value = strtok(readBuffer, " \t\n");
					value = strtok(NULL, " \t\n");
					strcpy(PASSWORD, value);
					printf("PASSWORD: %s\n", PASSWORD);
					bzero(readBuffer, sizeof(readBuffer));
				}
			}//end of else to fetch un pwd
		}//end of while
	}//end of if dfc.conf opens successfully
}



//-------------------------------SEND USER CREDENTIALS------------------------------
int sendUserCredentials(int sockfd){
	//first read the file
	readConfFile(0);
}




//------------------------------------------MAIN-------------------------------------
int main(int argc, char **argv){
	int choice = 0;
	int i;
	int sendBytes;
	int arrayOfFailedSend[MAX_CONN];
	
	char filenameGet[15];
	char filenamePut[15];
	char subDirectory[20];
	
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
			for(i=0; i<MAX_CONN;i++){
				int sendBytes = send(sockfd[i], (void*)&dummy, sizeof(int), 0);
				if(sendBytes < 0){
					arrayOfFailedSend[i] = TRUE;
					perror("Error in sending User Choice\n");
				}
				else{
					arrayOfFailedSend[i] = FALSE;
					printf("\nOption sent!");
				}
			}
			
			printf("Enter the filename to recieve: ");
			scanf("%s", filenameGet);
			printf("Enter the Sub Directory: ");
			scanf("%s", subDirectory);
			printf("Filename: %s\nSubdirectory: %s\n", filenameGet, subDirectory);
			
			int serverForUse[MAX_CONN] = {-1,-1,-1,-1};
			if(!arrayOfFailedSend[0]){
				serverForUse[0] = TRUE;
				if(!arrayOfFailedSend[2]){
					serverForUse[2] = TRUE;
					printf("COMPLETE 1\n")
				}
				else{
					if(!arrayOfFailedSend[1] && !arrayOfFailedSend[3]){
						serverForUse[1] = TRUE;
						serverForUse[3] = TRUE;
					}
					else{
						printf("INCOMPLETE 1\n")
						//goto GET_END;
					}
				}
			}
			else if(!arrayOfFailedSend[1]){
				serverForUse[1] = TRUE;
				if(!arrayOfFailedSend[3]){
					serverForUse[3] = TRUE;
					printf("COMPLETE 2\n");
				}
				else{
					printf("INCOMPLETE 2\n");
					//goto GET_END;
				}
			}
			else{
				printf("INCOMPLETE 3\n")
				//goto GET_END;
			}
			
			for(i=0; i<MAX_CONN; i++)
				printf("serverForUse[%d] = %d\n", i, serverForUse[i]);
			
			int start;
			for(i=0; i<MAX_CONN; i++){
				if(serverForUse[i] == TRUE){
					start = TRUE;
					int sendBytes = send(sockfd[i], (void*)&start, sizeof(int), 0);
					if(sendBytes < 0)
						perror("Error in send\n");
				}
				else{
					start = FALSE;
					int sendBytes = send(sockfd[i], (void*)&start, sizeof(int), 0);
					if(sendBytes < 0)
						perror("Error in send\n");
				}
			}
			
			#if 1
			for(i=0; i<MAX_CONN; i++)
				#else
					for(i=0; i<1; i++)
			#endif
			{
				if(serverForUse[i] == TRUE){
					if(sendUserCredentials(sockfd[i])){
						
					}
				}
			}//end of double for loop

		}//end of choice == GET
		
		else if(choice == PUT){
			
		}//end of choice == PUT
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
	}//end of while(1)
}