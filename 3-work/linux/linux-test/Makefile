
CC := gcc
SRC = uvc_test.c

uvc_test: uvc_test.c
	$(CC) -g uvc_test.c -o uvc_test -lv4l2 -lpthread
	
all: 
	$(CC) -fPIC -c ../public_tools/clog.c -o clog.o
	#ar rcs libclog.a clog.o
	$(CC) -fPIC -shared  -o libclog.so clog.o
	$(CC) -g clog_test.c -L./ -lclog -lpthread -o clog_test
	
clean:
	rm *.a
	rm *.so
	rm uvc_test
	