#include <dirent.h>
#include <fcntl.h>
#include  <stdlib.h>
#include  <stdio.h>
#include <unistd.h>
#include  <sys/types.h>
#include <sys/stat.h>
#include  <sys/socket.h>
#include  <netinet/in.h>       /* struct sockaddr_in, htons, htonl */
#include  <netdb.h>            /* struct hostent, gethostbyname() */
#include  <string.h>
#include <errno.h>
#include <ctype.h>

#include  "stream.h"           /* MAX_BLOCK_SIZE, readn(), writen() */
#include "values.h"

#define   SERV_TCP_PORT  40708 /* default server listening port */


void lpwd(char buf[], int bufsize);				//To display the current directory of the client
void ldir();							//To display the file listings
void lcd(char buf[]);				//Change to directory to a specified path
void cd(int sd);					//Sends protocol to change to the directory to a specified path in the server
void put(int sd, char newbuf[]);
void get(int sd);
void help();

int main(int argc, char *argv[])
{
  int sd, n, nr, nw, i=0;
  char buf[MAX_BLOCK_SIZE], host[60];
  char cpybuf[MAX_BLOCK_SIZE];
  char pathname[MAX_BLOCK_SIZE];
  unsigned short port;
  struct sockaddr_in ser_addr;
  int integer_pwd, integer_dir;
  struct hostent *hp;
  char *sen;
  int num, integer;
  int length;
  int fileValid = 0;
  int fileExist = 0;
  int fd;
  ssize_t nread;
  FILE *fpt;

  /* get server host name and port number */
  if (argc==1) {  /* assume server running on the local host and on default port */
      strcpy(host, "localhost");
      port = SERV_TCP_PORT;
  } else if (argc == 2) { /* use the given host name */
      strcpy(host, argv[1]);
      port = SERV_TCP_PORT;
  } else if (argc == 3) { // use given host and port for server
     strcpy(host, argv[1]);
      int n = atoi(argv[2]);
      if (n >= 1024 && n < 65536)
          port = n;
      else {
          printf("Error: server port number must be between 1024 and 65535\n");
          exit(1);
      }
  } else {
     printf("Usage: %s [ <server host name> [ <server listening port> ] ]\n", argv[0]);
     exit(1);
  }

  /* get host address, & build a server socket address */
  bzero((char *) &ser_addr, sizeof(ser_addr));
  ser_addr.sin_family = AF_INET;
  ser_addr.sin_port = htons(port);
  if ((hp = gethostbyname(host)) == NULL){
       printf("host %s not found\n", host); exit(1);
  }
  ser_addr.sin_addr.s_addr = * (u_long *) hp->h_addr;

  /* create TCP socket & connect socket to server address */
  sd = socket(PF_INET, SOCK_STREAM, 0);
  if (connect(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0) {
      perror("client connect"); exit(1);
  }

  int bufsize;
  while (++i) {				//while loop for option
    printf("myftp[%d]> ", i);
    fgets(buf, sizeof(buf), stdin);		//user input
    nr =strlen(buf);
    bufsize = sizeof(buf);
    if (buf[nr-1] == '\n') {
      buf[nr-1] = '\0';
      nr--;
    }
    if((strcmp(buf, "quit")==0)||(strcmp(buf, "help")==0)||(strcmp(buf, "lpwd")==0)||(strcmp(buf, "ldir")==0)||(strcmp(buf,"pwd")==0)
        ||(strcmp(buf, "dir")==0)||(strncmp(buf, "lcd", 3)==0)||(strncmp(buf, "cd", 2)==0)||(strncmp(buf, "put", 3)==0)||(strncmp(buf, "get", 3)==0)){
      /*quit the program*/
      if (strcmp(buf, "quit")==0) {
         printf("Bye from client\n");
         nw = writen(sd, CMD_QUIT, 1);
         exit(0);
      }
      /*help the program*/
      else if(strcmp(buf, "help") == 0){
        help();
      }
      /*support command lpwd*/
      else if(strcmp(buf, "lpwd") == 0){
        lpwd(buf, bufsize);
    	}
      /*support command ldir*/
      else if(strcmp(buf, "ldir") == 0){
    		ldir();
    	}
      /*support command pwd*/
      else if(strcmp(buf,"pwd") == 0){
    		nw = writen(sd, CMD_PWD, 1);      //send ASCII code 'W' to server
        nr=readn(sd, buf, sizeof(buf));    //recieve ASCII code 'W' from server
        buf[nr] = '\0';
        /*if the server responds with code 'W'*/
        if(strcmp(buf,CMD_PWD) == 0){
         nr=readn(sd, buf, sizeof(buf));  //recieve the length of path name from server
         integer_pwd = atoi(buf);        //length of the server pathname to integer
         if(integer_pwd > 0){
            nr=readn(sd, buf, sizeof(buf));     //recieve the path name from server
            printf("myftpd[%d]> %s\n", i, buf);	  //print current directory of server
         }
        }
    	}
      /*support command dir*/
      else if(strcmp(buf, "dir")== 0){
        /*erase the buffers*/
        bzero(buf, sizeof (buf));
        bzero(cpybuf, sizeof (cpybuf));

        int l=0;
        nw = writen(sd, CMD_DIR, 1);          //send ASCII code 'D' to server
        nr=readn(sd, buf, sizeof(buf));       //recieve ASCII code 'D' from server

        if (strcmp(buf, CMD_DIR)== 0){	//server replied 'D'
          bzero(buf, sizeof (buf));
          //nr = readn(sd, buf, sizeof(buf));
         int status;
         nr = readn(sd,buf, sizeof(buf));   //recieve the status from server
         status = atoi(buf);
         if(status >= 0){
           nr = readn(sd, cpybuf, sizeof(buf));       //recieve the name of the list of files and folders
           printf("%s\n\n", cpybuf);
         }else
            printf("\n");
         }
      }
      /*support command lcd*/
      else if(strncmp(buf, "lcd", 3) == 0){
        if(strlen(buf)<5){
          printf("Usage: cd <name of the folder>\n");
        }
        else{
          lcd(buf);
        }
      }
      /*support command cd*/
      else if(strncmp(buf, "cd", 2) == 0){
        char newbuf[100];
        char temp[100];
        int value, n;
        int status=0;
        int c=0, len=0;
        /*check if the client input the name of directory*/
        if(strlen(buf)<4){
          printf("Usage: cd <name of the folder>\n");
        }
        else{
          writen(sd, CMD_CD, 1);    //send ASCII code 'C' to server
          strcpy(newbuf, buf);      //copy buffer into the new buffer
          //printf("%s\n", newbuf);
          send(sd, newbuf, 100,0);      //send the new buffer to server
          recv(sd, &status, sizeof(int), 0);  //recieve the status from server
          //printf("%d........%d\n", status, STATUS_OKAY);
          if(!status){
              printf("Succeed in changing the directory\n");
          }else{
            printf("Failed in changing the directory\n");
          }
        }
      }
      /*support command put*/
      else if(strncmp(buf, "put", 3) == 0){
        int l=0, c=0;
        char newbuf[MAX_BLOCK_SIZE];
        if(strlen(buf)<5){
          printf("Usage: put <name of the text file>\n");
        }
        else{
          writen(sd, CMD_PUT, 1);     //send ASCII code 'P' to server
          /*extract the file name after put*/
          l = strlen(buf);
          while(c<l)
          {
            newbuf[c] = buf[5+c-1];
            c++;
          }
          newbuf[c] = '\0';
          put(sd, newbuf);
        }
      }
      /*support command get*/
      else if(strncmp(buf, "get", 3) == 0){
        int l=0, c=0;
        char newbuf[MAX_BLOCK_SIZE];
        if(strlen(buf)<5){
          printf("Usage: get <name of the text file>\n");
        }
        else{
          writen(sd, CMD_GET, 1);         //send ASCII code 'G' to server
          strcpy(newbuf, buf);
          printf("%s\n", newbuf);
          send(sd, newbuf, 100,0);
          get(sd);
        }
      }
    }else{
      printf("Type 'help' to learn more about the commands supported.\n");
    }
  }	//close while loop for option
  return(0);
}//end of main


//--------------------------------------------------------------------------------------------------------------------
/*
* Function used to print the directory of client.
* @param: char buf[], integer bufsize.
* @return
*/
void lpwd(char buf[], int bufsize){
  bzero(buf, MAX_BLOCK_SIZE);       //erase the data in the buf array
  if(getcwd(buf,bufsize)){        //determine the path name of the current client directory
    printf("\t%s\n", buf);        //print the directory
  }
  else{
    printf("Error");
  }
  return;
}

//--------------------------------------------------------------------------------------------------------------------
/*
* Function used to print the list of files in the current folder.
* @param
* @return
*/
void ldir(){
	struct dirent * de;
  int count =0;
	DIR *dirp = opendir(".");    //open the directory

  if (dirp == NULL)
  {
    printf("Could not open current directory");
    return;
  }

  while((de=readdir(dirp)) != NULL){		//loop till there is no more files or folders
      printf("%s\n", de->d_name);     //determine and print the name of each files or folders
  	}
  printf("\n");
}

//--------------------------------------------------------------------------------------------------------------------
/*
* Function used to move the directory ordered by user from the current directory.
* @param: char buf[]
* @return
*/
void lcd(char buf[]){
  int c=0, len=0;
  char drname[100];

/*extract the path name after lcd*/
  len = strlen(buf);    //get the lengh of the buffer
  while(c<len){         //loop until c reaches to the same number with len
    drname[c] = buf[5+c-1];       //5 is where the input of name starts
    c++;
  }
  drname[c] = '\0';
  //printf("%s////%lu", drname, strlen(drname));
  if(strcmp(drname, "..")==0){        //when the path name is ".."
    if(chdir("..")==0){           //chdir is to change the current directory
      printf("Successfully moved the directory back one directory\n");
    }else{
      printf("Failed in moving the directory back one directory\n");
    }
  }else if(strcmp(drname, ".")==0){     //when the path name is "."
    if(chdir(".")==0){
      printf("Successfully moved in the same directory as the current directory\n");
    }else{
      printf("Failed in moving in the same directory as the current directory\n");
    }
  }else{
    if(chdir(drname)==0){
      printf("Successfully changed the current directory to %s\n",drname);
    }else{
      printf("Failed in changing the current directory to %s\n",drname);
    }
  }
}

/*
* Function used to send a file from the Client to the Server.
* @param: integer sd.
* @return
*/
void put(int sd, char newbuf[])
{
  FILE *f;                                  //File pointer.
  char buf[MAX_BLOCK_SIZE];                 //Buffer to store values.
  char c;                                   //Char used to loop through file.

  f=fopen(newbuf,"r");              //Open the .txt file to be sent to the Server (Read only).
  if (f < 0)
  {
    printf("Error: Opening file failed (Client Side Send)\n");
    perror("Error");
    exit(1);
  }
  int words=0;                              //Int used to store number of words in the .txt file.
  while((c=getc(f))!=EOF)			          //Counting No of words in the file
  {
    fscanf(f , "%s" , buf);               //Go through the file word by word
    if(isspace(c)||c=='\t')     //Check if there is a space or tab.
    words++;
  }
  words=words+1;                            //Increment word count by 1 to include the last word in the .txt file.

  write(sd, &words, sizeof(int));           //Write number of words for Server.
  rewind(f);                                //Return the file pointer to the start of the file.

  char ch;                                  //Char used to look through file.
  while(ch !=EOF)                           //Loop through till End Of File.
  {
    fscanf(f, "%s", buf);                 //Go through the file word
    write(sd, buf, 512);                  //Write to server.
    ch = fgetc(f);                        //Update the pointer
  }
  printf("File has been sent.\n");
  fclose(f);
}

//--------------------------------------------------------------------------------------------------------------------
/*
* Function used to recieve a file from the Server.
* @param: integer sd.
* @return
*/
void get(int sd)
{
  FILE *fp;                            //File pointer.
  char buf[MAX_BLOCK_SIZE];            //Buffer to store values.


  fp = fopen("get_recv.txt","w");      //Create new .txt file in Client(Write only).
  if (fp < 0)
  {
    printf("Error: Opening file failed (Client Side Receive)\n");
    printf("Error description: %s\n",strerror(errno));
    exit(1);
  }
  int words;                           //Int used to store number of words in the .txt file.
  read(sd, &words, sizeof(int));       //Read the number of words in the file recieved from Server.

  int ch=0;                             //Char used to loop through file.
  while(ch != words)                   //Loop runs 'words' number of time.
  {
    read(sd, buf, 512);              //Read word by word what the Server sends.
    fprintf(fp , "%s " , buf);       //Print into text file.
    ch++;
  }
  printf("The file has been received.\n");
  fclose(fp);
}


 //--------------------------------------------------------------------------------------------------------------------
 /*
 * Function to guide users for using commands.
 * @param
 * @return
 */
 void help()
 {
     printf("-------------------------------------------------------------------------------\n");
     printf("•pwd - to display the current directory of the server.\n");
     printf("•lpwd - to display the current directory of the client.\n");
     printf("•dir - to display the file names under the current directory of the server.\n");
     printf("•ldir - to display the file names under the current directory of the client.\n");
     printf("•cd directory_pathname - to change the current directory of the server.\n");
     printf("•cd . - to stayin the same directory you are currently in, in the server.\n");
     printf("•cd .. - to move up one directory in the server.\n");
     printf("•lcd directory_pathname - to change the current directory of the client.\n");
     printf("•lcd . - to stayin the same directory you are currently in, in the client.\n");
     printf("•lcd .. - to move up one directory in the client.\n");
     printf("•get filename - to download a file from the server and save it into the client.\n");
     printf("•put filename - to upload a file from the client to the server.\n");
     printf("•quit - to terminate the myftp session.\n");
     printf("-------------------------------------------------------------------------------\n");
 }
