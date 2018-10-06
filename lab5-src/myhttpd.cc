
const char * usage =
"                                                               \n"
"daytime-server:                                                \n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   daytime-server <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             \n"
"                                                               \n"
"In another window type:                                       \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where daytime-server  \n"
"is running. <port> is the port number you used when you run   \n"
"daytime-server.                                               \n"
"                                                               \n"
"Then type your name and return. You will get a greeting and   \n"
"the time of the day.                                          \n"
"                                                               \n";


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <dirent.h>

int QueueLength = 5;
char* key = "/asdf";
pthread_mutex_t m1;
int tempout = dup(1);
int reqCount = 0;
time_t currTime, start;
// Processes time request
void processTimeRequest( int socket );

extern "C" void killZombies(int sig) {
	while(waitpid(-1,0,WNOHANG) > 0);
}

void processRequestThread(int socket) {
  processTimeRequest(socket);
  close(socket);
}

void poolSlave(int socket) {
  while(1){
    pthread_mutex_lock(&m1);
    struct sockaddr_in clientIPAddress;
    int alen = sizeof( clientIPAddress );
    int slaveSocket = accept( socket,
	      (struct sockaddr *)&clientIPAddress,
	      (socklen_t*)&alen);
    if ( slaveSocket < 0 ) {
      perror( "accept" );
      exit( -1 );
    }
    pthread_mutex_unlock(&m1);
    processTimeRequest(slaveSocket);
    close(slaveSocket);
  }
}

int
main( int argc, char ** argv )
{
  // Print usage if not enough arguments
  if ( argc < 2 ) {
    fprintf( stderr, "%s", usage );
    exit( -1 );
  }
  
  // Get the port from the arguments
  int port;
  char* flag = NULL;
  if (argc == 2)
    port = atoi( argv[1] );
  else {
    port = atoi(argv[2]);
    flag = argv[1];
  }
  time(&start); 
  
  // Set the IP address and port for this server
  struct sockaddr_in serverIPAddress; 
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);
  
  // Allocate a socket
  int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    exit( -1 );
  }

  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1; 
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
		       (char *) &optval, sizeof( int ) );
   
  // Bind the socket to the IP address and port
  int error = bind( masterSocket,
		    (struct sockaddr *)&serverIPAddress,
		    sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    exit( -1 );
  }
  
  // Put socket in listening mode and set the 
  // size of the queue of unprocessed connections
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    exit( -1 );
  }
  if(flag)
    if(strcmp(flag,"-p") == 0) {
      pthread_t tid[5];
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
      for(int i = 0 ; i < 5 ; i++) {
        pthread_create(&(tid[i]),&attr,(void*(*)(void*))poolSlave,(void*)masterSocket);
      }
      pthread_join(tid[0],NULL);
  }// else {
      while ( 1 ) {

        // Accept incoming connections
        struct sockaddr_in clientIPAddress;
        int alen = sizeof( clientIPAddress );
        int slaveSocket = accept( masterSocket,
			      (struct sockaddr *)&clientIPAddress,
			      (socklen_t*)&alen);

        struct sigaction sa1;
        sa1.sa_handler = killZombies;
        sigemptyset(&sa1.sa_mask);
        sa1.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa1, 0) == -1)
          perror("sigaction");

        if ( slaveSocket < 0 ) {
          perror( "accept" );
          exit( -1 );
        }
        if(flag != NULL) {
          if(strcmp(flag,"-f") == 0) {
            pid_t slave = fork();
            reqCount++;
            if(slave == 0) {
              processTimeRequest(slaveSocket);
              close(slaveSocket);
              exit(EXIT_SUCCESS);
            } 
          } 
          if (strcmp(flag,"-t") == 0) {
            pthread_t t1;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
            pthread_create(&t1,&attr,
	      (void*(*)(void*))processRequestThread,(void*)slaveSocket);
            continue;
          }
        } else {
          // Process request.
          processTimeRequest( slaveSocket );
        }
        // Close socket
        close( slaveSocket );
      }
   //}
}


int endsWith(char* c1, char* c2) {
	int temp = strlen(c1) - strlen(c2);
	if(strncmp(c1+temp,c2,strlen(c2)) == 0)
		return 1;
	else
		return 0;
}

void
processTimeRequest( int fd )
{
  char *cwd = (char*)malloc(256);
  cwd = getcwd(cwd,256);
  reqCount++;
  printf("cwd=%s\n",cwd);
  printf("count==%d\n",reqCount);
  char* statsTemp = (char*)malloc(1024);
  strcat(statsTemp,cwd);
  strcat(statsTemp,"/http-root-dir/htdocs/stats.html");
  printf("statsTemp=%s\n",statsTemp);
  int tempFd = open(statsTemp, O_WRONLY | O_TRUNC | O_CREAT);
  if(tempFd < 0)
    printf("open error\n");
  char updateTemp[1024];
  char* update1 = "<h1>Aaron Althoff</h1><br/><h4>";
  char* update2 = "</h4>";
  sprintf(updateTemp, "%s%d%s",update1,reqCount,update2);
  write(tempFd,0,0);
  if(write < 0)
    printf("write error\n");
  close(tempFd);
  // Buffer used to store the name received from the client
  const int MaxName = 65000;
  char name[ MaxName + 1 ];
  char path[ MaxName + 1 ];
  int nameLength = 0;
  int n;
  // Send prompt
  //const char * prompt = "\nType your name:";
  //write( fd, prompt, strlen( prompt ) );

  // Currently character read
  unsigned char newChar;

  // Last character read
  unsigned char lastChar = 0;
  unsigned char thirdChar = 0;
  unsigned char fourthChar = 0;
  int gotGet = 0;
  int gotPath = 0;
  int pathLength = 0;
  //int gotKey = 0;
  int count = 0;
  char keyCheck[10];
  //
  // The client should send <name><cr><lf>
  // Read the name of the client character by character until a
  // <CR><LF> is found.
  //
    
  while ( nameLength < MaxName &&
	  ( n = read( fd, &newChar, sizeof(newChar) ) ) > 0 ) {
    //printf("%c",newChar);
    if ( lastChar == '\015' && newChar == '\012' && fourthChar == '\015' && thirdChar == '\012') {
      // Discard previous <CR> from name
      nameLength-=3;
      if(!gotPath) {
        strcpy(path,name);
	pathLength = nameLength;
      }
      break;
    }
    if(newChar == ' ') {
      if(!gotGet) {
        gotGet = 1;
        continue;
      }
      if(!gotPath) {
	strcpy(path, name);
        gotPath = 1;
      }
    }
    if(!gotGet) continue;
    if(count < strlen(key)) {
      keyCheck[count] = newChar;
      count++;
    }
    name[ nameLength ] = newChar;
    nameLength++;
    if(!gotPath) pathLength++;
    fourthChar = thirdChar;
    thirdChar = lastChar;
    lastChar = newChar;
  }
  // Add null character at the end of the string
  name[ nameLength ] = 0;
  path[pathLength] = 0;
  keyCheck[count] = 0;
  if(strcmp(key,keyCheck) != 0)
    return;
  char prevChar = 1;
  int i = 0;
  while(prevChar) {
    path[i] = path[i+strlen(key)];
    prevChar = path[i];
    i++;
  }
  //Set full path to temp
  char* temp = (char*)malloc(1025);
  if(strstr(path,"/icons") == path || strstr(path,"/htdocs") == path) {
    temp = strcat(cwd,"/http-root-dir");
    temp = strcat(temp,path);
  } else if (strcmp(path,"/") == 0) {
    temp = strcat(cwd,"/http-root-dir/htdocs/index.html");
  } else {
    temp = strcat(cwd,"/http-root-dir/htdocs");
    temp = strcat(temp,path);
  }
  printf("path=%s\n",path);
  if(strstr(path,"/cgi-bin") == path) {
    char* args = strstr(path,"?");
    int ret = fork();
    if (ret == 0) {
	setenv("REQUEST_METHOD","GET",1);
        if(args)
		setenv("QUERY_STRING",args,1);
//	dup2(fd,1);
        char* firstLine = "HTTP/1.1 200 Document follows\r\n";
        char* secondLine = "Server: cs252\r\n";
        write(fd,firstLine,strlen(firstLine));
	write(fd,secondLine,strlen(secondLine));
	char* gucciGang = (char*)path;
        char** pass = &gucciGang;
        *(pass+1) = NULL;
        int exec = execvp(pass[0],pass);
//        dup2(tempout,1);
    } else {
	waitpid(ret,0,0);
    }
    printf("done\n");
    return;
  }
  char sortTag = 0;
  char asc = 0;
  if(strstr(temp,"?sort=name")) {
    sortTag = 1;
    if (endsWith(temp,"ord=asc"))
      asc = 1;
    printf("asc? %d\n",asc);
    temp[(unsigned long)strstr(temp,"?sort=name")-(unsigned long)temp] = 0;
  }
  DIR *dir = opendir(temp);
  printf("HERE1. dir=%d\n",dir);
  //if path is a directory, read all files and create links to them
  if(dir != NULL) {
    struct dirent* curr = readdir(dir);
    printf("HERE2\n");
    char* firstLine = "HTTP/1.1 200 Document follows\r\n";
    write(fd,firstLine,strlen(firstLine));
    char* secondLine = "Server: CS252 lab5\r\n";
    write(fd,secondLine,strlen(secondLine));
    char* thirdLine = "Content-type: ";
    write(fd,thirdLine,strlen(thirdLine));
    write(fd,"text/html",strlen("text/html"));
    write(fd,"\r\n",2);
    write(fd,"\r\n",2);
    char * sorting;
    if(asc) sorting = "<a href=?sort=name&ord=dec>name</a>";
    else sorting = "<a href=?sort=name&ord=asc>name</a>";
    write(fd,sorting,strlen(sorting));
    char * ulStart = "<ul>";
    write(fd,ulStart,strlen(ulStart));
    char** names = (char**)malloc(1024*sizeof(char*));
    int count = 0;
    while(curr) {
        printf("HERE3\n");
        char* filename = strdup(curr->d_name);
        *(names+count) = strdup(curr->d_name);
        count++;
        if(!sortTag) {
          char* char1 = "<li><a href=\"";
          char* char2 = "\">";
          char* char3 = "</a></li>";
	  write(fd,char1,strlen(char1));
	  write(fd,filename,strlen(filename));
	  write(fd,char2,strlen(char2));
	  write(fd,filename,strlen(filename));
	  write(fd,char3,strlen(char3));
        }
        curr = readdir(dir);
    }
    if(sortTag) {
      int i = 0;
      for(;i<count-1;i++) {
        int j = 0;
        for(;j<count-1-i;j++) {
          if(asc){
	    if(strcmp(*(names+j),*(names+j+1)) > 0) {
	      char* temp = strdup(*(names+j));
	      names[j] = strdup(names[j+1]);
	      names[j+1] = temp;
	    }
          } else {
	    if(strcmp(*(names+j),*(names+j+1)) < 0) {
	      char* temp = strdup(*(names+j));
	      names[j] = strdup(names[j+1]);
	      names[j+1] = temp;
	    }
          }
        }
      }
      i = 0;
      while(i < count) {
        printf("HERE3\n");
        char* filename = names[i];
        char* char1 = "<li><a href=\"";
        char* char2 = "\">";
        char* char3 = "</a></li>";
	write(fd,char1,strlen(char1));
	write(fd,filename,strlen(filename));
	write(fd,char2,strlen(char2));
	write(fd,filename,strlen(filename));
	write(fd,char3,strlen(char3));
        i++;
      }
    }
    char* ulEnd = "</ul>";
    write(fd,ulEnd,strlen(ulEnd));
    int i = 0;
    for(;i<count;i++)
      printf("%s  ",*(names+i));
    printf("HERE5\n");
    return;
  }


  //Set content type
  char* contentType;
  if(endsWith(temp,".html")|| endsWith(temp,".html/"))
    contentType = "text/html";
  else if(endsWith(temp,".gif") || endsWith(temp,".gif/"))
    contentType = "image/gif";
  else if(endsWith(temp,".svg") || endsWith(temp,".svg/"))
    contentType = "image/svg+xml";
  else
    contentType = "text/plain";
  
  if(endsWith(temp,"stats.html") || endsWith(temp,"stats.html/")) {
    printf("cwd=%s\n",cwd);
    printf("count==%d\n",reqCount);
    time(&currTime);
    double timeElapsed = difftime(currTime,start);
    char* statsTemp = (char*)malloc(1024);
    strcat(statsTemp,cwd);
    //strcat(statsTemp,"stats.html");
    printf("statsTemp=%s\n",statsTemp);
    int tempFd = open(statsTemp, O_WRONLY | O_TRUNC | O_CREAT);
    if(tempFd < 0)
      printf("open error\n");
    char updateTemp[1024];
    char* update1 = "<h1>Aaron Althoff</h1><br/><h4>";
    char* update2 = "</h4><br/><h4>";
    char* update3 = "</h4>";
    sprintf(updateTemp, "%sRequest Count: %d%sElapsed Time: %f%s",update1,reqCount,update2,timeElapsed,update3);
    printf("updateTemp = %s\n",updateTemp);
    write(tempFd,updateTemp,strlen(updateTemp));
    if(write < 0)
      printf("write error\n");
    close(tempFd);
  }

  int fd2 = open(temp, O_RDONLY);
//printf("path=%s    contentType=%s    fd=%d\n",temp,contentType,fd2);
  if(fd2 < 0) {
    char* firstLine = "HTTP/1.1 404 File Not Found\n";
    char* secondLine = "Server: CS 252 Lab5\n";
    char* thirdLine = "Content-type: text/plain\n\n";
    char* errorMessage = "Could not find specified URL. Server returned an error.\n";
    write(fd,firstLine,strlen(firstLine));
    write(fd,secondLine,strlen(secondLine));
    write(fd,thirdLine,strlen(thirdLine));
    write(fd,errorMessage,strlen(errorMessage));
  }
  else {
    //send header to client
    char* firstLine = "HTTP/1.1 200 Document follows\r\n";
    write(fd,firstLine,strlen(firstLine));
    char* secondLine = "Server: CS252 lab5\r\n";
    write(fd,secondLine,strlen(secondLine));
    char* thirdLine = "Content-type: ";
    write(fd,thirdLine,strlen(thirdLine));
    write(fd,contentType,strlen(contentType));
    write(fd,"\r\n",2);
    write(fd,"\r\n",2);
    char readChar = 0;
    int count = 0;
    while(count = read(fd2, &readChar, sizeof(readChar)))
      if(write(fd,&readChar, sizeof(readChar)) != count) {
        //printf("write");
      }
    //write(fd2,"\n",1);
  }
  close(fd2);
  if(temp != cwd)
    free(temp);
  free(cwd);
}





