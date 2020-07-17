#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include  <stdlib.h>     /* strlen(), strcmp() etc */
#include  <stdio.h>      /* printf()  */
#include  <string.h>     /* strlen(), strcmp() etc */
#include <unistd.h>
#include  <errno.h>      /* extern int errno, EINTR, perror() */
#include  <signal.h>     /* SIGCHLD, sigaction() */
#include  <syslog.h>
#include <arpa/inet.h>
#include  <sys/types.h>  /* pid_t, u_long, u_short */
#include  <sys/socket.h> /* struct sockaddr, socket(), etc */
#include  <sys/wait.h>   /* waitpid(), WNOHAND */
#include  <netinet/in.h> /* struct sockaddr_in, htons(), htonl(), */
#include <ctype.h>
                         /* and INADDR_ANY */
#include  "stream.h"     /* MAX_BLOCK_SIZE, readn(), writen() */
#include "values.h"

#define   SERV_TCP_PORT   40708   /* default server listening port */

void pwd(int sd);
void dir(int sd, char *buf);
int cd(int sd);
void put(int nsd);
void get(int nsd);



//--------------------------------------------------------------------------------------------------------------------
/*
* Function used to send a file from the Server to the Client.
* @param: integer nsd.
* @return
*/
void get(int nsd)
{
  FILE *f;                                //File pointer.
  char buf[MAX_BLOCK_SIZE];               //Buffer to store values.
  char c=0, l=0;
  char newbuf[100];
  FILE *lf;

  /*open the log file*/
  lf = fopen("logfile", "a+");	//open log file
  if (lf == NULL){
      printf("cannot create log file \n");
      exit(1);
  }
	recv(nsd, buf, 100, 0);
  fprintf(lf, "Client -> Server: %s \n", buf);
  /*extract the file name after get*/
	l = strlen(buf);
  while(c<l){
    newbuf[c] = buf[5+c-1];
    c++;
  }                           //Char used to loop through file.
  newbuf[c] = '\0';
  f=fopen(newbuf,"r");            //Open the .txt file to be sent to the Client (Read only).
  if (f < 0)
  {
    printf("Error: Opening file failed (Server Side Send)\n");
    printf("Error description: %s\n",strerror(errno));
    exit(1);
  }

  int words=0;                            //Int used to store number of words in the .txt file.
  while((c=getc(f))!=EOF)                 //Count the number of words in the file.
  {
  	fscanf(f , "%s" , buf);             //Go through the file word by word
  	if(isspace(c)||c=='\t')             //Check if there is a space or tab.
  	words++;                            // Increment word count.
  }
  words=words+1;                          //Increment word count by 1 to include the last word in the .txt file.

  write(nsd, &words, sizeof(int));        //Write word count for Server.
  fprintf(lf, "Server -> Client: %d \n", words);
  rewind(f);                              //Return the file pointer to the start of the file.

  char ch;                                //Char used to look through file.
  while(ch !=EOF)                         //Loop through till End Of File.
  {
      fscanf(f, "%s", buf);               //Go through the file word by word
      write(nsd, buf, 512);               //Write to server.
      fprintf(lf, "Server -> Client: %s \n", buf);
      ch = fgetc(f);                      //Update the pointer
  }
  printf("File has been sent.\n");
  fclose(f);
  fclose(lf);
}

//--------------------------------------------------------------------------------------------------------------------
/*
* Function used to recieve a file from the Client.
* @param: integer nsd.
* @return
*/
void put(int nsd){
  FILE *fp;                          //File pointer.
  char buf[MAX_BLOCK_SIZE];          //Buffer to store values.
  FILE *lf;

  /*open the log file*/
  lf = fopen("logfile", "a+");	//open log file
  if (lf == NULL){
   printf("cannot create log file \n");
   exit(1);
  }
  fp = fopen("put_recv.txt","w");    //Create new .txt file in Client(Write only).
  if (fp < 0)
  {
   printf("Error: Opening file failed (Server Side Receive)\n");
   perror("Error");
   exit(1);
  }
  int words;                         //Int used to store number of words in the .txt file.
  read(nsd, &words, sizeof(int));    //Read the number of words in the file recieved from Server.
  fprintf(lf, "Client -> Server: %d \n", words);

  int ch=0;                           //Char used to loop through file.
  while(ch != words)                 //Loop runs 'words' number of time.
  {
   read(nsd, buf, 512);           //Read word by word what the Server sends.
   fprintf(fp , "%s " , buf);     //Print into text file.
   fprintf(lf, "Client -> Server: %s \n", buf);
   ch++;
  }
  printf("The file has been recieved.\n");

  fclose(fp);                             //Close socket.
  fclose(lf);
}

//--------------------------------------------------------------------------------------------------------------------
/*
* Function used to print current directory of server.
* @param: integer sd.
* @return
*/
void pwd(int sd){
	char copybuf[MAX_BLOCK_SIZE];
  char str[MAX_BLOCK_SIZE];
	int len= 100;
  FILE *lf;

  /*open the log file*/
  lf = fopen("logfile", "a+");	//open log file
  if (lf == NULL){
      printf("cannot create log file \n");
      exit(1);
  }

	getcwd(copybuf, MAX_BLOCK_SIZE);     //execute pwd and write the string to cpybuf
	len =  strlen(copybuf);
	sprintf(str, "%d", len);         //copy the length of path name into char str[]
	writen(sd,str,sizeof(str));        //send the length of path name to client
  fprintf(lf, "Server -> Client: %s \n", str);
	writen(sd,copybuf,sizeof(copybuf));      //send the path name to client
  fprintf(lf, "Server -> Client: %s \n", copybuf);
  fclose(lf);
}

//--------------------------------------------------------------------------------------------------------------------
/*
* Function used to list all files and folders in the current folder.
* @param: integer sd, char *buf.
* @return
*/
void dir(int sd, char *buf){
	char files[MAX_BLOCK_SIZE];
  char tmp[MAX_BLOCK_SIZE];
	char str[10];
  char status;
  int len, nr, j = 0;
  int nw;
	int count = 0;
	int size;
	struct dirent * dirp;
	struct stat stats;
  DIR *dp;
  bzero(files, sizeof(files));
  FILE *lf;

  /*open the log file*/
  lf = fopen("logfile", "a+");	//open log file
  if (lf == NULL){
      printf("cannot create log file \n");
      exit(1);
  }

	dp = opendir(".");       //return DIR* to read the directory
  /*read the file or folder names in the current directory*/
	while((dirp = readdir(dp)) !=NULL){
		count++;        //count the number of files and folders in the current directory
    strcpy(tmp, dirp->d_name);    //get the name of files or folders

    /*check if there are more files or folders*/
    if(tmp[0] != '.'){
      if(j != 1)
        strcat(files, "\n\t");
      else
        strcat(files,"\t");
      strcat(files, dirp -> d_name);
    }
	}//end while
  nr = strlen(files);
  len = htons(nr);  //chamge to network bytes
  bcopy(&len, &buf[1], 2);  //copies 2 bytes from len to buf[1]

  /*check the status of result*/
  if(nr != 0)
    status = 0;
  else
    status = 1;

  //writen(sd, &buf[1], 2);
  writen(sd, &status, 1);   //send the status to client
  fprintf(lf, "Server -> Client: %s \n", &status);
  writen(sd, files, nr);    //send the name of the list of files and folders
  fprintf(lf, "Server -> Client: %s \n", files);
  fclose(lf);
}	//close dir function

//--------------------------------------------------------------------------------------------------------------------
/*
* Function used to move to the different directory.
* @param: integer sd.
* @return value
*/
int cd(int sd){
	int value;
	int status, l=0, c=0;
	char buf_value[100];
	char newbuf[100];
  FILE *lf;

  /*open the log file*/
  lf = fopen("logfile", "a+");	//open log file
  if (lf == NULL){
      printf("cannot create log file \n");
      exit(1);
  }

	recv(sd, buf_value, 100, 0);   //recieve the buffer from client
  fprintf(lf, "Server -> Client: %s \n", buf_value);

  /*extract the path name after cd*/
	l = strlen(buf_value);
  while(c<l){
    newbuf[c] = buf_value[4+c-1];
    c++;
  }
  newbuf[c] = '\0';
  /*determine the directory and change the directory according to the order*/
  if(strcmp(newbuf, "..")==0){
    if(chdir("..")==0){
      printf("Successfully moved the directory back one directory\n");
      status = STATUS_OKAY;
    }else{
      printf("Failed in moving the directory back one directory\n");
      status = STATUS_ERROR;
    }
  }else if(strcmp(newbuf, ".")==0){
    if(chdir(".")==0){
      printf("Successfully moved in the same directory as the current directory\n");
      status = STATUS_OKAY;
    }else{
      printf("Failed in moving in the same directory as the current directory\n");
      status = STATUS_ERROR;
    }
  }else{
    if(chdir(newbuf)==0){
      printf("Successfully changed the current directory to %s\n",newbuf);
      status = STATUS_OKAY;
    }else{
      printf("Failed in changing the current directory to %s\n",newbuf);
      status = STATUS_ERROR;
    }
  }
	value = send(sd, &status, sizeof(int),0);    //send the status of the result to client
  fprintf(lf, "Client -> Server: %d \n", status);
  fclose(lf);
	return value;
}

//--------------------------------------------------------------------------------------------------------------------
/*
* Function used to claim zombie processes, whicha are over
* @param
* @return
*/
void claim_children()
{
   pid_t pid=1;

   while (pid>0) { /* claim as many zombies as we can */
       pid = waitpid(0, (int *)0, WNOHANG);
   }
}	//end function claim_children

//--------------------------------------------------------------------------------------------------------------------
/*
* Function used to get processes into a deamon
* @param: void function
* @return
*/
void daemon_init(void)
{
   pid_t   pid;
   struct sigaction act;
   FILE *lf;

   /*open the log file*/
   lf = fopen("logfile", "a+");	//open log file
   if (lf == NULL){
       printf("cannot create log file \n");
       exit(1);
   }
   fprintf(lf, "--------------------------------------------\n");
   if ( (pid = fork()) < 0) {
        fprintf(lf, "The server connection is denied\n");
        perror("fork"); exit(1);
   } else if (pid > 0) {
        fprintf(lf, "The server connection is succeed.\n pid number is %d\n", pid);
        printf("Hay, you'd better remember my PID: %d\n", pid);
        exit(0);                  /* parent goes bye-bye */
   }

   /* child continues */
   setsid();                      /* become session leader */
//     chdir("/");                    /* change working directory */
   umask(0);                      /* clear file mode creation mask */

   /* catch SIGCHLD to remove zombies from system */
   act.sa_handler = claim_children; /* use reliable signal */
   sigemptyset(&act.sa_mask);       /* not to block other signals */
   act.sa_flags   = SA_NOCLDSTOP;   /* not catch stopped children */
   sigaction(SIGCHLD,(struct sigaction *)&act,(struct sigaction *)0);
   /* note: a less than perfect method is to use
            signal(SIGCHLD, claim_children);
   */
   fclose(lf);
}	//end function daemon_init

//--------------------------------------------------------------------------------------------------------------------
/*
* Function used to manage the communication with the client
* @param: integer sd.
* @return
*/
void serve_a_client(int sd)
{
  char buf[MAX_BLOCK_SIZE];
  int nr, nw;
  int	bufSize;
  int number;
  char file[100];
  int fd;
  ssize_t nread;
  FILE *fpt;
  FILE *lf;

  /*open the log file*/
  lf = fopen("logfile", "a+");	//open log file
  if (lf == NULL){
      printf("cannot create log file \n");
      exit(1);
  }
  while (1){	//infinite while loop
       /* read data from client */
       if ((nr = readn(sd, buf, sizeof(buf))) <= 0)
           return;   /* connection broken down */
       /* process data */
       buf[nr] = '\0';
				bufSize = sizeof(buf);
        fprintf(lf, "Client -> Server: %s \n", buf);
        /*if the client send the code 'Q'*/
        if(strcmp(buf,CMD_QUIT) == 0){
          fprintf(lf, "--------------------------------------------\n");
          fprintf(lf, "The server connection is ended\n");
				}
        /*if the client send the code 'W'*/
				if(strcmp(buf,CMD_PWD) == 0){
					writen(sd, buf, nr);    //response to the client with 'W'
          fprintf(lf, "Server -> Client: %s \n", buf);
          /* send results to client */
					pwd(sd);
				}
        /*if the client send the code 'D'*/
				if(strcmp(buf,CMD_DIR) == 0){
					writen(sd, buf, nr);		//response to the client with 'D'
          fprintf(lf, "Server -> Client: %s \n", buf);
          /* send results to client */
          dir(sd, buf);
				}
        /*if the client send the code 'C'*/
				if(strcmp(buf,CMD_CD) == 0){
          /* send results to client */
					cd(sd);
				}
        /*if the client send the code 'P'*/
				if(strcmp(buf,CMD_PUT) == 0){
          /* send results to client */
					put(sd);
				}
        /*if the client send the code 'G'*/
        if(strcmp(buf,CMD_GET) == 0){
          /* send results to client */
					get(sd);
				}
			}//close infinite while loop
			fclose(lf); 		//close log file
}	//end function serve_a_client


int main(int argc, char *argv[])
{
   int sd, nsd, n;
   pid_t pid;
   unsigned short port;   // server listening port
   socklen_t cli_addrlen;
   struct sockaddr_in ser_addr, cli_addr;
   int length;

   /* get the port number */
   if (argc == 1) {
        port = SERV_TCP_PORT;
   } else if (argc == 2) {

  int n = atoi(argv[1]);
        if (n >= 1024 && n < 65536)
            port = n;
        else {
            printf("Error: port number must be between 1024 and 65535\n");
            exit(1);
        }
   } else {
        printf("Usage: %s [ server listening port ]\n", argv[0]);
        exit(1);
   }

   /* turn the program into a daemon */
   daemon_init();

   /* set up listening socket sd */
   if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
         perror("server:socket"); exit(1);
   }

   /* build server Internet socket address */
   bzero((char *)&ser_addr, sizeof(ser_addr));
   ser_addr.sin_family = AF_INET;
   ser_addr.sin_port = htons(port);
   ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   /* note: accept client request sent to any one of the
      network interface(s) on this host.
   */

   /* bind server address to socket sd */
   if (bind(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0){
         perror("server bind"); exit(1);
   }

   /* become a listening socket */
   listen(sd, 5);

   while (1) {

        /* wait to accept a client request for connection */
        cli_addrlen = sizeof(cli_addr);
        nsd = accept(sd, (struct sockaddr *) &cli_addr, &cli_addrlen);
        if (nsd < 0) {
             if (errno == EINTR)   /* if interrupted by SIGCHLD */
                  continue;
             perror("server:accept"); exit(1);
        }

        /* create a child process to handle this client */
        if ((pid=fork()) <0) {
            perror("fork"); exit(1);
        } else if (pid > 0) {
            close(nsd);
            continue; /* parent to wait for next client */
        }

        /* now in child, serve the current client */
        close(sd);
        serve_a_client(nsd);
        exit(0);
   }
}	//end main
