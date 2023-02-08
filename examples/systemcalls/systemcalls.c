/*Author : Malola Simman Srinivasan Kannan
 *Mail id: masr4788@colorado.edu
 *File name: systemcall.c
 *Date : 4 Februrary 2024	 
 */
#include "systemcalls.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
	int result=system(cmd); //calling system call 
	if(result == 0)		//checking system call is successfull
	{
		return true;	//return true if system call is successfull
	}
	else
       	{
		return false;   //return false if fails to execute system call
	}
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/ 
    int res;
    pid_t pid;
    pid=fork(); // creating child process
    if(pid == -1)//checking whether fork is failed
    {
    	perror("fork");//print error and exit
	exit (-1);
    }
    else if(pid == 0)//checking child is created
    {
	    int ret_val=execv(command[0],command);//replaces the current process with a new process 
	    if(ret_val == -1) //checks execv fails
	    {
	   	perror("execv");//print error and exit
		exit(-1);
	    }
    }
    else{
   	 pid_t id = waitpid(pid,&res,0);//wait until child process completes
         if(id == -1)//checks whether waitpid fails
	 {
                perror("waitpid");//print waitpid fails and exit
		exit(-1);
         }
	 if(WIFEXITED(res))//returns true if the child terminated normally
	 {
	 	if(WEXITSTATUS(res)!=0)//returns the exit status of the child.
		{
			return false; //if WEXITSTATUS fails return false
		}
	 }
    }
    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    int res;    
    pid_t pid;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ;
    int fd =  open(outputfile, O_WRONLY|O_CREAT|O_TRUNC, mode );
     if(fd == -1)
     {
	     perror("open");
             abort();
     }
    pid=fork();
    if(pid == -1){
        perror("fork");
        exit (-1);
    }
    else if(pid == 0){
	    if (dup2(fd, 1) < 0)
	    {
		    perror("dup2");
		    abort();
	    }
            int ret_val=execv(command[0],command);
            if(ret_val == -1){
                perror("execv");
                exit(-1);
            }
    }
    else{
   	 pid_t id = waitpid(pid,&res,0);
         if(id == -1){
                perror("waitpid");
                return false;
	 }
         if(WIFEXITED(res)){
                if(WEXITSTATUS(res)!=0){
                        return false;
                }
         }
    }
    close(fd);
    va_end(args);

    return true;
}
