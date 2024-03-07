CFLAGS=-Wextra -Wall -Werror -ggdb

all:
	cc -o main main.c $(CFLAGS) -I./cutils
