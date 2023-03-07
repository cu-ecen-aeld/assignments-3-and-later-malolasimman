CC= $(CROSS_COMPILE)gcc
CFlags= -Wall -Werror -g -O0
.PHONY : all

all:aesdsocket.c
	$(CC) aesdsocket.c $(CFlags)  -o aesdsocket -lpthread
	 
.PHONY:clean
clean:
	rm -f *.o aesdsocket
