PROG=		t_dldns
SRC=		${PROG}.c ../req.c ../cJSON.c
OBJ=		$(SRC:.c=.o)
CFLAGS=		-Wall -Werror -Wextra -Wpedantic -pedantic
LDLIBS=		-lcurl -latf-c
uname=		$(shell uname -s)
is_linux=	$(filter Linux,$(uname))
LDLIBS+=	$(if $(is_linux), -lbsd, )

.PHONY: clean

all: $(PROG)
$(PROG): $(OBJ)

clean:
	rm -f *.o t_dldns
