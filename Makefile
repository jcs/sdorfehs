CC = gcc

SRC = main.o events.o manage.o list.o bar.o

LIBS = -lX11

LDFLAGS = -L/usr/X11R6/lib
CFLAGS = -g -Wall -I/usr/X11R6/include 

DEBUG = -DDEBUG

ratpoison: $(SRC)
	gcc $(SRC) -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) 

%.o : %.c ratpoison.h conf.h
	$(CC) -c $(CFLAGS) $(DEBUG) $< -o $@

clean :
	rm -f *.o ratpoison
