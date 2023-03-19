/* Author : Malola Simman Srinivasan Kannan
 *  Date : 26 February 2023
 *  mail id : masr4788@colorado.edu
 *  file name : aesdsocket.c
 */
//Headers
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/queue.h>

//Macros
#define buffer_size 1024
#define TIMESTAMP_LEN (128)

//#define SOCKET_PATH "/var/tmp/aesdsocketdata"
#if (USE_AESD_CHAR_DEVICE == 1)
const char *SOCKET_PATH = "/dev/aesdchar";
#elif (USE_AESD_CHAR_DEVICE == 0)
const char *SOCKET_PATH = "/var/tmp/aesdsocketdata";
#endif
/*
#define USE_AESD_CHAR_DEVICE    1
#if (USE_AESD_CHAR_DEVICE)
const char *SOCKET_PATH = "/dev/aesdchar";
#elif (!USE_AESD_CHAR_DEVICE)
const char *SOCKET_PATH = "/var/tmp/aesdsocketdata";
#endif
*/
//function declarations
void cleanup();
SLIST_HEAD(client_list_head_t, thread_args);

//Global variables
struct client_list_head_t client_head;
int sockfd; //socket file descriptor
struct addrinfo hints, *servinfo, *p; // address info structs
struct sockaddr_in conn_Addr; // socket address struct
int accept_rc; // accept return value
char *ptr = NULL; // pointer to hold dynamic memory
int fd=0;//file_descriptor
ssize_t rec_fd=1;
int res =0;
volatile int sig_flag = 0 ;
pthread_mutex_t lock;
socklen_t size=sizeof(struct sockaddr);
// size
struct thread_args
{
  struct sockaddr_in IP_addr;
  socklen_t client_addr_size;
  pthread_t thid;
  int accept_fd ;
  SLIST_ENTRY(thread_args) ll_entries;
};
typedef struct thread_args thread_param;
thread_param *th_data=NULL;

// signal handler function prototype
void signal_handler(int sig)
{
  if(sig==SIGINT)
    {
      sig_flag = 1;
      syslog(LOG_INFO," SIGINT occurred");
    }
  else if(sig==SIGTERM)
    {
      sig_flag = 1;
      syslog(LOG_INFO," SIGTERM occurred");
    }
  unlink("/var/tmp/aesdsocketdata"); //Deletes a file
  //Close socket and client connection
  cleanup();
}

//initializing thread param
thread_param* init_thread_param(){
  th_data = (thread_param*)malloc(sizeof(thread_param));
  th_data->IP_addr = conn_Addr;
  th_data->client_addr_size = size;
  th_data->accept_fd = accept_rc;
  SLIST_INSERT_HEAD(&client_head, th_data , ll_entries);
  return (th_data);
}
//thread function
void* thread_func(void *arg)
{
  ptr = (char*)malloc(1);
      if(ptr==NULL)
        {
          perror("malloc");
          cleanup();
          exit(14);
        }
  printf("1\n");
  printf("enters thread\n");
  ssize_t wr_bytes=0;
  int total_packet_size=0;
  char buffer[buffer_size];
  int send_rc=0;
  int i=0;
  int bytes_to_write=0;
  bool write_flag=false;
  rec_fd=recv(accept_rc,buffer,buffer_size,0);
  if(rec_fd==-1)
    {
      free(ptr);
      syslog(LOG_ERR, "Error in receiving of data packets from client");
      perror("recv");
      cleanup();
      return NULL;
      //exit(6);
    }
  while( (rec_fd!=0) && (!sig_flag) ) // receiving packets
  {
      for(i=0;i<buffer_size;i++)
        {
          if(sig_flag){
              break;
          }
          if(buffer[i]=='\n')//checking for newline
            {
              i++;
              write_flag=true;
              break;
            }
        }
      bytes_to_write+=i;
      //mutex
      ptr=(char *)realloc(ptr,bytes_to_write+1);//reallocating memory
      if(ptr==NULL)
        {
          free(ptr);
          perror("realloc");
          return NULL;
        }
      memcpy(ptr+bytes_to_write-i,buffer,i);// copying the buffer to pointer
      memset(buffer,0,buffer_size);// clearing buffer
      if(write_flag==true)
        {
          pthread_mutex_lock(&lock);
          wr_bytes= write(fd,ptr,bytes_to_write); //write packets contents to file
          pthread_mutex_unlock(&lock);
          if(wr_bytes!=bytes_to_write)
            {
              free(ptr);
              perror("write");
              return NULL;
            }
          write_flag=false;
          total_packet_size += bytes_to_write;
          char *read_arr;
          pthread_mutex_lock(&lock);
          lseek(fd, 0, SEEK_SET);
          int index = 0;
          char ch;
          for (; read(fd,&ch,1) > 0; index++);
          lseek(fd, 0, SEEK_SET);
          read_arr = (char*) malloc(sizeof(char)* index);
          int rd_status=read(fd, read_arr,index);//reading packet data
          pthread_mutex_unlock(&lock);
          if(rd_status==-1)
            {
              free(ptr);
              syslog(LOG_ERR, "fail to read");
              perror("read");
              return NULL;
            }
          else if(rd_status < total_packet_size)
            {
              free(ptr);
              syslog(LOG_ERR, "Not read complete bytes");
              return NULL;
            }
          for(int j=0; j<total_packet_size; j++){
              printf("%c",read_arr[j]);
          }
          send_rc=send(accept_rc,read_arr,index,0);//sending packet to client
          free(read_arr);
        
          if(send_rc==-1)
            {
              free(ptr);
              syslog(LOG_ERR, "fail to send packets");
              perror("send");
              return NULL;
            }
          
          bytes_to_write =0;
        }
      rec_fd=recv(accept_rc,buffer,buffer_size,0);//receiving next packets
      if(rec_fd==-1)
        {
          free(ptr);
          perror("recv");
          return NULL;
        }
    }
    free(ptr);
  return NULL;
}
void * log_timestamp()
{
  time_t curr_time;                 // stores the current time
  struct tm * curr_localtime;       // stores the local time representation of the current time
  char timestamp[TIMESTAMP_LEN];    // stores the formatted timestamp string

  sleep(10);                        // sleep for 10 sec and allow preemption

  // continue logging timestamps until the sig_flag is set to a non-zero value
  while (!sig_flag)
    {
      // get the current time and convert it to local time
      time(&curr_time);
      curr_localtime = localtime(&curr_time);

      // format the local time as a string and store it in the timestamp buffer
      strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %T %z\n", curr_localtime);

      // acquire a lock on the mutex to protect access to the shared file descriptor
      pthread_mutex_lock(&lock);

      // seek to the end of the file and append the timestamp
      lseek(fd, 0, SEEK_END);
      write(fd, timestamp, strlen(timestamp));

      // release the lock on the mutex
      pthread_mutex_unlock(&lock);

      // sleep for 10 seconds to allow preemption
      sleep(10);
    }

  // return NULL
  return NULL;
}
//Main functions
int main(int argc, char *argv[])
{
  // variable initialization
  int addr_fd=0;
  int bind_rc=0;
  int listen_rc=0;
  int value = 1;
  //int total_packet_size=0;
  fd = open(SOCKET_PATH, (O_CREAT | O_APPEND | O_RDWR), 0744);
  if(fd == -1)
    {
      perror("file open failed\n");
      syslog(LOG_ERR,"file open failed");
      cleanup();
      return (EXIT_FAILURE);
    }
  openlog(NULL, LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER); /*open connection for sys logging, ident is NULL to use this Program for the user level messages*/
  if((argc>1) && strcmp(argv[1],"-d")==0)// check for daemon mode flag
    {
      if(daemon(0,0)==-1)// set to daemon mode
        {
          syslog(LOG_ERR, "daemon mode failed");
          cleanup();
          exit(1);
        }
    }
  if(signal(SIGINT,signal_handler)==SIG_ERR)// handle SIGINT signal
    {
      syslog(LOG_ERR,"SIGINT failed");
      cleanup();
      exit(2);
    }
  if(signal(SIGTERM,signal_handler)==SIG_ERR)
    {
      syslog(LOG_ERR,"SIGTERM failed");// handle SIGTERM signal
      cleanup();
      exit(3);
    }
  SLIST_INIT(&client_head);
  pthread_mutex_init(&lock,NULL);
  //create socket
  sockfd=socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd==-1)
    {
      syslog(LOG_ERR, "fail to create socket");
      perror("socket");
      cleanup();
      exit(1);
    }
  // Set socket options for reusing address and port
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &value, sizeof(int)))
    {
      printf("Error: %s : Failed to set socket options\n", strerror(errno));
      syslog(LOG_ERR, "Error: %s : Failed to set socket options\n", strerror(errno));
      cleanup();
      return -1;
    }
  //Get address
  hints.ai_flags=AI_PASSIVE;// set socket options for passive socket
  addr_fd=getaddrinfo(NULL,"9000",&hints,&servinfo);// get address info for the socket
  if(addr_fd !=0)
    {
      syslog(LOG_ERR, "fail to get server address");
      perror("getaddrinfo");
      cleanup();
      exit(2);
    }
  //Bind
  bind_rc=bind(sockfd,servinfo->ai_addr,sizeof(struct sockaddr));// bind the socket to an address
  if(bind_rc==-1)
    {
      freeaddrinfo(servinfo);
      syslog(LOG_ERR, "fails to bind");
      perror("bind");
      cleanup();
      exit(3);
    }
  freeaddrinfo(servinfo);// free memory allocated for servinfo struct
  
  //Listen
  listen_rc=listen(sockfd,10);
  if(listen_rc==-1)
    {
      syslog(LOG_ERR, "failed to listen connection");
      perror("listen");
      cleanup();
      exit(4);
    }
  pthread_t timestamp;
  pthread_create(&timestamp, NULL, log_timestamp, NULL);
  while(!sig_flag)
    {
      //accept
      accept_rc=accept(sockfd,(struct sockaddr *)&conn_Addr,&size);
      if(accept_rc==-1)
        {
          syslog(LOG_ERR, "fail to accept");
          perror("accept");
          cleanup();
          exit(6);
        }
      else
        {
          syslog(LOG_INFO,"Accepts connection from %s",inet_ntoa(conn_Addr.sin_addr));
        }
      
      th_data = init_thread_param();
      pthread_mutex_lock(&lock);
      res = pthread_create(&(th_data->thid),NULL,thread_func,(void*)th_data);
      pthread_mutex_unlock(&lock);
      if(res < 0)
        {
          syslog(LOG_ERR, "Thread Creation failed");
          SLIST_REMOVE(&client_head, th_data, thread_args, ll_entries);
          close(fd);
          free(th_data);
        }
      else
        {
          pthread_join(th_data->thid, NULL);
        }
      syslog(LOG_INFO,"Closing connection from %s",inet_ntoa(conn_Addr.sin_addr));
      closelog();
    }
  cleanup();
  return 0;
}


void cleanup()
{
  // close open file descriptors
  close(fd);
  close(sockfd);
  close(accept_rc);

  // free allocated memory for each client thread data structure
  SLIST_FOREACH(th_data, &client_head, ll_entries)
  {
    if (th_data)
      free(th_data);
  }

  // free memory for the head of the client thread data structure
  while (!SLIST_EMPTY(&client_head))
  {
    th_data = SLIST_FIRST(&client_head);
    SLIST_REMOVE_HEAD(&client_head, ll_entries);
    free(th_data);
  }

  // initialize the client thread data structure head
  SLIST_INIT(&client_head);

  // destroy the mutex lock
  pthread_mutex_destroy(&lock);

  // remove the socket file
  if (unlink(SOCKET_PATH) == -1) {
    perror("Failed to remove socket file");
  }

  // close the system log
  closelog();
}
