PROG=		dldns
SRC=		$(wildcard *.c)
OBJ=		$(SRC:.c=.o)
CFLAGS=		-Wall -Werror -Wextra -Wpedantic -pedantic \
			-fPIE -fstack-protector-all -D_FORTIFY_SOURCE=2 -O3 \
			-DLOG_USE_COLOR
LDLIBS=		-lcurl
uname=		$(shell uname -s)
is_linux=	$(filter Linux,$(uname))
LDLIBS+=	$(if $(is_linux), -lbsd, )

.PHONY: clean

all: $(PROG)
$(PROG): $(OBJ)

clean:
	rm -f *.o dldns
