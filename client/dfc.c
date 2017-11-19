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

struct sockaddr_in serverAddress[MAX_CONN];
socklen_t serverLength[MAX_CONN];
int sockfd[MAX_CONN];
int serverSize = sizeof(serverAddress);
struct hostent *SERVERNAME;

char dfcConfigFilename[50];
char USERNAME[20];
char PASSWORD[20];
char SERVERS[4][50];

int PORT;


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
		printf("----------------------------------------------\n\n");
		
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
	int i = 0;
	char *value;
	char readBuffer[200];
	
	sprintf(dfcConfigFilename,"%s","dfc.conf");
	fp = fopen(dfcConfigFilename, "r");
	if(fp == NULL){
		perror("Error in opening dfc.conf file\n");
		exit(1);
	}
	else{
		int dfcConfigFileSize = getFileSize(fp);
		while(fgets(readBuffer, dfcConfigFileSize, fp) != NULL){
			if(lineLimit){
				//Fetch DFS Server Details
				if((strncmp(readBuffer,"Server",6)==0) || (strncmp(readBuffer,"SERVER",6)==0)){
					printf("readBuffer: %s\n", readBuffer);
					value = strtok(readBuffer, " \t\n");
					value = strtok(NULL, " \t\n");
					
					if(value[3] == '1'){
						value = strtok(NULL, " \t\n");
						strcpy(SERVERS[0], value);
						i = 0;
					}
					if(value[3] == '2'){
						value = strtok(NULL, " \t\n");
						strcpy(SERVERS[1], value);
						i = 1;
					}
					if(value[3] == '3'){
						value = strtok(NULL, " \t\n");
						strcpy(SERVERS[2], value);
						i = 2;
					}
					if(value[3] == '4'){
						value = strtok(NULL, " \t\n");
						strcpy(SERVERS[3], value);
						i = 3;
					}
					printf("SERVERS[%d]: %s\n", i, SERVERS[i]);
					bzero(readBuffer, sizeof(readBuffer));
					i = i%4;
				}
				
			}
			else{
				//Fetch USERNAME
				if(strncmp(readBuffer, "Username", 8) == 0){
					//printf("readBuffer: %s\n", readBuffer);
					value = strtok(readBuffer, " \t\n");
					value = strtok(NULL, " \t\n");
					strcpy(USERNAME, value);
					printf("USERNAME: %s\n", USERNAME);
					bzero(readBuffer, sizeof(readBuffer));
				}
				
				//Fetch PASSWORD
				if(strncmp(readBuffer, "Password", 8) == 0){
					//printf("readBuffer: %s\n", readBuffer);
					value = strtok(readBuffer, " \t\n");
					value = strtok(NULL, " \t\n");
					strcpy(PASSWORD, value);
					printf("PASSWORD: %s\n", PASSWORD);
					bzero(readBuffer, sizeof(readBuffer));
				}
			}//end of else to fetch un pwd
		}//end of while
		fclose(fp);
	}//end of if dfc.conf opens successfully
}



//-------------------------------SEND USER CREDENTIALS------------------------------
int sendUserCredentials(int sockfd){
	int ack;
	//read the file for username and password
	readConfFile(0);
	
	//send username and password to server
	if(send(sockfd, USERNAME, 20, 0) < 0)
		perror("Error in sending Username\n");
	
	if(send(sockfd, PASSWORD, 20, 0) < 0)
		perror("Error in sending Password\n");
	
	if(recv(sockfd, &ack, sizeof(int), 0) < 0)
		perror("Error in receiving ack\n");
	
	if(!ack){
		printf("INVALID USERNAME OR PASSWORD");
		return FAIL;
	}
	return PASS;
}



//---------------------------------RECEIVE FILE (GET)-------------------------------
int receiveFile(int sockfd, char *filenameGet, struct sockaddr_in serverAddress, socklen_t serverLength, char *subDirectory){
	int status;
	int fileSize = 0;
	int receivedSize = 0;
	int recvBytes = 0;
	int writtenSize = 0;
	char receivedFilename[100];
	char *readBuffer;
	readBuffer = malloc(400000);
	FILE *fp;
	
	//send the subDirectory
	if(send(sockfd, subDirectory, 100, 0) < 0)
		perror("Error in sending subDirectory\n");
	
	//receive the filename from the server
	bzero(receivedFilename, sizeof(receivedFilename));
	if(recv(sockfd, receivedFilename, sizeof(receivedFilename), 0) < 0)
		perror("Error in receiving filename\n");
	
	if(recv(sockfd, &status, sizeof(int), 0) < 0)
		perror("Error in receiving file status\n");
	
	if(status == 0)
		printf("FILE DOES NOT EXIST ON SERVER\n!");
	
	if(status == 1 || status == -1){
		//receive the filesize
		if(recv(sockfd, &fileSize, sizeof(int), 0) < 0)
			perror("Error in receiving file size\n");
		
		printf("fileSize: %d\n", fileSize);
		strncat(receivedFilename, "_get", 100);
		printf("receivedFilename: %s\n", receivedFilename);
		
		//open the file to write into it
		fp = fopen(receivedFilename, "w");
		if(fp == NULL){
			printf("Error in opening the receivedFilename\n");
			return -1;
		}

		//start receiving the file
		while(receivedSize < fileSize){
			recvBytes = recv(sockfd, readBuffer, 400000, 0);
			if(recvBytes == 0){
				perror("Error in reading file\n");
				break;
			}
			printf("Size of the file: %d\n", fileSize);
			printf("Number of bytes read now: %d\n", recvBytes);
			
			writtenSize = fwrite(readBuffer, 1, recvBytes, fp);
			printf("Size of the file written: %d", writtenSize);
			
			receivedSize = receivedSize + recvBytes;
			printf("Total size of the file received so far: %d\n\n", receivedSize);
		}
		fclose(fp);
		return PASS;
	}
	else
		return FAIL;
}



//--------------------------------SOCKET CREATION-----------------------------------

//Ref: https://en.wikibooks.org/wiki/C_Programming/Networking_in_UNIX
//Ref: https://courses.cs.washington.edu/courses/cse476/02wi/labs/lab1/client.c
//Ref: https://stackoverflow.com/questions/4181784/how-to-set-socket-timeout-in-c-when-making-multiple-connections
void createSockets(){
	char *serverName;
	char *port;
	
	for(int i=0; i<MAX_CONN; i++){
		serverLength[i] = sizeof(serverAddress[i]);
		serverName = strtok(SERVERS[i], ":");
		port = strtok(NULL, "");
		printf("Server: %s\n", serverName);
		printf("PORT: %s\n", port);
		PORT = atoi(port);
		
		//create socket
		if((sockfd[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			perror("Error creating the socket\n");
		
		//set up the address structure
		SERVERNAME = gethostbyname(serverName);
		memset((char*) &serverAddress[i], 0, sizeof(serverAddress[i]));
		serverAddress[i].sin_family = AF_INET;
		serverAddress[i].sin_port = htons(PORT);
		bcopy((char*)SERVERNAME->h_addr, (char*)&serverAddress[i].sin_addr.s_addr, SERVERNAME->h_length);
		
		//connect to server
		if(connect(sockfd[i], (struct sockaddr *)&serverAddress[i], sizeof(serverAddress[i])) < 0)
			perror("Error in connecting to the socket\n");
		
		struct timeval timeout;
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = 0;
		
		if(setsockopt(sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0)
			perror("Error in setsockopt()\n");			
	}//end of for
}



//-------------------------------CALCULATE MD5SUM------------------------------------
void calculateMD5(char *filenamePut, char md5sum[100]){
	char MD5Command[50];
	FILE *fp;
	strncpy(MD5Command, "md5sum", sizeof("md5sum "));
	strncat(MD5Command, filenamePut, strlen(filenamePut));
	
	fp = popen(MD5Command, "r");
	while(fgets(md5sum, 100, fp) != NULL)
		strtok(md5sum, " \t\n");
	pclose(fp);
}



//-----------------------------SEND FILES TO SERVERS--------------------------------
int sendFile(int sockfd, char *fname, struct sockaddr_in serverAddress, char *subDirectory){
	FILE *fp;
	int fSize;
	int sendBytes;
	size_t readSize;
	char sendBuffer[1024];
	
	if(!(fp = fopen(fname, "r")))
		perror("Error opening the file\n");
	
	fSize = getFileSize(fp);
	printf("Step: sending file size to server\n");
	if(send(sockfd, &fSize, sizeof(int), 0) < 0){
		perror("Error in sending the file\n");
		exit(1);
	}
	
	printf("Step: sending filename\n");
	if(send(sockfd, fname, 100, 0) < 0){
		perror("Error in sending the filename\n");
		exit(1);
	}
	
	printf("Step: sending subDirectory\n");
	if(send(sockfd, subDirectory, 100, 0) < 0){
		perror("Error in sending the subDirectory\n");
		exit(1);
	}
	
	printf("Step: Sending file in progress\n");
	while(!feof(fp)){
		readSize = fread(sendBuffer, 1, sizeof(sendBuffer)-1, fp);
		do{
			sendBytes = send(sockfd, sendBuffer, readSize, 0);
			if(sendBytes < 0)
				perror("Error in sending file chunks\n");
		} while(sendBytes < 0);
		bzero(sendBuffer, sizeof(sendBuffer));
	}
}





//------------------------------------------MAIN-------------------------------------
//Ref: To split the file into equal parts - http://www.theunixschool.com/2012/10/10-examples-of-split-command-in-unix.html
//Ref: To do AES encryption - https://askubuntu.com/questions/60712/how-do-i-quickly-encrypt-a-file-with-aes
int main(int argc, char **argv){
	int choice = 0;
	int i;
	int sendBytes;
	int arrayOfFailedSend[MAX_CONN];
	int receiveFileStatus = 0;
	int md5int;
	int md5Index;
	
	char filenameGet[15];
	char filenamePut[15];
	char subDirectory[20];
	char md5sum[100];
	
	signal(SIGPIPE, SIG_IGN);
	readConfFile(1);
	createSockets();
	
	FILE *fp;
	
	
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
		
		
		//------------------------CHOICE == LIST----------------------------------
		if(choice == LIST){
			
		}//end of choice == LIST
		
		
		//------------------------CHOICE == GET----------------------------------
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
			
			printf("Enter the filename to receive: ");
			scanf("%s", filenameGet);
			printf("Enter the Sub Directory: ");
			scanf("%s", subDirectory);
			printf("Filename: %s\nSubdirectory: %s\n", filenameGet, subDirectory);
			
			int serverForUse[MAX_CONN] = {-1,-1,-1,-1};
			if(!arrayOfFailedSend[0]){
				serverForUse[0] = TRUE;
				if(!arrayOfFailedSend[2]){
					serverForUse[2] = TRUE;
					printf("COMPLETE 1\n");
				}
				else{
					if(!arrayOfFailedSend[1] && !arrayOfFailedSend[3]){
						serverForUse[1] = TRUE;
						serverForUse[3] = TRUE;
					}
					else{
						printf("INCOMPLETE 1\n");
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
				printf("INCOMPLETE 3\n");
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
			
			//to test
			//sendUserCredentials(sockfd[i]);
			
			#if 1
			for(i=0; i<MAX_CONN; i++)
				#else
					for(i=0; i<1; i++)
			#endif
			{
				if(serverForUse[i] == TRUE){
					if(sendUserCredentials(sockfd[i])){
						//SEND_RECV 1
						if(send(sockfd[i], filenameGet, 50, 0) < 0)
							perror("Error in sending filename for get\n");
						
						receiveFileStatus = receiveFile(sockfd[i], filenameGet, serverAddress[i], serverLength[i], subDirectory);
					}
				}
			}//end of double for loop

			for (i=0; i<MAX_CONN; i++)
				serverForUse[i] = -1;
			
			char systemListGetFile[100];
			char decryptCommand[300];
			char fileList[4][100];
			char fileLS[100];
			i = 0;
			bzero(systemListGetFile, sizeof(systemListGetFile));
			
			strncpy(systemListGetFile, "ls -a .", strlen("ls -a ."));
			strncat(systemListGetFile, filenameGet, strlen(filenameGet));
			strncat(systemListGetFile, "*_get*", strlen("*_get*"));
			printf("systemListFiles: %s\n", systemListGetFile);
			
			FILE *filepointer = popen(systemListGetFile, "r");
			while(fgets(fileLS, 100, filepointer) != NULL){
				bzero(fileList[i], sizeof(fileList[i]));				
				strtok(fileLS, " \t\n");
				strncpy(fileList[i], fileLS, sizeof(fileLS));
				i++;
			}
			pclose(filepointer);
			
			bzero(decryptCommand, sizeof(decryptCommand));
			readConfFile(0);
			if(i < MAX_CONN)
				printf("INCOMPLETE FILE\n");
			else{
				char catFilePiecesCommand[300];
				for(i=0; i < MAX_CONN; i++){
					sprintf(decryptCommand, "openssl enc -d -aes-256-cbc -in %s-out de%s -k %s", fileList[i], fileList[i], PASSWORD);
					system(decryptCommand);
				}
				
				sprintf(catFilePiecesCommand, "cat de%s de%s de%s de%s > %s_get", fileList[0], fileList[1], fileList[2], fileList[3],  filenameGet);
				system(catFilePiecesCommand);
				printf("FILE CONCATENATION: SUCCESS");
				bzero(catFilePiecesCommand, sizeof(catFilePiecesCommand));
				//can remove files using rm command here
			}
			
			
			//bzero everything
			bzero(decryptCommand, sizeof(decryptCommand));
			for(i=0; i<MAX_CONN; i++)
				bzero(fileList[i], sizeof(fileList[i]));
			
			printf("\nDONE WITH GET FUNCTION\n");
		}//end of choice == GET
		
		
		//------------------------CHOICE == PUT----------------------------------
		else if(choice == PUT){
			printf("Enter the name of the file you want to send: ");
			scanf("%s", filenamePut);
			printf("Enter the  name of the sub directory: ");
			scanf("%s", subDirectory);
			
			printf("filenamePut: %s", filenamePut);
			printf("sub directory: %s", subDirectory);
			
			if(!(fp = fopen(filenamePut, "r")))
				perror("Error in opening the file\n");
			
			printf("Step: Calculating MD5sum\n");
			calculateMD5(filenamePut, md5sum);
			md5int = md5sum[strlen(md5sum)-1] % 4;
			md5Index = (4-md5int)%4;
			printf("md5Index: %d\n",  md5Index);
			
			printf("Step: Divide the file into four parts\n");
			char sysCmd[100];
			char fname[100];
			bzero(fname, sizeof(fname));
			strncpy(fname, filenamePut, sizeof(filenamePut));
			strncpy(fname, filenamePut, strlen(filenamePut));
			sprintf(sysCmd, "split -n 4 -a 1 -d %s en%s", filenamePut, filenamePut);
			printf("%s\n", sysCmd);
			system(sysCmd);
			
			printf("Step: Encrypting file using AES\n");
			char encryptCmd[100];
			bzero(encryptCmd, sizeof(encryptCmd));
			readConfFile(0); //to fetch password
			for(i=0; i<MAX_CONN; i++){
				sprintf(encryptCmd, "openssl enc -aes-256-cbc -in en%s%d -out %s%d -k %s", filenamePut, i, filenamePut, i, PASSWORD);
				system(encryptCmd);
			}
			
			char fnameIndex[4][100];
			char fIndex[1];
			int finalIndex;
			#if 1
			for(i=0; i<MAX_CONN; i++)
			#else
			for(i=0; i<1; i++)
			#endif
			{
				if(!arrayOfFailedSend[i]){
					if(sendUserCredentials(sockfd[i])){
						//sending first piece of file
						printf("Step: Calculating index for first piece of file\n");
						finalIndex = (i + md5Index) % 4;
						strncpy(fnameIndex[finalIndex], filenamePut, sizeof(filenamePut));
						sprintf(fIndex, "%d", finalIndex);
						printf("File Index: %s\n", fIndex);
						strncat(fnameIndex[finalIndex], fIndex, 1);
						printf("Filename: %s\n", fnameIndex[finalIndex]);
						printf("Step: Sending first piece of file\n");
						sendFile(sockfd[i], fnameIndex[finalIndex], serverAddress[i], subDirectory);
						sleep(1);
						
						//sending second piece of file
						printf("Step: Calculating index for second piece of file\n");
						strncpy(fnameIndex[(finalIndex+1)%4], filenamePut, sizeof(filenamePut));
						sprintf(fIndex, "%d", (finalIndex+1)%4);
						printf("File Index: %s\n", fIndex);
						strncat(fnameIndex[(finalIndex+1)%4], fIndex, 1);
						printf("Filename: %s\n", fnameIndex[(finalIndex+1)%4]);
						printf("Step: Sending second piece of file\n");
						sendFile(sockfd[i], fnameIndex[finalIndex], serverAddress[i], subDirectory);
						
						//bzero everything
						bzero(fnameIndex[finalIndex], sizeof(fnameIndex[finalIndex]));
						bzero(fnameIndex[(finalIndex+1)%4], sizeof(fnameIndex[(finalIndex+1)%4]));
						bzero(fIndex, sizeof(fIndex));
					}
				}
			}
		}//end of choice == PUT
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
	}//end of while(1)
}