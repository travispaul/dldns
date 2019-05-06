.include "../Makefile.inc"

PROG=		t_dldns
SRCS=		${PROG}.c ../req.c ../cJSON.c
LDADD=	-lcurl -latf-c
NOMAN=

test:
	atf-run t_dldns | atf-report

.include <bsd.prog.mk>
