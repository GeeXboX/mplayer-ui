include ../config.mak

LIBNAME=libmpui.a

SRCS=mpui_struct.c mpui_parser.c mpui_browser.c mpui_image.c mpui_focus.c mpui_render.c mpui_cmd.c vf_mpui.c
OBJS=$(SRCS:.c=.o)

CFLAGS=$(OPTFLAGS) -I. -I.. $(FREETYPE_INC) $(EXTRA_INC) -Wall

.SUFFIXES: .c .o


.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

$(LIBNAME): $(OBJS)
	$(AR) r $(LIBNAME) $(OBJS)
	$(RANLIB) $(LIBNAME)

all: $(LIBNAME)

clean:
	rm -f *.o *.a *~

distclean:
	rm -f Makefile.bak *.o *.a *~ .depend

dep: depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
