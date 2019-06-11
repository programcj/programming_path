CC=gcc
CFLAG=-g

sample-list:
	$(CC) $(CFLAG) sample-list.c ../public_tools/list_queue.c -lpthread

sample-tools:
	$(CC) $(CFLAG) sample-tools.c ../public_tools/tools.c -lpthread 

sample-clog: libptools
	$(CC) $(CFLAG) sample-clog.c -o sample-clog.exe -L../public_tools -lptools -lcurl -Wl,-rpath,../public_tools

clean:
	rm *.out
	rm *.exe

libptools:
	cd ../public_tools; make all
	