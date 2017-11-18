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
#define TIMEOUT 1

int PORT;

typedef struct credentials{
	char USERNAME[100];
	char PASSWORD[100];
}credentials;
credentials USERDATA;

char *dfsConfigFilename = "dfs.conf";



//----------------------------------GET FILE SIZE------------------------------------
int getFileSize(FILE *fp){
	int fileSize;
	fseek(fp, 0L, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	return fileSize;
}



//------------------------------VALIDATE USER DETAILS-------------------------------
int validateUserDetails(int acceptSock){
	int recvBytes;
	FILE *fp;
	int dfsConfigFileSize;
	char fileBuffer[200];
	char *unpwd;
	int valid = FAIL;
	
	if(recv(acceptSock,USERDATA.USERNAME,100,0) < 0){
		perror("Error in receiving username\n");
		exit(1);
	}
	
	if(recv(acceptSock,USERDATA.PASSWORD,100,0) < 0){
		perror("Error in receiving password\n");
		exit(1);
	}
	
	fp = fopen(dfsConfigFilename, "r");
	if(fp == NULL){
		perror(dfsConfigFilename);
		exit(1);
	}
	
	dfsConfigFileSize = getFileSize(fp);
	while(fgets(fileBuffer,dfsConfigFileSize,fp) != NULL){
		printf("fileBuffer: %s", fileBuffer);
		unpwd = strtok(fileBuffer, " \t\n");
		if((strncmp(unpwd, USERDATA.USERNAME, strlen(USERDATA.USERNAME)) == 0) && (strlen(unpwd) == strlen(USERDATA.USERNAME))){
			unpwd = strtok(NULL, " \t\n");
			if((strncmp(unpwd, USERDATA.PASSWORD, strlen(USERDATA.PASSWORD)) == 0) && (strlen(unpwd) == strlen(USERDATA.PASSWORD)))
				valid = PASS;
		}
	}
	fclose(fp);
	
	if(send(acceptSock, &valid, sizeof(int), 0) < 0)
		perror("Error in sending ack for username and password\n");
	
	printf("Is user credential valid?: %d", valid);
	return valid;
}




//-------------------------SEND FILE TO CLIENT-------------------------------------
void sendFile(int acceptSock, char *filenameGet, struct sockaddr_in clientAddress, int portIndex){
	
}




//----------------------------------MAIN------------------------------------

//Ref: https://stackoverflow.com/questions/30356043/finding-ip-address-of-client-and-server-in-socket-programming
int main(int argc, char **argv){
	int sockfd;
	int acceptSock;
	int recvBytes;
	int choice;
	int dummy;
	int start = FALSE;
	
	char filenameGet[15];
	char filenamePut[15];
	char subDirectory[20];
	
	struct sockaddr_in serverAddress, clientAddress;
	signal(SIGPIPE, SIG_IGN);
	
	//port number
	if(argc < 2){
		perror("Port number is missing\n");
		exit(1);
	}
	PORT = atoi(argv[1]);
	int portIndex = PORT % 5;
	
	//create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		perror("Error creating the socket\n");
	
	bzero((char*) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(PORT);
	
	if(bind(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
		perror("Error in bind()\n");
	
	int clientSize = sizeof(clientAddress);
	listen(sockfd,5);
	acceptSock = accept(sockfd, (struct sockaddr*)&clientAddress, &clientSize);
	if(acceptSock < 0)
		perror("Error in accept()");
	
	while(1){
		if(recv(acceptSock, (void*)&choice, sizeof(int), 0) < 0);
			perror("Error: Failed to receive the user choice\n");
		
		//------------------------CHOICE == LIST-----------------------------
		if(choice == LIST){
		}
		
		
		//------------------------CHOICE == GET-----------------------------
		else if(choice == GET){
			if(recv(acceptSock, (void*)&dummy, sizeof(int), 0) < 0)
				perror("Error: Failed to receive the user choice\n");
			
			if(recv(acceptSock, (void*)&start, sizeof(int), 0) < 0)
				perror("Error in receiving Start Status");
			
			if(start == TRUE){
				printf("Waiting to receive the file name");
				if(validateUserDetails(acceptSock)){
					if(recv(acceptSock, filenameGet, 50, 0) < 0)
						perror("Error receiving filename for GET\n");
					
					printf("Filename requested by client: %s\n", filenameGet);
					
					//send the file to the client
					sendFile(acceptSock, filenameGet, clientAddress, portIndex);
					printf("DONE WITH GET\n");
				}
			}
		}//end of GET
		
		
	}//end of while
	
	
	
	
	
}//end of main
