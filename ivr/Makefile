plug_in_name = ivr

PYTHON_VERSION = 2.2
PYTHON_DIR = /usr/local/include/python$(PYTHON_VERSION)

# for Text-to-Speech
FLITE_DIR = /usr/local/include/flite

LOCAL_INCLUDES = -I$(FLITE_DIR)/lang/usenglish
LD_FLAGS = -L/usr/lib/python$(PYTHON_VERSION)/config \
	-lpython$(PYTHON_VERSION) -ldl -lpthread -lutil -lm \
	-Xlinker -export-dynamic 
   

module_ldflags = -L/usr/lib/python$(PYTHON_VERSION)/config \
	-lpython$(PYTHON_VERSION) -ldl -lpthread -lutil -lm \
	-Xlinker -export-dynamic \
	/usr/lib/python2.2/lib-dynload/time.so

#	$(FLITE_DIR)/lib/libflite.a -L$(FLITE_DIR)/lib -lm  -lflite_cmu_us_kal  -lflite_usenglish \
#	  -lflite_cmulex $(FLITE_DIR)/lib/libflite.a -lflite \
   
module_cflags  = -I$(PYTHON_DIR) -I$(FLITE_DIR)/include 
#-DIVR_WITH_TTS

include ../Makefile.app_module
