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
#include <stdlib.h>

#define GET_FILE 1  
#define PUT_FILE 2
#define LIST_FILES 3
#define CLOSE_SOC 4

#define TRUE 1
#define FALSE 0

#define PASS 1
#define FAIL 0

#define MAX_CONN 4
#define TIMEOUT 1

int PORT;
char SERVERS[4][100];
char USERNAME[100];
char PASSWORD[100];
char dfcConfigFile[100];

struct sockaddr_in  serverAddress[MAX_CONN];
socklen_t serverLength[MAX_CONN];
int sockfd[MAX_CONN];
struct hostent *SERVER;


//----------------------------SHOW USER MENU---------------------------------
void displayMenu(){
	printf("\n----------------------------------------------\n");
	printf("list: get a list of files on the server\n");
	printf("put: upload a file to the server\n");
	printf("get: get a file from the server\n");
	printf("exit: close the connection\n");
	printf("----------------------------------------------\n\n");
}


//----------------------------GET USER CHOICE--------------------------------
char getUserChoice(){
	char command[6];
	char choice;
	displayMenu();
	
	while(1){
		printf("Please enter a command: ");
		scanf("%s", command);

		if(strcmp(command, "get") == 0){
			choice = GET_FILE;
			printf("\nCommand entered: %s", command);
			break;
		}
		else if(strcmp(command, "put") == 0){
			choice = PUT_FILE;
			printf("\nCommand entered: %s", command);
			break;
		}
		else if(strcmp(command, "ls") == 0){
			choice = LIST_FILES;
			printf("\nCommand entered: %s", command);
			break;
		}
		else if(strcmp(command, "exit") == 0){
			choice = CLOSE_SOC;
			printf("\nCommand entered: %s", command);
			break;
		}
		else{
			printf("\nInvalid command!\n");
			displayMenu();
		}
	}
	return choice;
}




//----------------------------GET FILE SIZE----------------------------------
static unsigned int getFileSize(FILE * fp){
    unsigned int fileSize;
    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    return fileSize;
}

 

//-------------------------READ DFC CONFIG FILE------------------------------
void readConfFile(int lineLimit){
    int i = 0;
	FILE *fp;
    char readBuffer[200];
    char *value;

    fp = fopen("dfc.conf", "r");
    if(fp == NULL){
        perror("Error in opening dfc.conf file\n");
        exit(1);
    }

    else{
        unsigned int dfcConfigFileSize = getFileSize (fp);
        while(fgets(readBuffer, dfcConfigFileSize, fp) != NULL){
            if (lineLimit){
	            if((strncmp(readBuffer,"Server",6)==0)  || (strncmp(readBuffer,"SERVER",6)==0)){
	                value = strtok(readBuffer, " \t\n");
	                value = strtok(NULL, " \t\n");

	                if (value[3] == '1'){
	                	value = strtok(NULL, " \t\n");
	                	strcpy(SERVERS[0], value);
	                	i = 0;
	                }
	                if (value[3] == '2'){
	                	value = strtok(NULL, " \t\n");
	                	strcpy(SERVERS[1], value);
	                	i = 1;
	                }
	                if (value[3] == '3'){
	                	value = strtok(NULL, " \t\n");
	                	strcpy(SERVERS[2], value);
	                	i = 2;
	                }
	                if (value[3] == '4'){
	                	value = strtok(NULL, " \t\n");
	                	strcpy(SERVERS[3], value);
	                	i = 3;
	                }
	                printf("SERVERS[%d]: %s\n", i, SERVERS[i]);
	                bzero(readBuffer, sizeof(readBuffer));
	                i = i % 4;
	            }
        	}
        	else
        	{
	            //Fetch USERNAME
	            if(strncmp(readBuffer, "Username", 8) == 0){
	                value = strtok(readBuffer, " \t\n");
	                value = strtok(NULL, " \t\n");
	                strcpy(USERNAME, value);
	                printf("USERNAME: %s\n", USERNAME);
	            	bzero(readBuffer, sizeof(readBuffer));
	            }

	            //Fetch PASSWORD
	            if(strncmp(readBuffer, "Password", 8) == 0){
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
int sendUserCredentials(int sockfd1){
	int ack;
	int n;
    readConfFile(0);
	
	printf("USERNAME: %s\n", USERNAME);
	printf("PASSWORD: %s\n", PASSWORD);

	n = send(sockfd1, USERNAME, 100, 0);
	if(n < 0)
		perror("choice sending failed");

	n = send(sockfd1, PASSWORD, 100, 0);
	if(n < 0)
		perror("choice sending failed");

	n = recv(sockfd1, &ack, sizeof(int), 0);
    if(n < 0)
      perror("ack receiving failed");
      
    if(!ack){
    	printf("INVALID USERNAME OR PASSWORD\n");
    	return FAIL;
    }
    return PASS;
}





//----------------------------SOCKET CREATION-------------------------------
//Ref: https://en.wikibooks.org/wiki/C_Programming/Networking_in_UNIX
//Ref: https://courses.cs.washington.edu/courses/cse476/02wi/labs/lab1/client.c
//Ref: https://stackoverflow.com/questions/4181784/how-to-set-socket-timeout-in-c-when-making-multiple-connections
void createSockets(){
    char *serverName;
    char *port;
	int i = 0;

    for(i=0; i<MAX_CONN; i++){
    	serverLength[i] = sizeof(serverAddress[i]);
    	serverName = strtok(SERVERS[i], ":");
		port = strtok(NULL,"");
		PORT = atoi(port);

		//create socket
		if((sockfd[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			perror("Error creating the socket\n");

		//set up the address structure
		SERVER = gethostbyname(serverName);
		memset((char *)&serverAddress[i], 0, sizeof(serverAddress[i]));
		serverAddress[i].sin_family = AF_INET;
		serverAddress[i].sin_port = htons(PORT);
		bcopy((char *)SERVER->h_addr, (char*)&serverAddress[i].sin_addr.s_addr, SERVER->h_length);
	     
		//connect to server
	    if(connect(sockfd[i],(struct sockaddr *)&serverAddress[i], sizeof(serverAddress[i])) < 0) 
	    	perror("Error in connecting to the socket\n");
		
		struct timeval timeout;      
	    timeout.tv_sec = TIMEOUT;
	    timeout.tv_usec = 0;

	    if(setsockopt(sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0)
	        perror("Error in setsockopt()\n");
    }
}



//----------------------------CALCULATE MD5SUM-------------------------------
void calculateMd5Sum(char *filenamePut, char md5sum[100]){
    char MD5Command[100];
	FILE *fp;
    strncpy(MD5Command, "md5sum ", sizeof("md5sum ")); 
    strncat(MD5Command, filenamePut, strlen(filenamePut));
    fp = popen(MD5Command, "r");
    while(fgets(md5sum, 100, fp) != NULL)
		strtok(md5sum,"  \t\n");
    pclose(fp);
}





//--------------------------SEND FILES TO SERVERS----------------------------
int sendFile(int sockfd, char *fname, struct sockaddr_in serverAddress, char *subDirectory){
	FILE *fp;
	int fSize;
	size_t readSize;
	size_t sendBytes;
	char sendBuffer[1024];
	struct timeval timeout = {2,0};

	if(!(fp = fopen(fname, "r")))
		perror("Error opening the file\n");
	
	printf("Step: Calculating file size\n");
	fseek(fp, 0, SEEK_END);
	fSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	printf("Total file size: %d\n", fSize);
	
	printf("Step: sending file size to server\n");
	sendBytes = send(sockfd, &fSize, sizeof(int), 0);
	if(sendBytes < 0){
		perror("Error in sending the file size\n");
		exit(1);
	}
	
	printf("Step: sending filename\n");
	sendBytes = send(sockfd, fname, 100, 0);
	if(sendBytes < 0){
		perror("Error in sending the file name\n");
		exit(1);
	}
	
	printf("Step: sending subDirectory\n");
	sendBytes = send(sockfd, subDirectory, 100, 0);
	if(sendBytes < 0){
		perror("Error in sending the subDirectory\n");
		exit(1);
	}
	
	printf("Step: File transmission in progress\n");
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





//--------------------------------RECEIVE FILE FROM SERVER-----------------------
int receiveFile(int sockfd, char *filename, struct sockaddr_in serverAddress, socklen_t clientlen, char *subDirectory){
	int receivedSize = 0;
	int recvBytes = 0;
	int writtenSize = 0;
	int stat = 0;
	int fileSize = 1;
	int status;
	char receivedFilename[100];
	char *readBuffer;
	readBuffer = malloc(300241);
	FILE *fp;
  
	bzero(receivedFilename, sizeof(receivedFilename));
	printf("Step: Sending subDirectory name");
	stat = send(sockfd, subDirectory, 100, 0);
	if(stat < 0)
		perror("Error in sending subDirectory\n");
	
	printf("Step: Receiving filename\n");
	stat = recv(sockfd, receivedFilename, sizeof(receivedFilename), 0);
	if(stat < 0)
		perror("Error in receiving filename\n");
  
	printf("Step: Receiving file status\n");
	stat = recv(sockfd, &status, sizeof(int), 0);
	if(stat < 0)
		perror("Error in receiving file status\n");
	
	if(status == 0)
		printf("FILE DOES NOT EXIST ON SERVER\n!");
  
	if(status == 1 || status == -1){
		printf("Step: receiving file size\n");
		stat = recv(sockfd, &fileSize, sizeof(int), 0);
		if(stat < 0)
			perror("Error in receiving file size\n");
		printf("fileSize: %d\n", fileSize);
		
		strncat(receivedFilename, "_received", 100);
		fp = fopen(receivedFilename, "w");
		if(fp == NULL){
			printf("Error has occurred. Image file could not be opened/ created\n");
			return -1; 
		}

		//start receiving the file
		printf("Step: start receiving the file packets\n");
		while(receivedSize < fileSize) {
			recvBytes = recv(sockfd, readBuffer, 300241, 0);
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





//------------------------------------------MAIN------------------------------------
//Ref: To split the file into equal parts - http://www.theunixschool.com/2012/10/10-examples-of-split-command-in-unix.html
//Ref: To do AES encryption - https://askubuntu.com/questions/60712/how-do-i-quickly-encrypt-a-file-with-aes
int main(int argc, char **argv){
	int md5Int;
	int md5Index;
	int finalIndex;
	int option = 0;
	int dummy = 0;
	int i, n;
	int arrayOfFailedSend[MAX_CONN];
	
	char md5sum[100];
	char filenameGet[20];
	char filenamePut[20];	
	char subDirectory[50];
	char fileNameList[10];
	
	FILE *fp;
	FILE *filepointer;
	
	if(argc < 2){
     	fprintf(stderr, "dfc.conf file is missing from the arguments\n");
     	exit(1);
  	}
  	
	sprintf(dfcConfigFile, "%s", argv[1]);
  	fp = fopen(dfcConfigFile, "r");
    if(fp == NULL){
        perror("Error in opening config file\n");
        exit(1);
    }
  	fclose(fp);

	signal(SIGPIPE, SIG_IGN);
	printf("Step: Reading Config file for Server details\n");
	readConfFile(1);
	printf("Step: Creating sockets\n");
	createSockets();
	
	while(1){
		option = getUserChoice();
		#if 1
		for (i=0;i<MAX_CONN;i++){
			int n = send(sockfd[i], (void *)&option, sizeof(int), 0);	
			if (n < 0){
				arrayOfFailedSend[i] = TRUE;
				perror("Writing to socket: option sending failed");
			}
			else{
				arrayOfFailedSend[i] = FALSE;
			}
		}
		#else
			int n = send(sockfd[0], (void *)&option, sizeof(int), 0);	
			if (n < 0)
				perror("Writing to socket: option sending failed");
		#endif
		
		int dummy = 100;
		switch(option)
		{
			//------------------------CHOICE == GET---------------------------------
			case GET_FILE:
				for(i=0; i<MAX_CONN; i++){
					int n = send(sockfd[i], (void *)&dummy, sizeof(int), 0);	
					if(n < 0){
						arrayOfFailedSend[i] = TRUE;
						perror("Error in sending User Choice\n");
					}
					else
						arrayOfFailedSend[i] = FALSE;
				}
				
				printf("Enter the filename and sub directory to receive:\n");
				scanf("%s", filenameGet);
				scanf("%s", subDirectory);
			
				printf("Step: Checking for serversForUse[] for optimized get\n");
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
			    			//goto end_of_get; 
			    		}
			    	}
			    }
			    else if(!arrayOfFailedSend[1]){
			    	serverForUse[1] = TRUE;
			    	if (!arrayOfFailedSend[3]){
			    		printf("COMPLETE 2\n");
			    		serverForUse[3] = TRUE;
			    	}
			    	else{
			    		printf("INCOMPLETE 2\n");
			    		//goto end_of_get; 
			    	}
			    }
			    else{
			    	printf("INCOMPLETE 3\n");
			    	//goto end_of_get;
			    }
				
			    for(i=0; i<MAX_CONN; i++)
					printf("serverForUse[%d] = %d\n", i, serverForUse[i]);
				
				printf("Step: Pre-informing server the status to start sending file\n");
			    int start;
			    for(i=0; i<MAX_CONN; i++){
			    	if(serverForUse[i] == TRUE){
			    		start = TRUE;
		    			n = send(sockfd[i], (void *)&start, sizeof(int), 0);
						if(n < 0)
							perror("Error in send\n");
					}
			    	else{
			    		start = FALSE;
		    			n = send(sockfd[i], (void *)&start, sizeof(int), 0);
						if(n < 0)
							perror("option sending failed");
					}
				}
				
				
				printf("Step: Sending filenameGet to server\n");
				#if 1
			    for(i=0; i<MAX_CONN; i++)
				#else
			    	for(i=0; i<1; i++)
				#endif
				{
			    	if(serverForUse[i] == TRUE){
			    		if(sendUserCredentials(sockfd[i])){
			    			//send filename 1
					    	n = send(sockfd[i], filenameGet, 50, 0);
							if(n < 0)
								perror("Error in sending filename for get\n");
							
							receiveFile(sockfd[i], filenameGet, serverAddress[i], serverLength[i], subDirectory);
							
							//sleep(1);
							
							//send filename 2
							n = send(sockfd[i], filenameGet, 50, 0);
							if(n < 0)
								perror("Error in sending filename for get\n");
							receiveFile(sockfd[i], filenameGet, serverAddress[i], serverLength[i], subDirectory);
						}
					}
				}
				
				for(i=0; i<MAX_CONN; i++)
					serverForUse[i] = -1;

			    char fileLs[100];
    			char systemListGetFile[100];
    			char decryptCommand[300];
    			char fileList[4][100];
				int i = 0;
				
				bzero(systemListGetFile, sizeof(systemListGetFile));
			    //This will do: ls -a
				strncpy(systemListGetFile, "ls -a .", strlen("ls -a ."));
				 //This will do: ls [filename]
			    strncat(systemListGetFile, filenameGet, strlen(filenameGet));
			    strncat(systemListGetFile, "*_rec*", strlen("*_rec*"));
			    
			    FILE *filepointer = popen(systemListGetFile, "r");
			    while(fgets(fileLs, 100, filepointer) != NULL){
			    	bzero(fileList[i], sizeof(fileList[i]));
					strtok(fileLs,"  \t\n");
					strncpy(fileList[i], fileLs, sizeof(fileLs));
			        i++;
			    }
			    pclose(filepointer);
				
			    
			    bzero(decryptCommand, sizeof(decryptCommand));
			    readConfFile(0);
			    if(i < MAX_CONN)
			    	printf("FILE INCOMPLETE\n");
			    else{
					printf("Step: decrypting\n");
			    	char catFilePiecesCommand[300];
			    	for(i=0; i<MAX_CONN; i++){
						sprintf(decryptCommand, "openssl enc -d -aes-256-cbc -in %s -out de%s -k %s", fileList[i], fileList[i], PASSWORD);
						system(decryptCommand);
					}
			    		
					printf("Step: Concateneting all the files\n");
			    	sprintf(catFilePiecesCommand, "cat de%s de%s de%s de%s > %s_received", fileList[0],fileList[1],fileList[2],fileList[3], filenameGet);
			    	system(catFilePiecesCommand);
			    	bzero(catFilePiecesCommand, sizeof(catFilePiecesCommand));
			    }

			    bzero(decryptCommand, sizeof(decryptCommand));
				for (i=0; i<MAX_CONN; i++)
					bzero(fileList[i],sizeof(fileList[i]));
				
				//end_of_get:
				printf("DONE WITH GET\n");
				break;


				
			//------------------------CHOICE == PUT----------------------------------
			case PUT_FILE:
				printf("Step: PUT\n");
				printf("Enter the name of the file and subDirectory: ");
				scanf("%s", filenamePut);
				scanf("%s", subDirectory);
				
				while(1){
			    	if(!(filepointer = fopen(filenamePut, "r"))){
			   			perror("Error in opening the file");
						printf("List of files:\n");
			    		system("ls");
			    		printf("Enter the file name again\n");
			    		scanf("%s", filenamePut);
			    	}
			    	else
			    		break;
			    }
			   	
			    //Calculate MD5 sum
				printf("Step: Calculating MD5sum\n");
			    calculateMd5Sum(filenamePut, md5sum);
				md5Int = md5sum[strlen(md5sum)-1] % 4;
				md5Index = (4-md5Int)%4;
				printf("md5Index %d\n", md5Index);
				
			    //Divide the file using split command
				printf("Step: Divide the file into four parts\n");
			    char systemCommand[150];
			    char filename[100];
			    bzero(filename, sizeof(filename));
			    strncpy(filename,filenamePut,sizeof(filenamePut));
			    strncpy(filename, filenamePut,strlen(filenamePut));
			    sprintf(systemCommand,"split -n 4 -a 1 -d %s fragment_%s",filenamePut, filenamePut);
			    system(systemCommand);

			    //Encryption using AES
				printf("Step: Encrypting file using AES\n");
			    char encryptyCommand[200];
			    bzero(encryptyCommand,sizeof(encryptyCommand));
			    readConfFile(0);
			    for (i=0; i<MAX_CONN; i++){
			    	sprintf(encryptyCommand,"openssl enc -aes-256-cbc -in fragment_%s%d -out %s%d -k %s", filenamePut, i, filenamePut, i, PASSWORD);
			    	system(encryptyCommand);
			  	}

			    char fnameWithIndex[4][100];
			    char fnIndex[1];
			    #if 1
			    	for (i=0; i<MAX_CONN; i++)
			    #else
			    	for (i=0; i<1; i++)
			    #endif
			    {
			    	if (!arrayOfFailedSend[i])
			    	{
				    	if (sendUserCredentials(sockfd[i]))
				    	{
					    	//first piece of file
							printf("Step: Calculating index for first piece of file\n");
					    	finalIndex = (i+md5Index)%4;
					    	strncpy(fnameWithIndex[finalIndex], filenamePut, sizeof(filenamePut));
					    	sprintf(fnIndex,"%d",finalIndex);
					    	printf("fnIndex  :   %s\n", fnIndex);
					    	strncat(fnameWithIndex[finalIndex], fnIndex, 1);
					    	printf("filename %s\n", fnameWithIndex[finalIndex]);
							sendFile(sockfd[i], fnameWithIndex[finalIndex], serverAddress[i], subDirectory);
							
							sleep(1);
							
							//second piece of file
							printf("Step: Calculating index for second piece of file\n");
							strncpy(fnameWithIndex[(finalIndex+1)%4], filenamePut, sizeof(filenamePut));
					    	sprintf(fnIndex,"%d",(finalIndex+1)%4);
					    	printf("fnIndex  :   %s\n", fnIndex);
					    	strncat(fnameWithIndex[(finalIndex+1)%4], fnIndex, 1);
					    	printf("filename %s\n", fnameWithIndex[(finalIndex+1)%4]);
							sendFile(sockfd[i], fnameWithIndex[(finalIndex+1)%4], serverAddress[i], subDirectory);
							
							//bzero everything
							bzero(fnameWithIndex[finalIndex], sizeof(fnameWithIndex[finalIndex]));
							bzero(fnameWithIndex[(finalIndex+1)%4], sizeof(fnameWithIndex[(finalIndex+1)%4]));
							bzero(fnIndex, sizeof(fnIndex));
						}
					}
				}
				printf("DONE WITH PUT\n");
				break;

			// This command gets the list of files form the server in its current directory
			case LIST_FILES:
				system("rm .list0_received .list1_received .list2_received .list3_received");
				printf("Enter the subDirectory\n");
				scanf("%s", subDirectory);
				
				int status =0;
				int checkNoCnnections = 0; 
				// this part should be uncommented
				for (i =0; i<MAX_CONN;i++)
				{
					sprintf(fileNameList, "list%d", i);
					//printf("fileNameList %s\n", fileNameList);
					if (!arrayOfFailedSend[i])
					{ 
						if (sendUserCredentials(sockfd[i]))
			    		{
						
							n = send(sockfd[i], fileNameList, 50, 0);
							if (n < 0)//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
							{
								perror("option sending failed");
							}
							//exit(1);
							status = receiveFile(sockfd[i], fileNameList, serverAddress[i], serverLength[i], subDirectory);
							if (status == 0)
							{
								checkNoCnnections++;
							}
							//printf("\n\nLOOPPP\n");
						}
						//	exit(1);
					}
					bzero(fileNameList, sizeof(fileNameList));
				}

				if (checkNoCnnections == 3)
				{
					printf("ALL SERVERS CLOSED\n");
				}
		        // creating files if not present
		        
		        system("touch .list0_received");
		        system("touch .list1_received");
		        system("touch .list2_received");
		        system("touch .list3_received");
		        // concat the files present
		        system("cat .list0_received .list1_received .list2_received .list3_received > temp_list");
		        system("sort temp_list | uniq > final_list");
		      
		        FILE *fp;
		        char wsBuf[200];
		        char *val1;
		        char array[256][256];
		        char array_withoutIndex[256][256];
		        char necessary_files[256][256];

		        int p = 0, q = 0;

		        FILE *fr;
				fr = fopen("final_list","r");
                if (fr != NULL)
                {
                    char line_string[500]; /* Buffer to store the contents of each line */
                    int i = 0;
                    char filename_string[500];
                    char filename_string1[500];
                    int subfile_count = 0;
                    while(fgets(line_string, sizeof(line_string), fr) != NULL)
                    {
                        if(strncmp(line_string,".",strlen(".")) == 0)
                        {
                            if(strlen(line_string)>3)
                            {
                                char *line_ptr; /* Pointer to the start of each line */
                                line_ptr = strstr((char *)line_string,".");
                                line_ptr = line_ptr + strlen(".");
                                if (subfile_count == 0)
                                {
                                    bzero(filename_string,sizeof(filename_string));
                                    memcpy(filename_string,line_ptr,strlen(line_ptr)-2);
                                }
                                bzero(filename_string1,sizeof(filename_string1));
                                memcpy(filename_string1,line_ptr,strlen(line_ptr)-2);
                                if(strcmp(filename_string,filename_string1) == 0)
                                {
                                    subfile_count = subfile_count +1;
                                    if(subfile_count == 4)
                                        printf("%s [COMPLETE]\n",filename_string);
                                }
                                else
                                {
                                    if(subfile_count != 4)
                                        printf("%s [INCOMPLETE]\n",filename_string);
                                    subfile_count = 1;
                                    strcpy(filename_string,filename_string1);

                                }
                            }
                        }
                    }
                    if(subfile_count != 4)
                        printf("%s [incomplete]\n",filename_string);
                }
                fclose(fr);

				
			break;

			default:
			break;

		}
	}


	//close(sockfd);
	return 0;
}
