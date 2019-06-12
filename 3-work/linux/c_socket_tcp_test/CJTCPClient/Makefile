CC=gcc
CFLAG= -g

all:
	$(CC) $(CFLAG) -c *.c 
	$(CC) -rdynamic *.o -o BindClient.exe  -lpthread

clean:
	rm *.o
	rm *.exe