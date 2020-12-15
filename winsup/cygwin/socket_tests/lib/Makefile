include ../Makefile.inc

AF_UNIX_LIB = ../libafunix.a

all: ${AF_UNIX_LIB}

${AF_UNIX_LIB}: *.c ename.c.inc
	${CC} -c ${CFLAGS} *.c
	${RM} ${AF_UNIX_LIB}
	${AR} rs ${AF_UNIX_LIB} *.o

ename.c.inc:
	sh Build_ename.sh > ename.c.inc
	echo 1>&2 "ename.c.inc built"

*.o: af_unix_hdr.h

error_functions.o: error_functions.h

scm_functions.o: scm_functions.h

unix_sockets.o: unix_sockets.h

inet_sockets.o: inet_sockets.h

read_line.o: read_line.h

get_num.o: get_num.h

clean:
	${RM} *.o ename.c.inc ${AF_UNIX_LIB}
