plug_in_name = ivr

#
# Python specific
#  (no need to change this if you want to use perl)
#
# PYTHON_VERSION might also be 2.2
# do a ls /usr/include/python2.3/Python.h to see if it's there
PYTHON_VERSION = 2.3
# adjust to point to python include path
# can also be /usr/include/python$(PYTHON_VERSION)
# look for Python.h in the specified path
PYTHON_DIR = /usr/include/python$(PYTHON_VERSION)
PYTHON_LIBDIR = /usr/lib/python$(PYTHON_VERSION)
# put used Python modules from lib-dynload here, e.g. time, mysql etc.
#PYTHON_DYNLOAD_MODULES = $(PYTHON_LIBDIR)/lib-dynload/time.so	

#
# perl specific
#
# uncomment the next lines if you want to script the ivr with perl scripts
#IVR_PERL_DYNLOAD_MODULES = /usr/lib/perl/5.8/auto/IO/IO.so /usr/lib/perl/5.8/auto/Fcntl/Fcntl.so /usr/lib/perl/5.8/auto/Socket/Socket.so /usr/local/lib/perl/5.8.3/auto/DBI/DBI.so /usr/lib/perl/5.8/auto/Sys/Hostname/Hostname.so /usr/lib/perl/5.8/auto/Sys/Syslog/Syslog.so /usr/lib/perl/5.8/auto/Sys/Syslog/Syslog.so /usr/lib/perl5/auto/Date/Calc/Calc.so /usr/lib/perl/5.8/auto/MIME/Base64/Base64.so /usr/lib/perl/5.8/auto/POSIX/POSIX.so /usr/lib/perl/5.8/auto/Time/HiRes/HiRes.so /usr/lib/perl/5.8/auto/Cwd/Cwd.so /usr/lib/perl5/auto/Bit/Vector/Vector.so /usr/lib/perl5/auto/DBD/mysql/mysql.so
#IVR_PERL_module_ldflags = `perl -MExtUtils::Embed -e ldopts` -DPERL_IMPLICIT_CONTEXT $(IVR_PERL_DYNLOAD_MODULES)
#IVR_PERL_module_cflags = -DIVR_PERL `perl -MExtUtils::Embed -e ccopts ` -DPERL_IMPLICIT_CONTEXT

#
#   flite text to speech
#
# uncomment the next lines if you want to have flite text-to-speech (ivr.say("Hello there"); ) 
# FLITE_DIR = /usr/src/flite-1.2-release
#IVR_TTS_module_ldflags =  $(FLITE_DIR)/lib/libflite.a -L$(FLITE_DIR)/lib -lm  \
#	-lflite_cmu_us_kal  -lflite_usenglish \
#	-lflite_cmulex $(FLITE_DIR)/lib/libflite.a -lflite
#IVR_TTS_module_cflags = -I$(FLITE_DIR)/include  -DIVR_WITH_TTS


LOCAL_INCLUDES = -I$(FLITE_DIR)/lang/usenglish
LD_FLAGS = -L$(PYTHON_LIBDIR)/config \
	-lpython$(PYTHON_VERSION) -ldl -lpthread -lutil -lm \
	-Xlinker --export-dynamic -Xlinker --no-strip-discarded
   

module_ldflags = -L$(PYTHON_LIBDIR)/config \
	-lpython$(PYTHON_VERSION) -ldl -lpthread -lutil -lm \
	-Xlinker --export-dynamic \
	$(IVR_TTS_module_ldflags) \
	$(IVR_PERL_module_ldflags) \
	$(PYTHON_DYNLOAD_MODULES)
   

# for perl support:
# -DIVR_PERL  `perl -MExtUtils::Embed -e ccopts `
# for flite text-to-speech support -DIVR_WITH_TTS
module_cflags  = -I$(PYTHON_DIR)  \
	$(IVR_TTS_module_cflags) \
	$(IVR_PERL_module_cflags)

include ../Makefile.app_module
