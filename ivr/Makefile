plug_in_name = ivr

PYTHON_VERSION = 2.2
# adjust to point to python include path
PYTHON_DIR = /home/ssa/Python/include/python$(PYTHON_VERSION)

# for Text-to-Speech
FLITE_DIR = /home/ssa/flite-1.2-release

LOCAL_INCLUDES = -I$(FLITE_DIR)/lang/usenglish
LD_FLAGS = -L/home/ssa/Python/lib/python$(PYTHON_VERSION)/config \
	-lpython$(PYTHON_VERSION) -ldl -lpthread -lutil -lm \
	-Xlinker -export-dynamic
   

module_ldflags = -L/home/ssa/Python/lib/python$(PYTHON_VERSION)/config \
	-lpython$(PYTHON_VERSION) -ldl -lpthread -lutil -lm \
	-Xlinker -export-dynamic 
#\
#	$(FLITE_DIR)/lib/libflite.a -L$(FLITE_DIR)/lib -lm  -lflite_cmu_us_kal  -lflite_usenglish \
#	  -lflite_cmulex $(FLITE_DIR)/lib/libflite.a -lflite \

# put used modules from lib-dynload here, e.g.
#	/home/ssa/Python/lib/python2.2/lib-dynload/time.so
   
module_cflags  = -I$(PYTHON_DIR) -I$(FLITE_DIR)/include 

# no TTS yet...
#-DIVR_WITH_TTS 

include ../Makefile.app_module
