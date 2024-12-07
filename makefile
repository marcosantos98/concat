CFLAGS=-Wextra -Wall -Werror 
DEBUG_CFLAGS=-ggdb -fsanitize=address -DDEBUG

all:
	cc -o main main.c $(CFLAGS) -I./cutils

debug:
	cc -o main main.c $(CFLAGS) $(DEBUG_CFLAGS) -I./cutils

release:
	cc -O3 -o main main.c -I./cutils
