bin_PROGRAMS=rtpproxy
rtpproxy_SOURCES=main.c myqueue.h rtp.h rtp_server.c rtp_server.h \
  rtpp_defines.h rtpp_log.h rtpp_record.c rtpp_record.h rtpp_session.h \
  rtpp_util.c rtpp_util.h \
  am_plugin_wrapper.h am_plugin_wrapper.cpp AmPlugIn.cpp AmPlugIn.h \
  log.h log.cpp \
  rtp_transcoder.c rtp_transcoder.h \
  amci/amci.h amci/codecs.h
rtpproxy_LDFLAGS=-ldl -rdynamic
if TRANSCODER 
   MOD_DIR = plug-in
endif 
SUBDIRS=. $(MOD_DIR)
