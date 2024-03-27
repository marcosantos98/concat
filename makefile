CFLAGS=-Wextra -Wall -Werror 
DEBUG_CFLAGS=-ggdb -fsanitize=address

all:
	cc -o main main.c tokenizer.c parser.c $(CFLAGS) -I./cutils

debug:
	cc -o main main.c $(CFLAGS) $(DEBUG_CFLAGS) -I./cutils
