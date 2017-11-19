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

#define GET_FILE 1
#define PUT_FILE  2
#define LIST_FILES 3
#define CLOSE_SOC 4

#define VALID 1
#define INVALID 0

#define PASS 1
#define FAIL 0
#define TRUE 1
#define FALSE 0

#define MAX_CONN 4
#define TIMEOUT 1
#define FIRSTFILE 0
#define SECONDFILE 1

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





 
  /*****************************************************
 * send_image: Transfers the file from Server to Client
 
 ARGUMENTS
 int sockfd - descriptor for socket created  
 char *filename - filename to send from Server to Client
 struct sockaddr_in servAddr  - contains the client informatiom
 *****************************************************/

int send_image(int sockfd, char *filename, struct sockaddr_in clientAddress, int portIndex, int fileNumber){

   FILE *picture;                   // Reads the picture
   char subDirectory[100];
   // size: stores the total size of the image, read_size: stores the return value of func recvfrm() 
   // Sequence number of the packer being sent
   // send_buffer: buffer to send the packet read_buffer: reads the message server


   size_t  read_size, recvBytes; 
   int packet_index, size;
   struct timeval timeout = {2,0}; // determines the timeout
   char send_buffer[1024], read_buffer[256];
   char systemListCmd[200];
   bzero(systemListCmd, sizeof(systemListCmd));
   packet_index = 1;

  fd_set fds;
  int buffer_fd, buffer_out, flags;
  socklen_t servlen = sizeof(clientAddress);  // new

  char dfsMainFolder[100];
  bzero(dfsMainFolder, sizeof(dfsMainFolder));

  // variables for opeing a directory
  DIR           *d;
  struct dirent *dir;
  int count = 0;
  int index = 0;
  char name[256][256];

  // receive the subDirectory name
  recvBytes = recv(sockfd, subDirectory, 100, 0);  // reading the filename 
  if (recvBytes < 0)
  {
    perror("Error receiving subDirectory");
  }
  printf("subDirectory%s\n", subDirectory);


  // USED for list dude
  if (fileNumber == -1)
  {
    if (strcmp(subDirectory,"/") == 0)
    {
      sprintf(systemListCmd, "ls -a DFS%d/%s > DFS%d/%s/.%s", portIndex, USERDATA.USERNAME, portIndex, USERDATA.USERNAME, filename);
      printf("systemListCmd %s\n", systemListCmd);
      system(systemListCmd);
      printf("\n\nEmpty subDirectory received\n\n");
      // have to send an acknowledgement if subDirectory is present or not
    }
    else
    {
      sprintf(systemListCmd, "ls -a DFS%d/%s/%s > DFS%d/%s/%s/.%s", portIndex, USERDATA.USERNAME, subDirectory,portIndex, USERDATA.USERNAME, subDirectory,filename);
      printf("systemListCmd %s\n", systemListCmd);
      system(systemListCmd);
      printf("&&&&&&&&&&&&&&&&&&& %s\n", systemListCmd);
    }

  }
  char dirToOpenWithUser[100];
  switch(portIndex)
  {
    case 1:
      strncpy(dfsMainFolder, "DFS1", sizeof("DFS1"));
      strncpy(dirToOpenWithUser, "./DFS1/", sizeof("./DFS1/"));
      strncat(dirToOpenWithUser, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
      //d = opendir(dirToOpenWithUser);
      //system("mkdir -p DFS1");
    break;
    case 2:
      strncpy(dfsMainFolder, "DFS2", sizeof("DFS2"));
      strncpy(dirToOpenWithUser, "./DFS2/", sizeof("./DFS2/"));
      strncat(dirToOpenWithUser, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
      //d = opendir(dirToOpenWithUser);;
      //system("mkdir -p DFS2");
    break;
    case 3:
      strncpy(dfsMainFolder, "DFS3", sizeof("DFS3"));
      strncpy(dirToOpenWithUser, "./DFS3/", sizeof("./DFS3/"));
      strncat(dirToOpenWithUser, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
      //d = opendir(dirToOpenWithUser);
      //system("mkdir -p DFS3");
    break;
    case 4:
      printf("I am coming here\n");
      strncpy(dfsMainFolder, "DFS4", sizeof("DFS4"));
      strncpy(dirToOpenWithUser, "./DFS4/", sizeof("./DFS4/"));
      strncat(dirToOpenWithUser, USERDATA.USERNAME, strlen(USERDATA.USERNAME));
      //d = opendir(dirToOpenWithUser);
      //system("mkdir -p DFS4");
    break;    
  }
  if (strcmp(subDirectory,"/") == 0)
  {
    printf("\n\nEmpty subDirectory received\n\n");
    // have to send an acknowledgement if subDirectory is present or not
  }
  else
  {
    sprintf(dirToOpenWithUser, "%s/%s", dirToOpenWithUser, subDirectory);
  }

  printf("dirToOpenWithUser %s\n", dirToOpenWithUser);
  //exit(1);
  // d = opendir(dirToOpenWithUser);
  // bzero(dirToOpenWithUser, sizeof(dirToOpenWithUser));

  char filename_dummy[100];
  bzero(filename_dummy, sizeof(filename_dummy));
  #if 0
  strcpy(filename_dummy, filename);
  #else
  sprintf(filename_dummy,".%s",filename);
  #endif

  // copies the current list of files in to the array name[count]
  //printf("************************************\n");
  int filerecvBytesus = -5;
  if (fileNumber != -1)
  {
    d = opendir(dirToOpenWithUser);
    //bzero(dirToOpenWithUser, sizeof(dirToOpenWithUser));
    if (d)
    {
      while ((dir = readdir(d)) != NULL)
      {
        
        //strcpy(name[count],dir->d_name);
          //printf("dir->d_name: %s %d\n", dir->d_name, sizeof(dir->d_name));
          //printf("filename_dummy: %s %d\n", filename_dummy, strlen(filename));
        //printf("%s\n", );
        if (strncmp(dir->d_name, filename_dummy, strlen(filename)) == 0)
        {
          printf(" dir->d_name[strlen(filename) %d\n",  dir->d_name[strlen(filename) + 1]);
          if ((dir->d_name[strlen(filename) + 2]>=0) &&  (dir->d_name[strlen(filename) + 2]<=3))
          {
            strcpy(name[count],dir->d_name);
            printf("name[count] %s\n", name[count]);
            count++;
          }
        }

      }

      closedir(d);
    }
    else
    {
      printf("FILE NOT FOUND serious issue\n");
      //filerecvBytesus = 0;
    }
    
    if (count == 0)
    {
    filerecvBytesus = 0;
    }
  }

  printf("************************************\n");

  
  strncat(dfsMainFolder,"/",sizeof("/")); //dfs[]/
  strncat(dfsMainFolder,USERDATA.USERNAME,sizeof(USERDATA.USERNAME)); //dfs[]/USERNAME
  strncat(dfsMainFolder,"/",sizeof("/")); //dfs[]/USERNAME/
  printf("dfsMainFolder %s\n", dfsMainFolder);
  char fndot[100];
  bzero(fndot,sizeof(fndot));
  #if 0
  
  strcpy(fndot, name[fileNumber]);  // take care over here

  #else
  //strncpy(fndot,".",sizeof("."));
  if (fileNumber != -1)
    strcat(fndot, name[fileNumber]);
  else
  {
    sprintf(fndot, ".%s", filename);
    if (strcmp(subDirectory,"/") == 0)
      //sprintf(dfsMainFolder,"%s%s/",dfsMainFolder);
    {

    }
    else
      sprintf(dfsMainFolder,"%s%s/",dfsMainFolder, subDirectory);
  }
  #endif

  printf("filename %s\n", filename);
  printf("fndot%s\n", fndot);
  // sending the filename from server to client
  send(sockfd, fndot, sizeof(fndot), 0);

  //printf("********************fndot: %s\n", fndot);
  strncat(dfsMainFolder,fndot, sizeof(fndot));
  printf("dfs folder latest%s\n", dfsMainFolder);
  //exit(1);
  char checkFileExists[200];
   bzero(checkFileExists, sizeof(checkFileExists));
   printf("fileNumber %d\n", fileNumber);

   if (filerecvBytesus == 0)
   {
    printf("FILE UNAVAILABLE\n");
   }
   else if (fileNumber>=-1)
   {
      if (strcmp(subDirectory,"/") == 0)  
      {
        sprintf(checkFileExists,"DFS%d/%s/%s",portIndex,USERDATA.USERNAME,fndot);
      }
      else
      {
        printf("comine gere\n");
       sprintf(checkFileExists,"DFS%d/%s/%s/%s",portIndex,USERDATA.USERNAME,subDirectory,fndot); 
      }
      printf("checkFileExists: %s\n", checkFileExists);
      if (access (checkFileExists, F_OK) != -1)
       {
        printf("FILE EXISTS\n");
          filerecvBytesus = 1;
           send(sockfd, (void *)&filerecvBytesus, sizeof(int), 0);
          if (recvBytes < 0)
         {
          perror("Error sending size");
          exit(1);
         }

       }
       else
       {
        filerecvBytesus = 0;
        printf("FILE NOT EXISTS\n");
           send(sockfd, (void *)&filerecvBytesus, sizeof(int), 0);
          if (recvBytes < 0)
         {
          perror("Error sending size");
          exit(1);
         }

       }

   }
   else
   {
    filerecvBytesus = -1;
               send(sockfd, (void *)&filerecvBytesus, sizeof(int), 0);
          if (recvBytes < 0)
         {
          perror("Error sending size");
          exit(1);
         }

   }

   if (filerecvBytesus == 1 || filerecvBytesus == -1)
   {
      if (!(picture = fopen(dfsMainFolder, "r"))) 
      {
        perror("fopen");
        //return -1;
      }    
      bzero(dfsMainFolder, sizeof(dfsMainFolder));
       printf("Finding the size of the file using fseek\n"); 
       
       fseek(picture, 0, SEEK_END);
       size = ftell(picture);
       fseek(picture, 0, SEEK_SET);
       printf("Total file size is: %d\n",size);


       printf("Sending Picture Size from Server to Client\n");
       send(sockfd, (void *)&size, sizeof(int), 0);
        if (recvBytes < 0)
       {
        perror("Error sending size");
        exit(1);
       }


      char sendBuf_withpcktSize[BUFLEN];

       while(!feof(picture)) {
          read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, picture);
     

          do{
       
                recvBytes = send(sockfd, send_buffer, read_size, 0);//, (struct sockaddr *)&clientAddr, sizeof(clientAddr));

      
          }while (recvBytes < 0);

          printf(" \n");
          printf(" \n");


          //Zero out our send buffer
          bzero(send_buffer, sizeof(send_buffer));
          //bzero(sendBuf_withpcktSize, sizeof(sendBuf_withpcktSize));
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

	printf("Step: Receiving file size\n");
	recvBytes = recv(sockfd, &fSize, sizeof(int), 0);
	if(recvBytes < 0)
		perror("Error in receiving the file size\n");
	printf("file size: %d\n", fSize);

	printf("Step: Receiving file name\n");
	recvBytes = recv(sockfd, fname, 100, 0);
	if(recvBytes < 0)
		perror("Error in receiving the file name\n");
	printf("file name: %s\n", fname);

	printf("Step: Receiving subDirectory\n");
	recvBytes = recv(sockfd, subDirectory, 100, 0);
	if (recvBytes < 0)
		perror("Error in receiving the subDirectory\n");
	printf("subDirectory: %s\n", subDirectory);
	
	bzero(fnameReceived, sizeof(fnameReceived));
	strncat(fnameReceived, fname, 100);
	strncat(fnameReceived, "_recv", 100);
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
	acceptSock = accept(sockfd,(struct sockaddr *) &clientAddress, &clientSize);
	if(acceptSock < 0)
		perror("ERROR on accept");

	for(;;){
		n = recv(acceptSock, (void *)&option, sizeof(int), 0);
		if (n < 0)
        perror("option receiving failed");

    switch(option){
		//------------------------CHOICE == GET-----------------------------
		case GET_FILE:
              n = recv(acceptSock, (void *)&dummy, sizeof(int), 0);
              if (n < 0)
              {
                perror("option receiving failed");
                //exit(1);
              }      
              
              n = recv(acceptSock, (void *)&start, sizeof(int), 0);    // Reading the option from user side
              if (n < 0)
              {
                perror("option receiving failed");
                //exit(1);
              } 
              if (start)
              {
                printf("receiving the file name\n");
                  if (validateUserDetails(acceptSock)){
                    recvBytes = recv(acceptSock, filenameGet, 50,0);
                     if (recvBytes < 0)
                     {
                      perror("Error sending fname");
                      exit(1);
                     }
                    
                    // sending the fp

                    send_image(acceptSock, filenameGet, clientAddress, portIndex, FIRSTFILE);  
                    printf("Exiting the GET_FILE\n");
                    sleep(1);
                    recvBytes = recv(acceptSock, filenameGet, 50,0);
                     if (recvBytes < 0)
                     {
                      perror("Error sending fname");
                      exit(1);
                     }
                    
                    // sending the fp

                    send_image(acceptSock, filenameGet, clientAddress, portIndex, SECONDFILE);  
                    printf("Exiting the GET_FILE\n");
                }
              }
          option = 10;
      break;

		//------------------------CHOICE == PUT-----------------------------
		case PUT_FILE:
            printf("Step: PUT\n");
            if(validateUserDetails(acceptSock)){ 
              //receive first chunk of file
              receiveFile(acceptSock, clientAddress, clientSize, portIndex); 
              printf("Step: completed put for file chunk 1\n");

              //receive second chunk of file
              receiveFile(acceptSock, clientAddress, clientSize, portIndex); 
              printf("Step: completed put for file chunk 2\n");
            }
            option = 10;
            break;

		//------------------------CHOICE == LIST-----------------------------
		case LIST_FILES:
			//bzero(systemListCmd, sizeof(systemListCmd));
			if (validateUserDetails(acceptSock))
			{

			  

			  recvBytes = recv(acceptSock, fileNameList, 50,0);
			  if (recvBytes < 0)
			  {
				perror("Error sending fname");
				exit(1);
			  }
			  // sprintf(systemListCmd, "ls -a DFS%d/%s > DFS%d/%s/.%s", portIndex, USERDATA.USERNAME, portIndex, USERDATA.USERNAME, fileNameList);
			  // printf("systemListCmd %s\n", systemListCmd);
			  // system(systemListCmd);
			  send_image(acceptSock, fileNameList, clientAddress, portIndex, -1); 
			  sleep(1);
			}

           option = 10;
			break;

		//------------------------CHOICE == EXIT-----------------------------
		case CLOSE_SOC:
			printf("Closing the socket and exiting\n");
			close(sockfd);   // closing the socket and exiting
			exit(1);
			option = 10;
			break;

		default:
			break;
		}
	}//end of for
}
