# Where to install the ratpoison executable
INSTALL_DIR=/usr/local/bin


# Spew lots of debug messages 
DEBUG = -DDEBUG

CC = gcc
LIBS = -lX11
LDFLAGS = -L/usr/X11R6/lib
CFLAGS = -g -Wall -I/usr/X11R6/include 

SRC = main.o events.o manage.o list.o bar.o
HEADERS = bar.h conf.h data.h events.h list.h manage.h ratpoison.h

all: ratpoison ratpoison.info

ratpoison: $(SRC) 
	gcc $(SRC) -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) 

ratpoison.info : ratpoison.texi
	makeinfo ratpoison.texi

install: ratpoison
	cp ratpoison $(INSTALL_DIR)

%.o : %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(DEBUG) $< -o $@

clean :
	rm -f *.o ratpoison ratpoison.info
