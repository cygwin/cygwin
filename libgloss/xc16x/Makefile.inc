## Copyright (C) 2006 KPIT Cummins
## Copyright (C) 2009 Conny Marco Menebröcker
## All rights reserved.
##
## Redistribution and use in source and binary forms is permitted
## provided that the above copyright notice and following paragraph are
## duplicated in all such forms.
##
## This file is distributed WITHOUT ANY WARRANTY; without even the implied
## warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

CRT0 = crt0.o

# Generic object files common to all targets.
GENOBJS_NO_TRAP = close.o create.o crt0.o fstat.o getchar1.o \
    isatty.o lseek.o mem-layout.o misc.o open.o \
    read.o sbrk.o syscalls.o write.o
GENOBJS = ${GENOBJS_NO_TRAP} trap_handle.o

# Object files specific to particular targets.
EVALOBJS = ${GENOBJS}
CYGMONOBJS = ${GENOBJS_NO_TRAP}

SCRIPTS = @script_list@
BSP = @bsp_list@

# Host specific makefile fragment comes in here.
@host_makefile_frag@

#
# build a test program for each target board. Just trying to get
# it to link is a good test, so we ignore all the errors for now.
#

all: ${CRT0} ${BSP}

#
# here's where we build the board support packages for each target
#

libeval.a: $(EVALOBJS)
	${AR} ${ARFLAGS} $@ $(EVALOBJS)
	${RANLIB} $@

libcygmon.a: $(CYGMONOBJS)
	${AR} ${ARFLAGS} $@ $(CYGMONOBJS)
	${RANLIB} $@


doc:	

clean mostlyclean:
	rm -f a.out core *.i *~ *.o *-test *.srec *.dis *.map *.x

distclean maintainer-clean realclean: clean
	rm -f Makefile config.status a.out

.PHONY: install info install-info clean-info
install:
	${mkinstalldirs} ${DESTDIR}${tooldir}/lib${MULTISUBDIR}
	@for crt in ${CRT0}; do \
	$(INSTALL_PROGRAM) $${crt} $(DESTDIR)$(tooldir)/lib${MULTISUBDIR}/$${crt}; \
	done
	@for bsp in ${BSP}; do \
	 $(INSTALL_PROGRAM) $${bsp} $(DESTDIR)$(tooldir)/lib${MULTISUBDIR}; \
	done
	@for script in ${SCRIPTS}; do \
	 $(INSTALL_DATA) ${srcdir}/$${script}.ld $(DESTDIR)$(tooldir)/lib${MULTISUBDIR}/$${script}.ld; \
	done

info:
install-info:
clean-info:

# these are for the BSPs
crt0.o: ${srcdir}/crt0.S

# target specific makefile fragment comes in here.
@target_makefile_frag@

Makefile: Makefile.in ../config.status
	cd .. && $(SHELL) config.status
