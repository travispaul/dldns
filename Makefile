PROG=			dldns
SRCS=			main.c cJSON.c req.c
OBJS=			*.o 
LDADD=		-lcurl
PREFIX?=	/usr/pkg
CFLAGS+=	-Wall -Werror -I${PREFIX}/include
LDFLAGS+=	-L${PREFIX}/lib

.include <bsd.prog.mk>
