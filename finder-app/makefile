.PHONY : all

all: writer

writer: writer-o
	gcc -o writer writer.o

writer-o: 
	gcc -c writer.c -o writer.o

CROSS_COMPILE: cross_compile_writer-o
	aarch64-none-linux-gnu-gcc -o writer writer.o

cross_compile_writer-o:
	aarch64-none-linux-gnu-gcc -c writer.c -o writer.o


.PHONY:clean
clean:
	rm -f *.o writer


