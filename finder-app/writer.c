/* File name    : writer.c
 * Author name  : Malola Simman Srinivasan Kannan
 * Mail_Id	: masr4788@colorado.edu
 */

//Headers
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>

//Main function
int main (int argc, char*argv[])
{
	openlog(NULL,0,LOG_USER);			//opening log
	char *writefile =argv[1];			//storing file path
	char *writestr =argv[2];			//storing string to be written on the file
	char *file_name=basename(argv[1]);		//storing file name
	size_t fw ; 					//Total no.of.characters written using write syscall
	if(argc==3)					//checking whether number of arguments ar valid
	{
		int fd;					//file descriptor
		mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;//selecting modes
	        fd = open(writefile, O_RDWR | O_NONBLOCK | O_TRUNC | O_CREAT, mode);    //opening file using open syscall
		if(fd == -1)								//checking whether file is opened successfully								
			syslog(LOG_ERR,"Failed to open file in %s\n",writefile);
		else
			syslog(LOG_DEBUG,"successfully opened %s\n",writefile);
		
	
		fw=write(fd,writestr,strlen(writestr));					//writing string to file using write syscall
		if(fw == -1)								//checks whether write syscall failed
			syslog(LOG_ERR,"Failed to write string %s\n",writestr);
		else{
			syslog(LOG_DEBUG,"Writing %s to %s\n",writestr,file_name);
			if (fw != strlen(writestr))					//checks whether entire string is written to the file
				syslog(LOG_ERR,"Failed to write entire string\n");
			if(fw == strlen(writestr))
				syslog(LOG_DEBUG,"successfully written the string %s to %s\n",writestr,file_name);
			close(fd);							//closes file descriptor
		}
	}
	else
       	{
		syslog(LOG_ERR,"Invalid Number of arguments: %d\n",argc-1);		//Invalid arguments 
		return 1;
	}
	
	closelog();									//closes log
	return 0;

}

