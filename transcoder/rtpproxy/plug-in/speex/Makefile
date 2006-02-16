plug_in_name = speex

# SPEEXDIR=speex-1.0.4
SPEEXDIR=speex_src
CONFIGURE_FLAG=--with-pic

module_ldflags = 
module_cflags  = -I $(SPEEXDIR)/include -ansi # -DNOFPU

module_extra_objs = $(SPEEXDIR)/libspeex/.libs/libspeex.a

extra_clean = clean_speex

include ../Makefile.audio_module

$(SPEEXDIR)/Makefile:
	cd $(SPEEXDIR); ./configure $(CONFIGURE_FLAG)

$(SPEEXDIR)/libspeex/.libs/libspeex.a:: $(SPEEXDIR)/Makefile
	$(MAKE) -C $(SPEEXDIR)/libspeex

.PHONY: clean_speex
clean_speex:
	$(MAKE) -C $(SPEEXDIR) distclean
