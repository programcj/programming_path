
CC := gcc

#CC := arm-hisiv300-linux-gcc

SRC_FILE = src/clog.c \
	src/mem.c \
	src/sockets.c \
	src/main.c

all:
	$(CC)	 $(SRC_FILE) -o server -lpthread
	
clean:
	rm server