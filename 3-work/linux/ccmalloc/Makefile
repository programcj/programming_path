SRC:= main.c mem.c ../public_tools/clog.c
CC= gcc

all:

	$(CC) -g $(SRC) -o test_malloc -Wl,-wrap=malloc  -Wl,-wrap=free -lpthread

clean:
	rm test_malloc
	