CFlags= -g -Wall
CROSS_COMPILE=$(something)
.PHONY : all

all: writer

writer: writer-o
	 $(CROSS_COMPILE)gcc $(CFlags) -o writer writer.o

writer-o: 
	$(CROSS_COMPILE)gcc $(CFlags) -c writer.c -o writer.o

CROSS_COMPILE: cross_compile_writer-o
	 $(CROSS_COMPILE)gcc $(CFlags) -o writer writer.o

cross_compile_writer-o:
	   $(CROSS_COMPILE)gcc $(CFlags) -c writer.c -o writer.o


.PHONY:clean
clean:
	rm -f *.o writer


