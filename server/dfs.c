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
	
	printf("Step: Receiving Username\n");
	if(recv(acceptSock,USERDATA.USERNAME,100,0) < 0){
		perror("Error in receiving username\n");
		exit(1);
	}
	
	printf("Step: Receiving Password\n");
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
	printf("Step: Validating Username and Password\n");
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
	
	printf("Step: Sending ACK for receipt of Username and Password\n");
	if(send(acceptSock, &valid, sizeof(int), 0) < 0)
		perror("Error in sending ack for username and password\n");
	
	printf("Is user credential valid?: %d", valid);
	return valid;
}




//-------------------------SEND FILE TO CLIENT-------------------------------------
void sendFile(int acceptSock, char *filenameGet, struct sockaddr_in clientAddress, int portIndex, int choiceType){
	FILE *fp;
	struct timeval timeout = {2,0};
	
	int packetNumber = 1;
	int packetSize;
	int count = 0;
	int size;
	
	char subFolder[100];
	char sendBuffer[1024], recvBuffer[256];
	char listCommand[200];
	char dfsMainFolder[100];
	char dfsUserFolder[100];
	char filename[200][200];
	char fndot[100];
	char fileExistsCheck[100];
	char name[256][256];
	
	DIR *directory;
	struct dirent *dir;
	size_t readSize, stat;
	
	bzero(dfsMainFolder, sizeof(dfsMainFolder));
	bzero(dfsUserFolder, sizeof(dfsUserFolder));
	bzero(fndot, sizeof(fndot));
	bzero(fileExistsCheck, sizeof(fileExistsCheck));
	fd_set fd;
	
	
	if(recv(acceptSock, subFolder, 100, 0) < 0)
		perror("Error in receiving subFolder name");
	else{
		printf("Sub folder name: %s\n", subFolder);
		
		if(choiceType == LIST){
			if(strcmp(subFolder, "/") == 0){
				sprintf(listCommand, "ls -a DFS%d/%s > DFS%d/%s/.%s", portIndex, USERDATA.USERNAME, portIndex, USERDATA.PASSWORD, filenameGet);
				printf("List command: %s\n", listCommand);
				system(listCommand);
				printf("Sub folder is empty\n");
			}
			else{
				sprintf(listCommand, "ls -a DFS%d/%s/%s > DFS%d/%s/.%s", portIndex, USERDATA.USERNAME, subFolder, portIndex, USERDATA.USERNAME, filenameGet);
				printf("List command: %s\n", listCommand);
				system(listCommand);
			}
		}
		
		if(portIndex == 1){
			strncpy(dfsMainFolder, "DFS1", sizeof("DFS1"));
			strncpy(dfsUserFolder, "./DFS1/", sizeof("./DFS1/"));
			strncpy(dfsUserFolder, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
		}
		else if(portIndex == 2){
			strncpy(dfsMainFolder, "DFS2", sizeof("DFS2"));
			strncpy(dfsUserFolder, "./DFS2/", sizeof("./DFS2/"));
			strncpy(dfsUserFolder, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
		}
		else if(portIndex == 3){
			strncpy(dfsMainFolder, "DFS3", sizeof("DFS3"));
			strncpy(dfsUserFolder, "./DFS3/", sizeof("./DFS3/"));
			strncpy(dfsUserFolder, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
		}
		else if(portIndex == 4){
			strncpy(dfsMainFolder, "DFS4", sizeof("DFS4"));
			strncpy(dfsUserFolder, "./DFS4/", sizeof("./DFS4/"));
			strncpy(dfsUserFolder, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
		}
		
		if(strcmp(subFolder, "/") == 0)
			printf("Sub folder is empty\n");
		else
			sprintf(dfsUserFolder, "%s/%s", dfsUserFolder, subFolder);
		
		printf("dfsUserFolder: %s\n",  dfsUserFolder);
		
		char fn[100];
		bzero(fn, sizeof(fn));
		#if 0
		strcpy(fn, filenameGet);
		#else
		sprintf(fn,".%s",filenameGet);
		#endif
		
		int status = -5;
		if(choiceType != LIST){
			directory = opendir(dfsUserFolder);
			
			if(directory){
				while((dir = readdir(directory)) != NULL){
					if(strncmp(dir->d_name, fn, strlen(filenameGet)) == 0){
						printf("dir->d_name %d\n", dir->d_name[strlen(filenameGet)+1]);
						if((dir->d_name[strlen(filenameGet)+2] >= 0) && (dir->d_name[strlen(filenameGet)+2] <= 3)){
							strcpy(name[count], dir->d_name);
							printf("name[count] %s\n", name[count]);
							count++;
						}
					}
				}
				closedir(directory);
			}
			else
				perror("FILE NOT FOUND\n");
			
			if(count == 0)
				status = 0;
		}
		
		strncat(dfsMainFolder, "/", sizeof("/"));
		strncat(dfsMainFolder, USERDATA.USERNAME, sizeof(USERDATA.USERNAME));
		strncat(dfsMainFolder, "/", sizeof("/"));
		printf("dfsMainFolder: %s\n", dfsMainFolder);
		
		#if 0
			strcpy(fndot, name[choiceType]);
		#else
			if(choiceType != LIST)
				strcat(fndot, name[choiceType]);
			else{
				sprintf(fndot, ".%s", filenameGet);
				if(strcmp(subFolder, "/") != 0)
					sprintf(dfsMainFolder, "%s%s/", dfsMainFolder, subFolder);
			}
		#endif
		
		printf("filenameGet: %s\n", filenameGet);
		printf("FileNameDot: %s\n", fndot);
		
		send(acceptSock, fndot, sizeof(fndot), 0);
		strncat(dfsMainFolder, fndot, sizeof(fndot));
		printf("file Number: %d\n", choiceType);
		
		if(status == 0)
			perror("FILE NOT FOUND\n");
		else if(choiceType >= -1){
			if(strcmp(subFolder, "/") == 0)
				sprintf(fileExistsCheck, "DFS%d/%s/%s", portIndex, USERDATA.USERNAME, fndot);
			else
				sprintf(fileExistsCheck, "DFS%d/%s/%s/%s", portIndex, USERDATA.USERNAME, subFolder, fndot);
			
			printf("fileExistsCheck: %s\n", fileExistsCheck);
			if(access(fileExistsCheck, F_OK) != -1){
				printf("FILE EXISTS");
				status = 1;
				if(send(acceptSock, (void*)&status, sizeof(int), 0) < 0)
					perror("Error in sending file status");
			}
			else{
				status = 0;
				printf("FILE NOT FOUND");
				if(send(acceptSock, (void*)&status, sizeof(int), 0) < 0){
					perror("Error in sending file status");
					exit(1);
				}
			}
		}
		else{
			status = -1;
			if(send(acceptSock, (void*)&status, sizeof(int), 0) < 0){
				perror("Error in sending file status");
				exit(1);
			}
		}
		
		if(status == 1 || status == -1){
			if(!(fp = fopen(dfsMainFolder, "r")))
				perror("Error in opening the file");
			bzero(dfsMainFolder, sizeof(dfsMainFolder));
			
			size = getFileSize(fp);
			printf("The file size is: %d\n", size);
			
			printf("Sending file\n\n");
			if(send(acceptSock, (void*)&size, sizeof(int), 0) < 0){
				perror("Error in sending file size\n");
				exit(1);
			}
			
			while(!feof(fp)){
				readSize = fread(sendBuffer, 1,  sizeof(sendBuffer)-1, fp);
				do{
					stat = send(acceptSock, sendBuffer, readSize, 0);
				}while(stat < 0);
				
				bzero(sendBuffer, sizeof(sendBuffer));
			}
		}
	}//end of else recv subFolder
}






//------------------------------RECEIVE FILE FROM CLIENT----------------------------
int receiveFile(int sockfd, struct sockaddr_in clientAddress, socklen_t clientSize, int portIndex){
	int fSize;
	int recvSize;
	int recvBytes;
	int writtenBytes;
	char fname[100];
	char subDirectory[100];
	char fnameReceived[100];
	char dfsMainFolder[100];
	char dfsUserFolder[150];
	char dfsUsernameAndSubFolder[200];
	char fndot[100];
	char *receiveBuffer;
	FILE *fp;
	
	receiveBuffer = malloc(300000);
	
	printf("Step: Receiving file size\n");
	if(recv(sockfd, &fSize, sizeof(int), 0) < 0)
		perror("Error in receiving the file size\n");
	printf("file size: %d\n", fSize);
	
	printf("Step: Receiving file name\n");
	if(recv(sockfd, &fname, 100, 0) < 0)
		perror("Error in receiving the file name\n");
	printf("file name: %s\n", fname);
	
	printf("Step: Receiving subDirectory\n");
	if(recv(sockfd, &fname, 100, 0) < 0)
		perror("Error in receiving the subDirectory\n");
	printf("subDirectory: %s\n", subDirectory);
	
	bzero(fnameReceived, sizeof(fnameReceived));
	strncat(fnameReceived, fname, 100);
	strncat(fnameReceived, "_recv",100);
	bzero(dfsMainFolder, sizeof(dfsMainFolder));
	
	printf("Step: Creating DFS MAIN directories\n");
	if(portIndex == 1){
		strncpy(dfsMainFolder, "DFS1", sizeof("DFS1"));
		system("mkdir -p DFS1");
		strncpy(dfsUserFolder, "mkdir -p DFS1/", strlen("mkdir -p DFS1/"));
	}
	else if(portIndex == 2){
		strncpy(dfsMainFolder, "DFS2", sizeof("DFS2"));
		system("mkdir -p DFS2");
		strncpy(dfsUserFolder, "mkdir -p DFS2/", strlen("mkdir -p DFS2/"));
	}
	else if(portIndex == 3){
		strncpy(dfsMainFolder, "DFS3", sizeof("DFS3"));
		system("mkdir -p DFS3");
		strncpy(dfsUserFolder, "mkdir -p DFS3/", strlen("mkdir -p DFS3/"));
	}
	else if(portIndex == 4){
		strncpy(dfsMainFolder, "DFS4", sizeof("DFS4"));
		system("mkdir -p DFS4");
		strncpy(dfsUserFolder, "mkdir -p DFS4/", strlen("mkdir -p DFS4/"));
	}
	
	printf("Step: Creating USERNAME directory\n");
	strncat(dfsUserFolder, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
	system(dfsUserFolder);
	
	printf("Step: Creating USERNAME/subdirectory\n");
	if(strcmp(subDirectory, "/") == 0)
		printf("subDirectory is empty\n");
	else{
		sprintf(dfsUsernameAndSubFolder, "mkdir -p %s/%s", dfsUserFolder, subDirectory);
		system(dfsUsernameAndSubFolder);
	}
	
	strncat(dfsMainFolder, "/", sizeof("/"));
	strncat(dfsMainFolder, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
	bzero(fndot, sizeof(fndot));
	
	#if 0
		strcpy(fndot, fname);
	#else
		strncpy(fndot, ".", sizeof("."));
		strcat(fndot, fname);
	#endif
	
	if(strcmp(subDirectory, "/") == 0)
		printf("subDirectory is empty\n");
	else
		sprintf(dfsMainFolder, "%s/%s", dfsMainFolder, subDirectory);
	
	strncat(dfsMainFolder, "/", sizeof("/"));
	strncat(dfsMainFolder, fndot, sizeof(fndot));
	printf("DFS Main Folder: %s", dfsMainFolder);
	
	fp = fopen(dfsMainFolder, "w");
	if(fp == NULL){
		perror("Error in opening the file\n");
		return -1;
	}
	
	printf("Step: Receiving file chunks\n");
	while(recvSize < fSize){
		recvBytes = recv(sockfd, receiveBuffer, 300000, 0);
		writtenBytes = fwrite(receiveBuffer, 1, recvBytes, fp);
		recvSize = recvSize + recvBytes;
		printf("Total filesize received so far: %d", recvSize);
	}
	
	bzero(dfsUserFolder, sizeof(dfsUserFolder));
	fclose(fp);
	return 1;
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
	listen(sockfd,5);
	socklen_t clientSize = sizeof(clientAddress);
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
					sendFile(acceptSock, filenameGet, clientAddress, portIndex, GET);
					printf("DONE WITH GET\n");
				}
			}
		}//end of GET
		
		else if(choice == PUT){
			printf("Step: PUT\n");
			if(validateUserDetails(acceptSock)){
				printf("Step: Receive first piece of file\n");
				receiveFile(acceptSock, clientAddress, clientSize, portIndex);
				
				printf("Step: Receive second piece of file\n");
				receiveFile(acceptSock, clientAddress, clientSize, portIndex);
			}
		}
		
		
	}//end of while
	
	
	
	
	
}//end of main
