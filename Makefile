PROG=		dldns
SRCS=		dldns.c cJSON.c req.c
OBJS=		*.o
LDADD=		-lcurl
PREFIX?=	/usr/pkg
CFLAGS+=	-Wall -Werror -Wextra -Wpedantic -pedantic -I${PREFIX}/include
LDFLAGS+=	-L${PREFIX}/lib

.include <bsd.prog.mk>
