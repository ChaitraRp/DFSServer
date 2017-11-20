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

#define BUFLEN 10240

#define GET 1
#define PUT  2
#define LIST 3
#define MKDIR 4
#define EXIT 5

#define VALID 1
#define INVALID 0

#define PASS 1
#define FAIL 0
#define TRUE 1
#define FALSE 0

#define MAX_CONN 4
#define TIMEOUT 1

typedef struct credentials{
  char USERNAME[100];
  char PASSWORD[100];
}credentials;

credentials USERDATA;

char dfsConfigFilename[100];
int PORT;
  

  
//--------------------------------GET FILE SIZE------------------------------------
static unsigned int getFileSize(FILE *fp){
	unsigned int fileSize;
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
	
	//username
	printf("Step: Receiving Username\n");
	if(recv(acceptSock,USERDATA.USERNAME,100,0) < 0){
		perror("Error in receiving username\n");
		exit(1);
	}

	//password
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
	
	//validation part
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
	
	return valid;
}





//-------------------------SEND FILE TO CLIENT-------------------------------------
int sendFile(int acceptSock, char *filename, struct sockaddr_in clientAddress, int portIndex, int fileNumber){
	FILE *fp;
	struct timeval timeout = {2,0};
	
	int fSize;
	int count = 0;
	int status = -5;
	int n;
	char subDirectory[100];
	char name[256][256];
	char sendBuffer[1024];
	char recvBuffer[256];
	char listCommand[200];
	char dfsMainFolder[100];
	char dfsUserFolder[100];
	char fn[100];
	char fndot[100];
	char fileExistsCheck[200];
	
	DIR *directory;
	struct dirent *dir;
	size_t  readSize, recvBytes;
	
	bzero(listCommand, sizeof(listCommand));
	bzero(dfsMainFolder, sizeof(dfsMainFolder));
	
	recvBytes = recv(acceptSock, subDirectory, 100, 0);
	if(recvBytes < 0)
		perror("Error in receiving subDirectory name");


	//this here is for list command
	if(fileNumber == -1){
		if(strcmp(subDirectory, "/") == 0){
			sprintf(listCommand, "ls -a DFS%d/%s > DFS%d/%s/.%s", portIndex, USERDATA.USERNAME, portIndex, USERDATA.USERNAME, filename);
			printf("listCommand %s\n", listCommand);
			system(listCommand);
			printf("\n\nEmpty subDirectory received\n\n");
		}
		else{
			sprintf(listCommand, "ls -a DFS%d/%s/%s > DFS%d/%s/%s/.%s", portIndex, USERDATA.USERNAME, subDirectory, portIndex, USERDATA.USERNAME, subDirectory, filename);
			printf("listCommand %s\n", listCommand);
			system(listCommand);
		}
	}
	
	//fetch names of  4 folders
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
  
	//check for sub directories
	if(strcmp(subDirectory,"/") == 0)
		printf("Sub folder is empty\n");
    
	else
		sprintf(dfsUserFolder, "%s/%s", dfsUserFolder, subDirectory);

	printf("dfsUserFolder: %s\n", dfsUserFolder);

  
	bzero(fn, sizeof(fn));
	#if 0
		strcpy(fn, filename);
	#else
		sprintf(fn,".%s",filename);
	#endif

	if(fileNumber != -1){
		directory = opendir(dfsUserFolder);
 
		if(directory){
			while((dir = readdir(directory)) != NULL){
				if(strncmp(dir->d_name, fn, strlen(filename)) == 0){
					if((dir->d_name[strlen(filename)+2] >= 0) &&  (dir->d_name[strlen(filename)+2] <= 3)){
						strcpy(name[count], dir->d_name);
						printf("name[count]: %s\n", name[count]);
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
  
	bzero(fndot,sizeof(fndot));
	#if 0
		strcpy(fndot, name[fileNumber]);
	#else
		if (fileNumber != -1)
		strcat(fndot, name[fileNumber]);
		else{
			sprintf(fndot, ".%s", filename);
			if(strcmp(subDirectory, "/") == 0){
				
			}
			else
				sprintf(dfsMainFolder, "%s%s/", dfsMainFolder, subDirectory);
		}
	#endif

	
	//send filename to client
	printf("Step: Sending filename from server to client\n");  
	n = send(acceptSock, fndot, sizeof(fndot), 0);
	if(n < 0)
		perror("Error sending the filename to client\n");
  
	strncat(dfsMainFolder, fndot, sizeof(fndot));
	printf("dfsMainFolder: %s\n", dfsMainFolder);
	bzero(fileExistsCheck, sizeof(fileExistsCheck));

	if(status == 0)
		perror("FILE NOT FOUND\n");
	else if(fileNumber >= -1){
		if(strcmp(subDirectory, "/") == 0)  
			sprintf(fileExistsCheck, "DFS%d/%s/%s", portIndex, USERDATA.USERNAME,fndot);
		else
			sprintf(fileExistsCheck, "DFS%d/%s/%s/%s", portIndex, USERDATA.USERNAME,subDirectory, fndot);

		if(access(fileExistsCheck, F_OK) != -1){
			printf("FILE EXISTS\n");
			status = 1;
			n = send(acceptSock, (void *)&status, sizeof(int), 0);
			if(n < 0){
				perror("Error sending fSize");
				exit(1);
			}
		}
		else{
			status = 0;
			printf("FILE NOT FOUND\n");
			n = send(acceptSock, (void *)&status, sizeof(int), 0);
			if(n < 0){
				perror("Error sending fSize");
				exit(1);
			}
		}
	}
	else{
		status = -1;
        n = send(acceptSock, (void *)&status, sizeof(int), 0);
        if(n < 0){
			perror("Error sending fSize");
			exit(1);
        }
	}

	if(status == 1 || status == -1){
		if(!(fp = fopen(dfsMainFolder, "r"))) 
			perror("Error in opening the file\n");    
		
		bzero(dfsMainFolder, sizeof(dfsMainFolder));
		fseek(fp, 0, SEEK_END);
		fSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		printf("The file size is: %d\n", fSize);
		
		printf("Step: Sending file from Server to Client\n");
		n = send(acceptSock, (void *)&fSize, sizeof(int), 0);
        if(n < 0){
			perror("Error sending fSize");
			exit(1);
		}
		
		while(!feof(fp)){
			readSize = fread(sendBuffer, 1, sizeof(sendBuffer)-1, fp);
			do{
                recvBytes = send(acceptSock, sendBuffer, readSize, 0);
			}while (recvBytes < 0);
			bzero(sendBuffer, sizeof(sendBuffer));
        }
	}
}






//------------------------------RECEIVE FILE FROM CLIENT---------------------------
int receiveFile(int sockfd, struct sockaddr_in clientAddress, socklen_t clientSize, int portIndex){
	int fSize = 1;
	int recvSize = 0;
	int recvBytes = 0;
	int writtenBytes;

	char fname[100];
	char subDirectory[100];
	char fnameReceived[100];
	char dfsMainFolder[100];
	char dfsUserFolder[150];
	char dfsUsernameAndSubFolder[200];
	char fndot[100];
	char filenamePut[50];
	char filenameGet[50];
	char *receiveBuffer;
	receiveBuffer = malloc(300241);

	FILE *fp;

	//receive filesize
	printf("Step: Receiving file size\n");
	recvBytes = recv(sockfd, &fSize, sizeof(int), 0);
	if(recvBytes < 0)
		perror("Error in receiving the file size\n");
	printf("file size: %d\n", fSize);

	//receive filename
	printf("Step: Receiving file name\n");
	recvBytes = recv(sockfd, fname, 100, 0);
	if(recvBytes < 0)
		perror("Error in receiving the file name\n");
	printf("file name: %s\n", fname);

	//receive subdirectory
	printf("Step: Receiving subDirectory\n");
	recvBytes = recv(sockfd, subDirectory, 100, 0);
	if (recvBytes < 0)
		perror("Error in receiving the subDirectory\n");
	printf("subDirectory: %s\n", subDirectory);
	
	bzero(fnameReceived, sizeof(fnameReceived));
	strncat(fnameReceived, fname, 100);
	strncat(fnameReceived, "_recv", 100);
	bzero(dfsMainFolder, sizeof(dfsMainFolder));
	
	//create 4 main directories
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
	
	//create username directories
	printf("Step: Creating USERNAME directory\n");
	strncat(dfsUserFolder, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
	system(dfsUserFolder);
	
	//create subdirectory
	printf("Step: Creating USERNAME/subdirectory\n");
	if(strcmp(subDirectory, "/") == 0)
		printf("Empty subDirectory\n");
	else{
		sprintf(dfsUsernameAndSubFolder, "mkdir -p %s/%s", dfsUserFolder, subDirectory);
		system(dfsUsernameAndSubFolder);
	}
  
	strncat(dfsMainFolder, "/", sizeof("/"));
	strncat(dfsMainFolder, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
  
	bzero(fndot,sizeof(fndot));
	#if 0
		strcpy(fndot, fname);
	#else
		strncpy(fndot, ".", sizeof("."));
		strcat(fndot, fname);
	#endif

	if(strcmp(subDirectory, "/") == 0)
		printf("subDirectory is empty\n");
	else
		sprintf(dfsMainFolder,"%s/%s",dfsMainFolder,subDirectory);
    
	strncat(dfsMainFolder, "/", sizeof("/"));
	strncat(dfsMainFolder, fndot, sizeof(fndot));
	printf("DFS Main Folder: %s\n", dfsMainFolder);
	
	fp = fopen(dfsMainFolder, "w");
	if(fp == NULL){
		perror("Error in opening the file\n");
		return -1;
	}

	printf("Step: Receiving file chunks\n");
	while(recvSize < fSize) {
    recvBytes = recv(sockfd, receiveBuffer, 300241, 0);
    writtenBytes = fwrite(receiveBuffer, 1, recvBytes, fp);
    recvSize = recvSize + recvBytes;
    printf("Total filesize received so far: %d\n", recvSize);
	}

	fclose(fp);
	bzero(dfsUserFolder, sizeof(dfsUserFolder));
	return 1;
}




//---------------------------RECEIVE SUB DIRECTORY------------------------------
int receiveSubDirectory(int sockfd, struct sockaddr_in clientAddress, socklen_t clientSize, int portIndex){
	int recvBytes = 0;
	char subDirectory[100];
	char dfsMainFolder[100];
	char dfsUserFolder[150];
	char dfsUsernameAndSubFolder[200];

	printf("Step: Receiving subDirectory\n");
	recvBytes = recv(sockfd, subDirectory, 100, 0);
	if (recvBytes < 0)
		perror("Error in receiving the subDirectory\n");
	printf("subDirectory: %s\n", subDirectory);
	
	//create 4 main directories
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
	
	//create username directory
	printf("Step: Creating USERNAME directory\n");
	strncat(dfsUserFolder, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
	system(dfsUserFolder);
	
	//create sub folder
	printf("Step: Creating USERNAME/subdirectory\n");
	if(strcmp(subDirectory, "/") == 0)
		printf("Empty subDirectory\n");
	else{
		sprintf(dfsUsernameAndSubFolder, "mkdir -p %s/%s", dfsUserFolder, subDirectory);
		system(dfsUsernameAndSubFolder);
	}
  
	strncat(dfsMainFolder, "/", sizeof("/"));
	strncat(dfsMainFolder, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
	return 1;
}







//----------------------------------MAIN------------------------------------
//Ref: https://stackoverflow.com/questions/30356043/finding-ip-address-of-client-and-server-in-socket-programming
int main(int argc, char **argv){
	signal(SIGPIPE, SIG_IGN);
  
	int sockfd;
	int acceptSock;
	int n;
	int option;
	int recvBytes;
	int portIndex;
	int start;
	int dummy;
  
	char buffer[256];
	char filenameGet[15];
	
	struct sockaddr_in serverAddress, clientAddress;
	socklen_t clientSize;
	char fileNameList[10];
  
	if(argc < 3){
		fprintf(stderr, "port number or config file is missing in arguments\n");
		exit(1);
	}
	
	printf("Step: Getting index from port number\n");
	PORT = atoi(argv[1]);
	portIndex = PORT%5;
	printf("PortIndex: %d\n", portIndex);
	
	printf("Step: Verifying dfs.conf file\n");
	sprintf(dfsConfigFilename,"%s",argv[2]);
	FILE *fp;
	fp=fopen(dfsConfigFilename,"r");
	if (fp == NULL){
		perror(dfsConfigFilename);
		exit(1);
	}
	fclose(fp);
  
	//create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		perror("ERROR opening socket");
	bzero((char *) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(PORT);
	
	if(bind(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) 
		perror("ERROR on binding");
    
	printf("Step: Server started\n");
	listen(sockfd,5);
	clientSize = sizeof(clientAddress);
	
	while(1){
		acceptSock = accept(sockfd,(struct sockaddr *) &clientAddress, &clientSize);
		if(acceptSock < 0)
			perror("ERROR on accept");
		
		else{
			//for multiple connections
			if(fork() == 0){
			for(;;){
				n = recv(acceptSock, (void *)&option, sizeof(int), 0);
				if (n < 0)
					perror("Error in receiving option\n");
			
				//--------------------------CHOICE == GET-----------------------------
				if(option == GET){
					n = recv(acceptSock, (void *)&dummy, sizeof(int), 0);
					if(n < 0)
						perror("error in receiving option\n");
					  
					n = recv(acceptSock, (void *)&start, sizeof(int), 0);
					if(n < 0)
						perror("option receiving the start status");
					  
					if(start){
						printf("Step: Receiving filename\n");
						if(validateUserDetails(acceptSock)){
							recvBytes = recv(acceptSock, filenameGet, 50,0);
							if(recvBytes < 0){
							  perror("Error sending file name\n");
							  exit(1);
							}
							
							printf("Filename to send: %s\n", filenameGet);
							sendFile(acceptSock, filenameGet, clientAddress, portIndex, 0);  
							sleep(1);
							
							recvBytes = recv(acceptSock, filenameGet, 50,0);
							if (recvBytes < 0){
							  perror("Error sending file name\n");
							  exit(1);
							}
							
							printf("Filename to send: %s\n", filenameGet);
							sendFile(acceptSock, filenameGet, clientAddress, portIndex, 1);  
							printf("DONE WITH GET\n");
						}
					}	
				}
			
				//------------------------CHOICE == PUT----------------------------------
				else if(option == PUT){
					printf("Step: PUT\n");
					if(validateUserDetails(acceptSock)){ 
					  //receive first chunk of file
					  receiveFile(acceptSock, clientAddress, clientSize, portIndex); 
					  printf("Step: completed put for file chunk 1\n");

					  //receive second chunk of file
					  receiveFile(acceptSock, clientAddress, clientSize, portIndex); 
					  printf("Step: completed put for file chunk 2\n");
					  printf("DONE WITH PUT\n");
					}
				}
				
				//------------------------CHOICE == LIST-----------------------------
				else if(option == LIST){
					if(validateUserDetails(acceptSock)){
						recvBytes = recv(acceptSock, fileNameList, 50,0);
						if(recvBytes < 0){
							perror("Error sending list file");
							exit(1);
						} 
						sendFile(acceptSock, fileNameList, clientAddress, portIndex, -1); 
						sleep(1);
					}
					printf("DONE WITH LIST\n");
				}
				
				//--------------------------CHOICE == MKDIR--------------------------
				else if(option == MKDIR){
					printf("Step: MKDIR\n");
					if(validateUserDetails(acceptSock)){
						receiveSubDirectory(acceptSock, clientAddress, clientSize, portIndex); 
					}
					printf("DONE WITH MKDIR\n");
				}
				
				//--------------------------CHOICE == EXIT---------------------------
				else if(option == EXIT){
					printf("GOODBYE!\n");
					for(int j=0; j<MAX_CONN; j++)
						close(sockfd);
					exit(0);
				}
				
				else{
					printf("GOODBYE!\n");
					for(int j=0; j<MAX_CONN; j++)
						close(sockfd);
					exit(0);
				}
			}//end of for
			}//if(fork())
		}//else of if(acceptSock)	
	}//while(1)
}
