PREFIX?=	/usr/pkg

CFLAGS+=	-Wall -Werror -Wextra -Wpedantic -pedantic
CFLAGS+=	-I${PREFIX}/include

LDFLAGS+=	-L${PREFIX}/lib
