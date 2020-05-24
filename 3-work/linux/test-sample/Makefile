CC=gcc
CFLAG=-g 
LDFLAGS = -lpthread -L../public_tools -lptools -Wl,-rpath,../public_tools

sample-list:
	$(CC) $(CFLAG) sample-list.c ../public_tools/list_queue.c -lpthread

sample-tools:
	$(CC) $(CFLAG) sample-tools.c ../public_tools/tools.c -lpthread 

sample-clog: libptools
	$(CC) $(CFLAG) sample-clog.c -o sample-clog.exe -L../public_tools -lptools -lcurl -Wl,-rpath,../public_tools

sample-carray: libptools
	$(CC) $(CFLAG) sample-carray.c -o sample-carray.exe $(LDFLAGS)
	
test_proxy:
	$(CC) $(CFLAG) test_proxy.c -o test_proxy.exe $(LDFLAGS)
	
clean:
	rm *.out
	rm *.exe

libptools:
	cd ../public_tools; make all
	