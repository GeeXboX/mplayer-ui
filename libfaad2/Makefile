
LIBNAME = libfaad2.a

include ../config.mak

SRCS    = bits.c cfft.c common.c decoder.c drc.c error.c filtbank.c hcr.c huffman.c ic_predict.c is.c lt_predict.c mdct.c mp4.c ms.c output.c pns.c ps_dec.c ps_syntax.c  pulse.c rvlc.c sbr_dct.c sbr_dec.c sbr_e_nf.c sbr_fbt.c sbr_hfadj.c sbr_hfgen.c sbr_huff.c sbr_qmf.c sbr_syntax.c sbr_tf_grid.c specrec.c ssr.c ssr_fb.c ssr_ipqf.c syntax.c tns.c
OBJS	= $(SRCS:.c=.o)

CFLAGS  = -I. $(OPTFLAGS) 

# Uncomment this to use the FIXED_POINT implementation of FAAD2.
# This should improve performance, especially for SBR files.
#CFLAGS  = -I. $(OPTFLAGS) -DFIXED_POINT

.SUFFIXES: .c .o

# .PHONY: all clean

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

$(LIBNAME):	$(OBJS)
	$(AR) r $(LIBNAME) $(OBJS)
	$(RANLIB) $(LIBNAME)

all:	$(LIBNAME)

clean:
	rm -f *.o *.a *~

distclean:
	rm -f test *.o *.a *~ .depend

dep:    depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
