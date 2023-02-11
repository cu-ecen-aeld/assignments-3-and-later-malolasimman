CFlags= -g -Wall
CROSS_COMPILE=
CC= gcc
.PHONY : all

all: writer

writer: writer-o
	 $(CROSS_COMPILE)$(CC) $(CFlags) -o writer writer.o

writer-o: 
	$(CROSS_COMPILE)$(CC) $(CFlags) -c writer.c -o writer.o


.PHONY:clean
clean:
	rm -f *.o writer


