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

//Global variables
int sockfd; //socket file descriptor
struct addrinfo hints, *servinfo, *p; // address info structs
struct sockaddr_in conn_Addr; // socket address struct
int accept_rc; // accept return value
char *ptr; // pointer to hold dynamic memory

// signal handler function prototype
void signal_handler(int sig)
{
	if(sig==SIGINT)
	{
		syslog(LOG_INFO," SIGINT occurred");
	}
	else if(sig==SIGTERM)
	{
		syslog(LOG_INFO," SIGTERM occurred");
	}
	
	unlink("/var/tmp/aesdsocketdata"); //Deletes a file
	
	//Close socket and client connection
	close(sockfd);
	close(accept_rc);
	
	exit(0); //Exit success 
}

//Main functions
int main(int argc, char *argv[])
{
        // variable initialization
        int addr_fd=0; 
        int bind_rc=0;
        int listen_rc=0;
        char buffer[1024];
        int fd=0;
        int send_rc=0;
        int i=0;
        int bytes_to_write=0;
        ssize_t rec_fd=1;
        ssize_t wr_bytes=0;
        socklen_t size=sizeof(struct sockaddr);
        bool write_flag=false;
        int total_packet_size=0;

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
        while(1)
        {

                //Listen
                listen_rc=listen(sockfd,10);
                if(listen_rc==-1)
                {
                        syslog(LOG_ERR, "failed to listen connection");
                        perror("listen");
                        exit(4);
                }

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
                rec_fd=recv(accept_rc,buffer,1024,0);
                if(rec_fd==-1)
                {
                        syslog(LOG_ERR, "Error in receiving of data packets from client");
                        perror("recv");
                        exit(6);
                }


                ptr = (char*)malloc(1);
                if(ptr==NULL)
                {
                        perror("malloc");
                        exit(14);
                }

                while(rec_fd!=0) // receiving packets
                {

                        for(i=0;i<1024;i++)
                        {
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
                                exit(8);
                        }
                        memcpy(ptr+bytes_to_write-i,buffer,i);// copying the buffer to pointer
                        memset(buffer,0,1024);// clearing buffer


                        if(write_flag==true)
                        {

                                wr_bytes= write(fd,ptr,bytes_to_write); //write packets contents to file
                                if(wr_bytes!=bytes_to_write)
                                {
                                perror("write");
                                exit(8);
                                }

                                close(fd);
                                write_flag=false;
                                total_packet_size += bytes_to_write;
                                int op_fd=open("/var/tmp/aesdsocketdata",O_RDONLY);//opening file
                                if(op_fd==-1)
                                {
                                        syslog(LOG_ERR, "fail to open");
                                        perror("open");
                                        exit(10);
                                }
                                char read_arr[total_packet_size];
                                memset(read_arr,0,bytes_to_write);
                                int rd_status=read(op_fd,&read_arr,total_packet_size);//reading packet data
                                if(rd_status==-1)
                                {
                                        syslog(LOG_ERR, "fail to read");
                                        perror("read");
                                        exit(11);
                                }
                                else if(rd_status < total_packet_size) 
                                {
                                        syslog(LOG_ERR, "Not read complete bytes");
                                        exit(17);
                                }
                                for(int j=0; j<total_packet_size; j++){
                                        printf("%c",read_arr[j]);
                                }
                                send_rc=send(accept_rc,&read_arr,total_packet_size,0);//sending packet to client 
                                if(send_rc==-1)
                                {
                                        syslog(LOG_ERR, "fail to send packets");
                                        perror("send");
                                        exit(12);
                                }

                                close(op_fd); //closing file

                                bytes_to_write =0;
                        }
                        rec_fd=recv(accept_rc,buffer,1024,0);//receiving next packets
                        if(rec_fd==-1)
                        {
                                perror("recv");
                                exit(6);
                        }
                }
        free(ptr);
        closelog();
        }
        return 0;
}


