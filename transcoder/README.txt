about

This is a small transcoder made from combining rtpproxy with the codecs from sems.

configure rtpproxy  with --enable-transcoder and start with -a lib/audio to enable transcoding support in rtpproxy.

todo:

rtpproxy: 
  - buffer if different frame size
  - check for packet loss
  - use lt dlopen
