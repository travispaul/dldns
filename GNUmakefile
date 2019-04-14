PROG=		dldns
SRC=		$(wildcard *.c)
OBJ=		$(SRC:.c=.o)
CFLAGS=		-Wall -Werror -Wextra -Wpedantic -pedantic
LDLIBS=		-lcurl
uname=		$(shell uname -s)
is_linux=	$(filter Linux,$(uname))
LDLIBS+=	$(if $(is_linux), -lbsd, )

.PHONY: clean

all: $(PROG)
$(PROG): $(OBJ)

clean:
	rm -f *.o dldns
