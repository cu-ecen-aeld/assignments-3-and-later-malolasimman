#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
int main (int argc, char*argv[])
{
	openlog(NULL,0,LOG_USER);
	char *writefile =argv[1];
	char *writestr =argv[2];
	char *file_name=basename(argv[1]);
	size_t fw ; 
	if(argc==3)
	{
		int fd;
		mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
	        fd = open(writefile, O_RDWR | O_NONBLOCK | O_TRUNC | O_CREAT, mode);
		if(fd == -1)
			syslog(LOG_ERR,"Failed to open file in %s\n",writefile);
		else
			syslog(LOG_DEBUG,"successfully opened %s\n",writefile);
		
	
		fw=write(fd,writestr,strlen(writestr));
		if(fw == -1)
			syslog(LOG_ERR,"Failed to write string %s\n",writestr);
		else{
			syslog(LOG_DEBUG,"Writing %s to %s\n",writestr,file_name);
			if (fw != strlen(writestr))
				syslog(LOG_ERR,"Failed to write entire string\n");
			if(fw == strlen(writestr))
				syslog(LOG_DEBUG,"successfully written the string %s to %s\n",writestr,file_name);
			close(fd);
		}
	}
	else
       	{
		syslog(LOG_ERR,"Invalid Number of arguments: %d\n",argc-1);
		return -1;
	}
	
	closelog();
	return 0;

}

