
TOP         = /usr/local/sce/ee
LIBDIR      = $(TOP)/lib
INCDIR      = $(TOP)/include

TARGET      = main
OBJS        = crt0.o        \
              $(TARGET).o   \
              dma.o         \
              vif.o         \
              vu.o          \
              gif.o         \
              gs.o          \
              pcrtc.o       \
              dmacalls.o    \
              mikemath.o    \
              render.o      \
              import.o      \
              texture.o     \
              material.o    \
              mesh.o        \
              nx_init.o     \
              line.o        \
              chars.o       \
              microcode.o   \
              /skate4/code/sys/file/ngps/p_filesys.o    \
              /skate4/code/core/debug/Debug.o           \
              /skate4/code/core/debug/Assert.o          \

LCFILE      = app.cmd

LIBS        = $(LIBDIR)/libsn.a    \
              $(LIBDIR)/libgraph.a \
              $(LIBDIR)/libdma.a   \
              $(LIBDIR)/libdev.a   \
              $(LIBDIR)/libpad.a   \
              $(LIBDIR)/libpkt.a   \
              $(LIBDIR)/libvu0.a

PREFIX      = ee
AS          = $(PREFIX)-gcc
CC          = $(PREFIX)-gcc
LD          = $(PREFIX)-gcc
DVPASM      = ps2dvpas
OBJDUMP     = $(PREFIX)-objdump
RUN         = ps2run -r
RM          = del

CFLAGS      = -O0 -g  -Wall -Werror -fno-common
CXXFLAGS    = -O0 -Wall -Werror -fno-exceptions -fno-common
ASFLAGS     = -c -xassembler-with-cpp
DVPASMFLAGS = -g -W
LDFLAGS     = -Wl,-Map,$(TARGET).map -nostartfiles -L$(LIBDIR) -lm
TMPFLAGS    =

.SUFFIXES: .c .s .cc .dsm .vsm

all: $(TARGET).elf

main.elf: $(OBJS) $(LIBS)
	$(LD) -o $@ -T $(LCFILE) $(OBJS) $(LIBS) $(LDFLAGS)

crt0.o: crt0.s
	$(AS) $(ASFLAGS) $(TMPFLAGS) -o $@ $<

.s.o:
	$(AS) $(ASFLAGS) $(TMPFLAGS) -I$(INCDIR) -o $@ $<

.dsm.o:
	$(DVPASM) $(DVPASMFLAGS) -I$(INCDIR) -o $@ $<

.vsm.o:
	$(DVPASM) $(DVPASMFLAGS) -I$(INCDIR) -o $@ $<

.cpp.o:
	$(CC) $(CFLAGS) $(TMPFLAGS) -I$(INCDIR) -I/skate4/code -I/skate4/code/core/STL \
    -I/usr/local/sce/common/include -D__PLAT_NGPS__ -c $< -o $*.o \


.cc.o:
	$(CC) $(CXXFLAGS) $(TMPFLAGS) -I$(INCDIR) -c $< -o $*.o

run: $(TARGET).elf
	$(RUN) $(TARGET).elf

clean:
	$(RM) *.o
	$(RM) *.map
	$(RM) *.dis
	$(RM) *.elf

