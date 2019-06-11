CC= gcc
CFLAGS = -g -I/home/cc/ffmpeg/install/include
LDFLAGS = -L/home/cc/ffmpeg/install/lib
LDFLAGS += -lavfilter -lavcodec -lavdevice  -lswscale -lswresample -lavformat -lavutil 
LDFLAGS += -lpthread -lm -lz

all:
	$(CC) $(CFLAGS) readjpg.c $(LDFLAGS)
	$(CC) $(CFLAGS) jpg_crop.c $(LDFLAGS) -o jpg_crop.exe
	
clean:
	rm *.out
	rm *.exe