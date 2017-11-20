NETSYS Programming Assignment 3
Name: Chaitra Ramachandra

------------------------------------CONTENTS-----------------------------------------
The client folder contains:
1. dfc.c - This is the client code
2. dfc.conf - Configuration file for client
3. Makefile

The server folder contains:
1. dfs.c - This is the server code
2. dfs.conf - Configuration file for server
3. Makefile

---------------------------------HOW TO RUN THE CODE---------------------------------
Start the Server on terminal as follows:
./dfs 10001 dfs.conf
./dfs 10002 dfs.conf
./dfs 10003 dfs.conf
./dfs 10004 dfs.conf

Start the Client on terminal as follows:
./dfc dfc.conf

------------------------------COMMANDS IMPLEMENTED-----------------------------------
GET:
By giving filename and subfolder,
- Fetches the file parts from all the 4 servers
- gets files from username folder by default
- gets files from subDirectory if sub folder is given

PUT:
By giving filename and subfolder,
- Splits the given file into 4 parts and stores on 4 servers based on MD5 index
- puts file into username folder by default
- puts file into subDirectory if sub folder is given

LIST:
- lists all files on the server
- lists files from username folder by default
- lists files from subfolder if subfolder is given

MKDIR:
- creates subfolder into Username folder

-------------------------------EXTRA CREDITS / MANDATORY------------------------------
Subfolder using MKDIR
Encryption/Decryption system open ssl aes command
Optimized get (part of it)