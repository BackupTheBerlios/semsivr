plug_in_name = ivr

PYTHON_VERSION = 2.2
# adjust to point to python include path
# can also be /usr/include/python$(PYTHON_VERSION)
# look for Python.h in the specified path
PYTHON_DIR = /usr/local/include/python$(PYTHON_VERSION)

# uncomment the next two lines if you want to script the ivr with perl scripts
#IVR_PERL_module_ldflags = `perl -MExtUtils::Embed -e ldopts` -DPERL_IMPLICIT_CONTEXT
#IVR_PERL_module_cflags = -DIVR_PERL `perl -MExtUtils::Embed -e ccopts ` -DPERL_IMPLICIT_CONTEXT

# uncomment the next lines if you want to have flite text-to-speech (ivr.say("Hello there"); ) 

# FLITE_DIR = /usr/src/flite-1.2-release
#IVR_TTS_module_ldflags =  $(FLITE_DIR)/lib/libflite.a -L$(FLITE_DIR)/lib -lm  \
#	-lflite_cmu_us_kal  -lflite_usenglish \
#	-lflite_cmulex $(FLITE_DIR)/lib/libflite.a -lflite
#IVR_TTS_module_cflags = -I$(FLITE_DIR)/include  -DIVR_WITH_TTS



LOCAL_INCLUDES = -I$(FLITE_DIR)/lang/usenglish
LD_FLAGS = -L/usr/lib/python$(PYTHON_VERSION)/config \
	-lpython$(PYTHON_VERSION) -ldl -lpthread -lutil -lm \
	-Xlinker -export-dynamic
   

module_ldflags = -L/usr/lib/python$(PYTHON_VERSION)/config \
	-lpython$(PYTHON_VERSION) -ldl -lpthread -lutil -lm \
	-Xlinker -export-dynamic \
	$(IVR_TTS_module_ldflags) \
	$(IVR_PERL_module_ldflags)	
# Python: put used modules from pythons lib-dynload here, e.g.
#	/usr/lib/python2.2/lib-dynload/time.so
   
# for perl support:
# -DIVR_PERL  `perl -MExtUtils::Embed -e ccopts `
# for flite text-to-speech support -DIVR_WITH_TTS
module_cflags  = -I$(PYTHON_DIR)  \
	$(IVR_TTS_module_cflags) \
	$(IVR_PERL_module_cflags)

include ../Makefile.app_module
