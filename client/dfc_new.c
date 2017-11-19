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

#define SUCCESS 1
#define FAIL 0

#define TIMEOUT 1

char SERVERS[4][100];
char USERNAME[100];
char PASSWORD[100];
char dfcConfigFile[100];

int PORT;

#define MAX_CONN 4
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
	                i = i%4;
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
int sendUserDetails(int sockfd1){
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
    return SUCCESS;
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






int receive_image(int sockfd, char *filename, struct sockaddr_in serverAddress, socklen_t clientlen, char *subDirectory)
{
	
}



int main(int argc, char **argv)
{
	
	//exit(1);
	if (argc < 2) {
     	fprintf(stderr,"configuration file absent\n");
     	exit(1);
  	}
  	sprintf(dfcConfigFile,"%s",argv[1]);
  	FILE *fp;
  	fp=fopen(dfcConfigFile,"r");
    if (fp == NULL)
    {

        perror(dfcConfigFile);
        exit(1);
    }
  	fclose(fp);
  	printf("%s\n", dfcConfigFile);

  	//exit(1);
	signal(SIGPIPE, SIG_IGN);
	char md5sum[100];
	int md5sumInt;
	int sendFailedArray[MAX_CONN];
	int listCheck[MAX_CONN] = {0, 0, 0, 0};
	int dummy = 0;
	// This function reads server info and user credentials
	readConfFile(1);
	// This function create four sockets reading the address from the dsc.conf
	createSockets();
	//char *putFileName1;
	char *subFolder;
	int i;
	int option = 0;      
	char getFileName[20];
	char putFileName[20];//, putFileName1[50];
	int n;
	FILE *picture;
	int j;
	int ack_putfile;
	int md5sumIndex;
	int finalIndex;
	char *val1;
	char subfolder[50];
	char fileNameList[10];
	while(1)
	{
		//
		option = getUserChoice(); // selecting the command received
		printf("option entered is %d\n", option);

		printf("Option Sent !!\n");

#if 1
		for (i=0;i<MAX_CONN;i++)
		{
			printf("**Option Sent !!\n");
			int n = send(sockfd[i], (void *)&option, sizeof(int), 0);	
			printf(" ....%d\n", n);
			if (n < 0)//, 0, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
			{
				sendFailedArray[i] = TRUE;
				perror("Writing to socket: option sending failed");
			}
			else
			{
				sendFailedArray[i] = FALSE;
			}
		}
#else
			int n = send(sockfd[0], (void *)&option, sizeof(int), 0);	
			printf(" ....%d\n", n);
			if (n < 0)//, 0, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
			{
				perror("Writing to socket: option sending failed");
			}

#endif
		printf("Option Sent !!\n");
		// Sending the commad to server based on the option received
		int j;
		int dummy = 100;
		switch(option)
		{
			// This command gets the file from server
			case GET_FILE:

					for (i=0;i<MAX_CONN;i++)
					{
						printf("**Option Sent !!\n");
						int n = send(sockfd[i], (void *)&dummy, sizeof(int), 0);	
						printf(" ....%d\n", n);
						if (n < 0)//, 0, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
						{
							sendFailedArray[i] = TRUE;
							perror("Writing to socket: option sending failed");
						}
						else
						{
							sendFailedArray[i] = FALSE;
						}
					}
				// choosing the filename
				printf("Enter the file name and subfoolder you wish to receive from\n");
				scanf("%s", getFileName);
				scanf("%s", subfolder);
				printf("The file name entered is %s\n", getFileName);
				printf("subfolder %s\n", subfolder);

				// while(1)
			 //    {
			 //    	if (!(picture = fopen(getFileName, "r"))) 
			 //    	{
			 //   			perror("fopen");
				// 		printf("These are the list of files in your folder\n");
			 //    		system("ls");	    	
			 //    		printf("Re enter the file name\n");
			 //    		scanf("%s", getFileName);
			 //    	}

			 //    	else{
			 //    		break;
			 //    	}
			 //    }

			    // coptimizing get
			    // for (i<0;i<MAX_CONN;i++)
			    // {
			    // 	if (!sendFailedArray[i]) 
			    // }
			    int serverToUse[MAX_CONN ] = {-1,-1,-1,-1};
			    if (!sendFailedArray[0])
			    {
			    	serverToUse[0] = 1;
			    	if (!sendFailedArray[2])
			    	//get from 0 and 2 
			    	{
			    		serverToUse[2] = 1;
			    		printf("COMPLETE 1\n");
			    	}
			    	else
			    	{
			    		if (!sendFailedArray[1] && !sendFailedArray[3])
			    		{
			    			serverToUse[1] = 1;	
			    			serverToUse[3] = 1;	
			    		}
			    		else
			    		{
			    			printf("INCOMPLETE 1\n");
			    			//goto end_of_get; 
			    		}
			    	}
			    }
			    else if (!sendFailedArray[1])
			    {
			    	serverToUse[1] = 1;
			    	if (!sendFailedArray[3])
			    	//get from 1 and 3  
			    	{
			    		printf("COMPLETE 2\n");
			    		serverToUse[3] = 1;
			    	}
			    	else
			    	{
			    		printf("INCOMPLETE 2\n");
			    		//goto end_of_get; 
			    	}
			    }
			    else
			    {
			    	printf("INCOMPLETE 3\n");
			    	//goto end_of_get;
			    }
			    for (i =0;i<MAX_CONN;i++)
			    {
			    	printf("%d\n",serverToUse[i]);
			    }
			   // exit(1);
			    // sending packet to notify server to start or not
			    int youCanSart;
			    for (i=0;i<MAX_CONN;i++)
			    {
			    	if (serverToUse[i] == 1)
			    	{
			    		youCanSart = TRUE;
		    			n = send(sockfd[i], (void *)&youCanSart, sizeof(int), 0);
						if (n < 0)//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
						{
							perror("option sending failed");
						}
					}

			    	else
			    	{
			    		youCanSart = FALSE;
		    			n = send(sockfd[i], (void *)&youCanSart, sizeof(int), 0);
						if (n < 0)//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
						{
							perror("option sending failed");
						}
					}					
				}
							
			   #if 1
			    for (i = 0; i< MAX_CONN; i++)
			    #else
			    	for (i = 0; i< 1; i++)
			   #endif
			    //tempVali = i;

			    {
			    	//if (!sendFailedArray[i])
			    	if (serverToUse[i] == 1)
			    	{
			    		printf("**************************************************************************************\n");

			    		if (sendUserDetails(sockfd[i])){
			    			// receivinf image 1
					    	n = send(sockfd[i], getFileName, 50, 0);
							if (n < 0)//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
							{
								perror("option sending failed");
							}
							receive_image(sockfd[i], getFileName, serverAddress[i], serverLength[i], subfolder);
							printf("**********************\n");
							//sleep(1);
							// receiving image 2
							n = send(sockfd[i], getFileName, 50, 0);
							if (n < 0)//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
							{
								perror("option sending failed");
							}
							receive_image(sockfd[i], getFileName, serverAddress[i], serverLength[i], subfolder);
							printf("**********************\n");
						}

						//}

					}

		
				}
				//exit(1);
				for (i = 0;i<MAX_CONN;i++)
				{
					serverToUse[i] = -1;
				}

				// start of concatenation
			    char fileLs[100];
    			char systemLSgetFiles[100];
    			char decryptSystemCmd[300];
    			bzero(systemLSgetFiles, sizeof(systemLSgetFiles));
			    strncpy(systemLSgetFiles, "ls -a .", strlen("ls -a .")); // ls 
			    strncat(systemLSgetFiles, getFileName, strlen(getFileName)); // ls [filename]
			    strncat(systemLSgetFiles, "*_rec*", strlen("*_rec*"));
			    char fileList[4][100];
			    printf("systemLSgetFiles %s\n", systemLSgetFiles);
			    FILE *f = popen(systemLSgetFiles, "r");
			    int i = 0;
			    while (fgets(fileLs, 100, f) != NULL) {
			    	bzero(fileList[i], sizeof(fileList[i]));
			    	
					strtok(fileLs,"  \t\n");
					//printf("fileLs %s\n", fileLs);
					strncpy(fileList[i], fileLs, sizeof(fileLs));
			        printf( "%s %lu\n", fileList[i], strlen(fileList[i]) );
			        i++;
			    }
			    pclose(f);
			    //printf("value of i %d\n", i);
			    bzero(decryptSystemCmd, sizeof(decryptSystemCmd));
			    readConfFile(0);
			    if (i<MAX_CONN)
			    {
			    	printf("FILE INCOMPLETE\n");
			    }
			    else
			    {
			    	char catCommand[300];
			    	for(i=0;i<MAX_CONN;i++)
			    	{
			    		sprintf(decryptSystemCmd,"openssl enc -d -aes-256-cbc -in %s -out de%s -k %s", fileList[i], fileList[i], PASSWORD);
			    		printf("mama here%s\n", decryptSystemCmd);
			    		system(decryptSystemCmd);
			    		//exit(1);

			    	}
			    		
			    	sprintf(catCommand,"cat de%s de%s de%s de%s > %s_received", fileList[0],fileList[1],fileList[2],fileList[3], getFileName);
			    	//printf("wow %s\n", catCommand);
			    	system(catCommand);
			    	printf("FILE CONCAT SUCCESSFUL\n");
			    	bzero(catCommand, sizeof(catCommand));
			    	system("rm .foo10_received .foo11_received .foo12_received .foo13_received");
			    }

			    bzero(decryptSystemCmd,sizeof(decryptSystemCmd));
				for (i=0;i<MAX_CONN;i++)
				{
					bzero(fileList[i],sizeof(fileList[i]));
				}
				//
			    // system("rm .foo10_received .foo11_received .foo12_received .foo13_received");
				//exit(1);
				//end_of_get:
				printf("Exiting get function\n");
				break;

			// This command puts the file onto server
			// USERNAME send
			// password send
			// ack for validity of user credentials receive
			// size send
			// filename send
			// fileContent send

			case PUT_FILE:

				printf("Enter the file name and subfolder you wish to send to\n");
				scanf("%s", putFileName);
				scanf("%s", subfolder);

				printf("putFileName %s\n", putFileName);
				printf("subfolder %s\n", subfolder);

				while(1)
			    {
			    	if (!(picture = fopen(putFileName, "r"))) 
			    	{
			   			perror("fopen");
						printf("These are the list of files in your folder\n");
			    		system("ls");	    	
			    		printf("Re enter the file name\n");
			    		scanf("%s", putFileName);
			    	}

			    	else{
			    		break;
			    	}
			    }
			   	
			    // computes the md5sum of the file and is excuted on if the valid filename is entered
			    calculateMd5Sum(putFileName, md5sum);
				md5sumInt = md5sum[strlen(md5sum)-1] % 4;

				md5sumIndex = (4-md5sumInt)%4;
				printf("md5sumIndex %d\n", md5sumIndex);
			    // split -n 4 -a 1 -d [filename] [filename_withindex]
			    printf("Dividing the file into four equal parts...\n");
			    char systemCommand[150];
			    char filename[100];
			    bzero(filename, sizeof(filename));
			    strncpy(filename,putFileName,sizeof(putFileName));
			    //strncpy(systemCommand, "split -n 4 -a 1 -d ", st("split -n 4 -a 1 -d "));
			    //strncat(filename, " ", sizeof(" "));
			    strncpy(filename, putFileName,strlen(putFileName));
			    sprintf(systemCommand,"split -n 4 -a 1 -d %s en%s",putFileName, putFileName);
			    //strncat(systemCommand, filename, sizeof(filename));
			    printf("%s\n", systemCommand);
			    system(systemCommand);

			    printf("Dividing done\n\n");
			    printf("Starting Encryption...\n");
			    printf("\nUSING AES Encryption\n");
			    char encryptSystemCmd[200];
			    bzero(encryptSystemCmd,sizeof(encryptSystemCmd));
			    readConfFile(0);
			    for (i=0;i<MAX_CONN;i++)
			    {
			    	sprintf(encryptSystemCmd,"openssl enc -aes-256-cbc -in en%s%d -out %s%d -k %s", putFileName,i, putFileName, i, PASSWORD);
			    	system(encryptSystemCmd);
			    	printf("%s\n", encryptSystemCmd);
			    }

			    char filenameWithIndex[4][100];
			    char fileIndex[1];
			    // Sending the file to server
			    /*******************************************
			    // Formulas for how finalIndex is calculated
			    // finalIndex = (i+md5sumIndex)%4
			    // md5sumIndex = (4-md5sumInt)%4;
			    // md5sumInt = md5sum[strlen(md5sum)-1] % 4;
			    //*******************************************/
			    #if 1
			    	for (i = 0; i< MAX_CONN; i++)
			    #else
			    	for (i = 0; i< 1; i++)
			    #endif
			    {
			    	if (!sendFailedArray[i])
			    	{
				    	if (sendUserDetails(sockfd[i]))
				    	{
					    	//creating the file name with index
					    	// first file
					    	finalIndex = (i+md5sumIndex)%4;
					    	printf("***********************            %d  %d\n", finalIndex, (finalIndex+1)%4);
					    	strncpy(filenameWithIndex[finalIndex], putFileName, sizeof(putFileName));
					    	sprintf(fileIndex,"%d",finalIndex);
					    	printf("fileIndex  :   %s\n", fileIndex);
					    	strncat(filenameWithIndex[finalIndex], fileIndex, 1);
					    	printf("filename %s\n", filenameWithIndex[finalIndex]);
					    	 
							sendFile(sockfd[i], filenameWithIndex[finalIndex], serverAddress[i], subfolder);
							sleep(1);
							// seconf file
							strncpy(filenameWithIndex[(finalIndex+1)%4], putFileName, sizeof(putFileName));
					    	sprintf(fileIndex,"%d",(finalIndex+1)%4);
					    	printf("fileIndex  :   %s\n", fileIndex);
					    	strncat(filenameWithIndex[(finalIndex+1)%4], fileIndex, 1);
					    	printf("filename %s\n", filenameWithIndex[(finalIndex+1)%4]);
							sendFile(sockfd[i], filenameWithIndex[(finalIndex+1)%4], serverAddress[i], subfolder);
							
							bzero(filenameWithIndex[finalIndex], sizeof(filenameWithIndex[finalIndex]));
							bzero(filenameWithIndex[(finalIndex+1)%4], sizeof(filenameWithIndex[(finalIndex+1)%4]));
							bzero(fileIndex, sizeof(fileIndex));
						}
					}
				}

				break;

			// This command gets the list of files form the server in its current directory
			case LIST_FILES:
				system("rm .list0_received .list1_received .list2_received .list3_received");
				printf("Enter the subfolder\n");
				scanf("%s", subfolder);
				
				int status =0;
				int checkNoCnnections = 0; 
				// this part should be uncommented
				for (i =0; i<MAX_CONN;i++)
				{
					sprintf(fileNameList, "list%d", i);
					//printf("fileNameList %s\n", fileNameList);
					if (!sendFailedArray[i])
					{ 
						if (sendUserDetails(sockfd[i]))
			    		{
						
							n = send(sockfd[i], fileNameList, 50, 0);
							if (n < 0)//, (struct sockaddr *)&servAddr, sizeof(servAddr))==-1)
							{
								perror("option sending failed");
							}
							//exit(1);
							status = receive_image(sockfd[i], fileNameList, serverAddress[i], serverLength[i], subfolder);
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
