VERSION=	1.5

VERSION!=	[ -d .git ] && \
		echo "git-`git rev-list --abbrev-commit --tags --max-count=1`" || \
		echo "${VERSION}"

CC?=		cc
PREFIX?=	/usr/local
PKGLIBS=	x11 xft xrandr xtst xres freetype2
CFLAGS+=	-O2 -Wall \
		-Wunused -Wmissing-prototypes -Wstrict-prototypes \
		`pkg-config --cflags ${PKGLIBS}` \
		-DVERSION=\"${VERSION}\"
LDFLAGS+=	`pkg-config --libs ${PKGLIBS}`

# uncomment to enable debugging
#CFLAGS+=	-g -DDEBUG=1
# and this for subsystem-specific debugging
#CFLAGS+=	-DINPUT_DEBUG=1
#CFLAGS+=	-DSENDCMD_DEBUG=1

BINDIR=		${DESTDIR}$(PREFIX)/bin
MANDIR?=	${DESTDIR}$(PREFIX)/man/man1

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
