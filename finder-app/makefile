CC= $(CROSS_COMPILE)gcc
CFlags= -g -Wall -Werror -Wextra
.PHONY : all

all: writer

writer: writer-o
	 $(CC) $(CFlags) -o writer writer.o

writer-o: 
	$(CC) $(CFlags) -c writer.c -o writer.o


.PHONY:clean
clean:
	rm -f *.o writer


