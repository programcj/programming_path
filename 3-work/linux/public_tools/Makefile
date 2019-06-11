CC = gcc
CFLAG = -L./

all:
	$(CC) $(CFLAG) -fPIC -shared *.c -o libptools.so -lpthread -lm 

clean:
	rm *.so
		