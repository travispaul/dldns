PREFIX?=	/usr/pkg

CFLAGS+=	-Wall -Werror -Wextra -Wpedantic -pedantic
CFLAGS+=	-I${PREFIX}/include
CFLAGS+=	-DLOG_USE_COLOR

LDFLAGS+=	-L${PREFIX}/lib
