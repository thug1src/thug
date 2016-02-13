ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup
	make all
else
include PathDefs
endif

TOPDIR = /usr/local/sce
INCOPT = -I$(TOPDIR)/common/include -I$(TOPDIR)/iop/install/include

CFLAGS  = $(INCOPT) -I. -Wall -G0 -g -D__LANGUAGE_C
ASFLAGS = $(INCOPT) -G0
RM          = /bin/rm -f

COMPILE.s = $(CC) -xassembler-with-cpp $(ASFLAGS) $(CPPFLAGS) $(TARGET_MACH) -c

#----------- customize section --------------
PROGNAME = fileio

OBJS     = start.o command.o
#ILIBS	= -ilb=$(TOP)lib/iop.ilb 
ILIBS	=
LIBI	= /usr/local/sce/iop/install/lib

#----------- rules --------------
all:	$(PROGNAME).irx

clean:
	rm -f *.o $(PROGNAME).irx *.obj *.map

$(PROGNAME).irx: $(OBJS)
	$(LINK.o)  -o $@ \
		$(OBJS) -L../../../lib -L./ -L$(LIBI) -ilb=cdvdman.ilb

start.o:	fileio.h fileio_iop.h
command.o:	fileio.h fileio_iop.h
