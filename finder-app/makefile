CC= $(CROSS_COMPILE)gcc
CFlags= -Wall -Werror
.PHONY : all

all:writer.c
	$(CC) writer.c $(CFlags) -o writer
	 
.PHONY:clean
clean:
	rm -f *.o writer


