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
#include "freequeue.h"

#define buffer_size 1024

//Global variables
int sockfd; //socket file descriptor
struct addrinfo hints, *servinfo, *p; // address info structs
struct sockaddr_in conn_Addr; // socket address struct
int accept_rc; // accept return value
char *ptr = NULL; // pointer to hold dynamic memory
int fd=0;//file_descriptor
ssize_t rec_fd=1;
static int flag =0; 
volatile int sig_flag = 0 ; 
pthread_mutex_t lock;

typedef struct thread_args{
        pthread_t thid;
        int Accept_rc;
        int File_rc;
        SLIST_ENTRY(thread_args) ll_entries;
        
}thread_args;


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
	close(sockfd);
	close(accept_rc);
	
	exit(0); //Exit success 
}
void init_thread_param(thread_args *data){
        data->Accept_rc=accept_rc;
        //data->client_addr=conn_Addr;
        data->File_rc=fd;
        return;
}

void *thread_func(void *arg)
{
        printf("22\n");
        thread_args *th = (thread_args *)arg; 
        printf("22\n");
	ssize_t wr_bytes=0;
        int total_packet_size=0;
        char buffer[buffer_size];
        int send_rc=0;
        int i=0;
        int bytes_to_write=0;
         bool write_flag=false;
        rec_fd=recv(th->Accept_rc,buffer,buffer_size,0);
        if(rec_fd==-1)
        {
                syslog(LOG_ERR, "Error in receiving of data packets from client");
                perror("recv");
                pthread_exit(NULL);
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
                ptr=(char *)realloc(ptr,bytes_to_write+1);//reallocating memory
                if(ptr==NULL)
                {
                        perror("realloc");
                        pthread_exit(NULL);
                        //exit(8);
                }
                memcpy(ptr+bytes_to_write-i,buffer,i);// copying the buffer to pointer
                memset(buffer,0,buffer_size);// clearing buffer

                if(write_flag==true)
                {
                        pthread_mutex_lock(&lock);
                        wr_bytes= write(th->File_rc,ptr,bytes_to_write); //write packets contents to file
                        pthread_mutex_unlock(&lock);
                        if(wr_bytes!=bytes_to_write)
                        {
                                perror("write");
                                pthread_exit(NULL);
                                //exit(8);
                        }

                        close(fd);
                        write_flag=false;
                        total_packet_size += bytes_to_write;
                        int op_fd=open("/var/tmp/aesdsocketdata",O_RDONLY);//opening file
                        if(op_fd==-1)
                        {
                                syslog(LOG_ERR, "fail to open");
                                perror("open");
                                pthread_exit(NULL);
                                //exit(10);
                        }
                        char read_arr[total_packet_size];
                        memset(read_arr,0,bytes_to_write);
                        int rd_status=read(op_fd,&read_arr,total_packet_size);//reading packet data
                        if(rd_status==-1)
                        {
                                syslog(LOG_ERR, "fail to read");
                                perror("read");
                                pthread_exit(NULL);
                                //exit(11);
                        }
                        else if(rd_status < total_packet_size) 
                        {
                                syslog(LOG_ERR, "Not read complete bytes");
                                pthread_exit(NULL);
                                //exit(17);
                        }
                        for(int j=0; j<total_packet_size; j++){
                                printf("%c",read_arr[j]);
                        }
                        send_rc=send(th->Accept_rc,&read_arr,total_packet_size,0);//sending packet to client 
                        if(send_rc==-1)
                        {
                                syslog(LOG_ERR, "fail to send packets");
                                perror("send");
                                pthread_exit(NULL);
                                //exit(12);
                                
                        }

                        close(op_fd); //closing file

                        bytes_to_write =0;
                }
                rec_fd=recv(th->Accept_rc,buffer,buffer_size,0);//receiving next packets
                if(rec_fd==-1)
                {
                        perror("recv");
                        pthread_exit(NULL);
                        //exit(6);
                }
        }
        free(th);
        return NULL;
}


void linkedlist_thread_entry() {
    static int entry_flag = 1;
    thread_args *curr_thread = NULL;
    thread_args *thread_ptr = NULL;
    thread_ptr = malloc(sizeof(thread_args));
    if (thread_ptr == NULL) {
        // handle error
    }
    thread_ptr->thid = pthread_self();
    // set other fields as needed

    SLIST_HEAD(slisthead, thread_args) head = SLIST_HEAD_INITIALIZER(head);
    SLIST_INIT(&head);
    if (entry_flag == 1) {
        SLIST_INIT(&head);
        SLIST_INSERT_HEAD(&head, thread_ptr, ll_entries);
        entry_flag = 0;
    } else {
        SLIST_INSERT_HEAD(&head, thread_ptr, ll_entries);
    }
    pthread_mutex_lock(&lock);
    for (curr_thread = SLIST_FIRST(&head); curr_thread != NULL; ) {
        if (flag) {
            syslog((LOG_INFO), "Killing connection thread");
            pthread_join(curr_thread->thid, NULL);
            SLIST_REMOVE(&head, curr_thread, thread_args, ll_entries);
            free(curr_thread);
            curr_thread = SLIST_FIRST(&head); // start over from the beginning
        } else {
            curr_thread = SLIST_NEXT(curr_thread, ll_entries);
        }
    }
    pthread_mutex_unlock(&lock);
}


//Main functions
int main(int argc, char *argv[])
{
        // variable initialization
        int addr_fd=0; 
        int bind_rc=0;
        int listen_rc=0;
       
        socklen_t size=sizeof(struct sockaddr);
       
        //int total_packet_size=0;

        openlog(NULL,LOG_PID, LOG_USER); // initialize syslog
        if((argc>1) && strcmp(argv[1],"-d")==0)// check for daemon mode flag
        {
                if(daemon(0,0)==-1)// set to daemon mode
                {
                        syslog(LOG_ERR, "daemon mode failed");
                        exit(1);
                }
        }

        if(signal(SIGINT,signal_handler)==SIG_ERR)// handle SIGINT signal
        {
                syslog(LOG_ERR,"SIGINT failed");
                exit(2);
        }
        if(signal(SIGTERM,signal_handler)==SIG_ERR)
        {
                syslog(LOG_ERR,"SIGTERM failed");// handle SIGTERM signal
                exit(3);
        }

        //create socket
        sockfd=socket(PF_INET, SOCK_STREAM, 0);
        if(sockfd==-1)
        {
                syslog(LOG_ERR, "fail to create socket");
                perror("socket");
                exit(1);
        }

        //Get address
        hints.ai_flags=AI_PASSIVE;// set socket options for passive socket
        addr_fd=getaddrinfo(NULL,"9000",&hints,&servinfo);// get address info for the socket
        if(addr_fd !=0)
        {
                syslog(LOG_ERR, "fail to get server address");
                perror("getaddrinfo");
                exit(2);
        }

        //Bind
        bind_rc=bind(sockfd,servinfo->ai_addr,sizeof(struct sockaddr));// bind the socket to an address
        if(bind_rc==-1)
        {
                freeaddrinfo(servinfo);
                syslog(LOG_ERR, "fails to bind");
                perror("bind");
                exit(3);
        }

        freeaddrinfo(servinfo);// free memory allocated for servinfo struct
        fd=open("/var/tmp/aesdsocketdata", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO); //To open/create a file and give permissions to all
        if (fd==-1)
        {
                perror("open");
                exit(7);
        }
        close(fd);// close the file to create it
        //Listen
        listen_rc=listen(sockfd,10);
        if(listen_rc==-1)
        {
                syslog(LOG_ERR, "failed to listen connection");
                perror("listen");
                exit(4);
        }
	printf("11\n");
        pthread_mutex_init(&lock,NULL);
        while(!sig_flag)
        {                
                //accept
                accept_rc=accept(sockfd,(struct sockaddr *)&conn_Addr,&size);
                if(accept_rc==-1)
                {
                        syslog(LOG_ERR, "fail to accept");
                        perror("accept");
                        exit(6);
                }
                else
                {
                        syslog(LOG_INFO,"Accepts connection from %s",inet_ntoa(conn_Addr.sin_addr));
                }


                fd = open("/var/tmp/aesdsocketdata", O_APPEND | O_WRONLY); 
                
                ptr = (char*)malloc(1);
                if(ptr==NULL)
                {
                        perror("malloc");
                        exit(14);
                }
                printf("1\n");
		thread_args *th_data=NULL;
                init_thread_param(th_data);
                flag=0;
                pthread_create(&(th_data->thid),NULL,thread_func,th_data);
                flag=1;
		printf("2\n");
                linkedlist_thread_entry();
		printf("3\n");
                syslog(LOG_INFO,"Closing connection from %s",inet_ntoa(conn_Addr.sin_addr));
                free(ptr);
                closelog();
        }
        return 0;
}



