
MPROOT=../..

include $(MPROOT)/config.mak

INCLUDE = -I$(MPROOT) -I$(MPROOT)/loader $(EXTRA_INC)
CFLAGS  = $(OPTFLAGS) $(INCLUDE)

.SUFFIXES: .c .cpp .o

# .PHONY: all clean

all: netstream

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<


netstream: $(MPROOT)/libmpdemux/libmpdemux.a netstream.o
# FIXME: linking is a mess that should be fixed properly some day
# it does not work with either GUI, live.com or libavformat enabled
	$(CC) $(CFLAGS) netstream.o -o netstream $(MPROOT)/libmpdemux/libmpdemux.a $(MPROOT)/libmpdvdkit2/libmpdvdkit.a $(MPROOT)/libmpcodecs/native/minilzo.o $(MPROOT)/libvo/aclib.o $(MPROOT)/libmpcodecs/img_format.o $(MPROOT)/liba52/liba52.a $(MPROOT)/libao2/afmt.o $(MPROOT)/m_option.o $(MPROOT)/m_struct.o $(MPROOT)/subreader.o $(MPROOT)/mp_msg.o $(MPROOT)/osdep/shmem.o $(MPROOT)/osdep/timer-*.o $(MPROOT)/osdep/strl.o $(VORBIS_LIB) $(CDPARANOIA_LIB) $(Z_LIB) $(GIF_LIB) $(SMBSUPPORT_LIB) $(LIVE_LIBS) $(ARCH_LIB) $(COMMON_LIBS) -lm

clean:
	rm -f *.o *.a *~

distclean:
	rm -f test Makefile.bak *.o *.a *~ .depend

dep:    depend

depend:
	$(CC) -MM $(CFLAGS) test.c $(SRCS) 1>.depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
