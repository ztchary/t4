CC=cc
CFLAGS=-Wall -Werror

t4: t4.c
	$(CC) $(CFLAGS) -o t4 t4.c

