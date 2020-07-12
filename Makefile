PREFIX?=	/usr/local
X11BASE?=	/usr/X11R6
SYSCONFDIR?=	/etc

PKGLIBS=	x11 xft xrandr xtst

ifeq ($(shell uname), Linux)
PKGLIBS+=	libbsd-overlay
CFLAGS+=	-DLIBBSD_OVERLAY -I/usr/include/bsd
endif

CC?=		cc
CFLAGS+=	-O2 -Wall \
		-Wunused -Wmissing-prototypes -Wstrict-prototypes -Wunused \
		-DSYSCONFDIR="\"$(SYSCONFDIR)\"" \
		`pkg-config --cflags ${PKGLIBS}`
LDFLAGS+=	`pkg-config --libs ${PKGLIBS}`

# uncomment to enable debugging
#CFLAGS+=	-g -DDEBUG=1
# and this for input-specific debugging
#CFLAGS+=	-DINPUT_DEBUG=1

BINDIR=		$(PREFIX)/bin
MANDIR=		$(PREFIX)/man/man1

SRC!=		ls *.c
OBJ=		${SRC:.c=.o}

BIN=		sdorfehs
MAN=		sdorfehs.1

all: sdorfehs

sdorfehs: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

install: all
	mkdir -p $(BINDIR) $(MANDIR)
	install -s $(BIN) $(BINDIR)
	install -m 644 $(MAN) $(MANDIR)

regress:
	scan-build $(MAKE)

clean:
	rm -f $(BIN) $(OBJ)

.PHONY: all install clean
