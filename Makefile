.include "Makefile.inc"

PROG=		dldns
SRCS=		${PROG}.c req.c cJSON.c
OBJS=		*.o
LDADD=	-lcurl

.include <bsd.prog.mk>
