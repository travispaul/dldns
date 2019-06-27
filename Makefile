.include "Makefile.inc"

PROG=		dldns
SRCS=		${PROG}.c req.c cJSON.c log.c
OBJS=		*.o
LDADD=	-lcurl

.include <bsd.prog.mk>
