about transcoder

This is a small transcoder made from combining rtpproxy 
with the codecs from sems. SER's nathelper module changes 
the SDP accordingly.

how to use

configure rtpproxy  with --enable-transcoder and start 
with -a lib/audio to enable transcoding support in rtpproxy.

todo:

rtpproxy: 
  - buffer if different frame size
  - check for packet loss
  - use lt dlopen


