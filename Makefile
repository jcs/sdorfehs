DESTDIR=
PREFIX=		/usr/local
X11BASE=	/usr/X11R6
SYSCONFDIR=	/etc

PKGLIBS=	x11 xft xrandr xtst

CC=		cc
CFLAGS=		-g -O2 -Wall -DSYSCONFDIR="\"$(SYSCONFDIR)\"" \
		`pkg-config --cflags ${PKGLIBS}`
LDFLAGS=	`pkg-config --libs ${PKGLIBS}`

# uncomment to enable debugging
#CFLAGS+=	-DDEBUG=1
# and this for input-specific debugging
#CFLAGS+=	-DINPUT_DEBUG=1

BINDIR=		$(DESTDIR)$(PREFIX)/bin
MANDIR=		$(DESTDIR)$(PREFIX)/man/man1

OBJ=		actions.o \
		bar.o \
		communications.o \
		completions.o \
		editor.o \
		events.o \
		format.o \
		frame.o \
		globals.o \
		group.o \
		history.o \
		hook.o \
		input.o \
		linkedlist.o \
		manage.o \
		number.o \
		sbuf.o \
		screen.o \
		sdorfehs.o \
		split.o \
		utf8.o \
		util.o \
		vscreen.o \
		window.o \
		xrandr.o

DEPS=		actions.h \
		bar.h \
		communications.h \
		completions.h \
		config.h \
		data.h \
		editor.h \
		events.h \
		format.h \
		frame.h \
		globals.h \
		group.h \
		history.h \
		hook.h \
		input.h \
		linkedlist.h \
		manage.h \
		messages.h \
		number.h \
		sbuf.h \
		screen.h \
		sdorfehs.h \
		split.h \
		utf8.h \
		util.h \
		vscreen.h \
		window.h \
		xrandr.h

BIN=		sdorfehs

MAN=		sdorfehs.1

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

sdorfehs: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

all: sdorfehs

install: all
	mkdir -p $(BINDIR) $(MANDIR)
	install -s $(BIN) $(BINDIR)
	install -m 644 $(MAN) $(MANDIR)

clean:
	rm -f $(BIN) $(OBJ)

.PHONY: all install clean
