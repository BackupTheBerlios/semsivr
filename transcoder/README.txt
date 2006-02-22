about

This is a small transcoder made from combining rtpproxy with the codecs from sems.

configure rtpproxy  with --enable-transcoder and start with -a lib/audio to enable 
transcoding support in rtpproxy.

codec codes for the first parameter to force_rtp_transcode:

      I  :  iLBC in 30ms frame mode
      i  :  iLBC in 20ms frame mode
      G  :  GSM 0610
      W	 :  g722.2
      A	 :  G711a (PCMA)
      U  :  G711u (PCMU)
      S  :  speex nb

example, from iLBC (caller) to Speex (callee):
	if (method == "INVITE") {
		force_rtp_transcode("S","192.168.5.100");
		t_on_reply("1");
		t_relay();
	}
...

onreply_route[1] {
	if (!(status=~"183" || status=~"200"))
		break;
	force_rtp_transcode("I","192.168.5.100");
}


todo:

rtpproxy: 
  - buffer if different frame size OK
  - check for packet loss (TS)
  - use lt dlopen
  - update RTCP	