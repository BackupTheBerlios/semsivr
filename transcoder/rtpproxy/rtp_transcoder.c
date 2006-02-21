/*
 * Copyright (c) 2006 Stefan Sayer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: rtp_transcoder.c,v 1.4 2006/02/21 23:57:38 sayer Exp $
 *
 */
#include <stdlib.h>

#include "rtpp_util.h"
#include "rtp.h"
#include "rtpp_log.h"

#include "am_plugin_wrapper.h"

#include "rtp_transcoder.h" 
#include "rtpp_session.h"

extern rtpp_log_t glog;

struct rtp_transcoder *rtp_transcoder_new(char from_payload_id, int from_codec_id, 
					  char* format_parameters_from,
					  char to_payload_id,   int to_codec_id,
					  char* format_parameters_to) {
  struct rtp_transcoder *rt;

  rt = (struct rtp_transcoder *)malloc(sizeof(*rt));
  if (rt == NULL)
    return NULL;

  memset(rt, 0, sizeof(*rt));
  
  if (!rtp_transcoder_update(rt, from_payload_id, from_codec_id, 
			     format_parameters_from,
			     to_payload_id, to_codec_id,
			     format_parameters_to)) {
    free(rt);
    return NULL;
  }

  return rt;
}

int rtp_transcoder_update(struct rtp_transcoder* rt, 
			  char from_payload_id, int from_codec_id, 
			  char* format_parameters_from,
			  char to_payload_id,   int to_codec_id,
			  char* format_parameters_to) {

  amci_codec_fmt_info_t fmtinfo[8];
  unsigned int i;

  if (from_codec_id) {
    // free the old codec...
    if ((rt->codec_from != NULL) && (rt->codec_from->destroy != NULL))
      rt->codec_from->destroy(rt->handle_from);

    rt->from_payload_id = from_payload_id;
    // get the codec
    rt->codec_from = am_plugin_get_codec(from_codec_id);
    if (rt->codec_from == NULL) 
      return 0;
    // init the codec
    if (rt->codec_from->init != NULL) {
      rt->handle_from = rt->codec_from->init(format_parameters_from, 
					     fmtinfo);
      for (i=0; fmtinfo[i].id!=0;i++) {
	if (fmtinfo[i].id == AMCI_FMT_FRAME_SIZE) {
	  rt->from_framelength = fmtinfo[i].value * 2; // sizeof(PCM16)
	      rtpp_log_write(RTPP_LOG_INFO, sp->log, // DEBUG
		   "transcode_init: from_framelength %d",
		   rt->from_framelength);

	}
	if (fmtinfo[i].id == AMCI_FMT_ENCODED_FRAME_SIZE) {
	  rt->from_encodedsize = fmtinfo[i].value;
	}
      }  
    }
    rtpp_log_write(RTPP_LOG_INFO, glog, "rtp_trancoder_update: from %d/%d", 
		   to_codec_id, to_payload_id);
  }
  
  if (to_codec_id) {
    // free the old codec...
    if ((rt->codec_to != NULL) && (rt->codec_to->destroy != NULL))
      rt->codec_to->destroy(rt->handle_to);

    memset(fmtinfo, 0, sizeof(*fmtinfo));

    rt->to_payload_id   = to_payload_id;
    // get the codec
    rt->codec_to   = am_plugin_get_codec(to_codec_id);
    if (rt->codec_to == NULL) 
      return 0;
    // init the codecs
    if (rt->codec_to->init != NULL) {
      rt->handle_to = rt->codec_to->init(format_parameters_to, 
					 fmtinfo);
      for (i=0; fmtinfo[i].id!=0;i++) {
	if (fmtinfo[i].id == AMCI_FMT_FRAME_SIZE) {
	  rt->to_framelength = fmtinfo[i].value * 2; // sizeof(PCM16);
	  rtpp_log_write(RTPP_LOG_INFO, sp->log, // DEBUG
			 "transcode_init: to_framelength %d",
			 rt->to_framelength);
	}
	if (fmtinfo[i].id == AMCI_FMT_ENCODED_FRAME_SIZE) {
	  rt->to_encodedsize = fmtinfo[i].value;
	}
      }  
    }
    rtpp_log_write(RTPP_LOG_INFO, glog, "rtp_trancoder_update: to %d/%d", 
	    to_codec_id, to_payload_id);
  }

  rt->audio_end = rt->pcmbuf;

  return 1;
}

void rtp_transcoder_free(struct rtp_transcoder *rt) {
  if (rt->codec_to->destroy != NULL)
    rt->codec_to->destroy(rt->handle_to);
  if (rt->codec_from->destroy != NULL)
    rt->codec_from->destroy(rt->handle_from);

  free(rt);
}

int rtp_transcoder_transcode(struct rtp_transcoder *rt, struct rtpp_session* sp, 
			     char* buf, int* len) {


  int audio_len; 
  char* rtpp_audio_offset;
  rtp_hdr_t *rtp;
  div_t blocks;
  
  if ((rt->codec_from == NULL) || (rt->codec_to == NULL)) {
    rtpp_log_write(RTPP_LOG_ERR, sp->log,
		   "transcode: codec not initialized.");
    return 0;
  }
  
  rtp = (rtp_hdr_t *)buf;
  
  if (rtp->pt != rt->from_payload_id) {
    rtpp_log_write(RTPP_LOG_INFO, sp->log,
		   "transcode: expected payload %d, received %d",
		   rt->from_payload_id, rtp->pt );
    return 0;
  }

  rtpp_audio_offset = buf + RTP_HDR_LEN(rtp);
  if (rtp->m || (RTP_GET_SEQ(rtp) != rt->last_seq+1)) {
    if (rt->last_seq)
      rtpp_log_write(RTPP_LOG_INFO, sp->log,
		     "transcode: packetloss (%u, %u)",
		     rtp->seq , rt->last_seq+1 );
     rt->audio_end = rt->pcmbuf; // packet loss -> drop buffered audio
  }
  rt->last_seq  =  RTP_GET_SEQ(rtp);

  audio_len = *len - RTP_HDR_LEN(rtp);
  rtpp_log_write(RTPP_LOG_INFO, sp->log, // DEBUG
		 "tr: got audio %d (in buffer %d)",
		 audio_len, rt->audio_end - rt->pcmbuf);

  audio_len = rt->codec_from->type2intern(rt->audio_end, rtpp_audio_offset, audio_len, 
					  1, 8000, rt->handle_from);
  rtpp_log_write(RTPP_LOG_INFO, sp->log, // DEBUG
		 "tr:  aud len after t2int: %d",
		 audio_len);
  if (audio_len <= 0) {
    if (audio_len < 0) 
      rtpp_log_write(RTPP_LOG_ERR, sp->log,
		     "transcode: codec_from->type2intern failed.");
    return 0;
  }

  rt->audio_end += audio_len;

  if (rt->to_framelength) {
    blocks = div(rt->audio_end - rt->pcmbuf, rt->to_framelength);

    if (blocks.quot)
      audio_len = rt->codec_to->intern2type(rtpp_audio_offset, rt->pcmbuf, blocks.quot * rt->to_framelength, 
					    1, 8000, rt->handle_to);
    else 
      audio_len = 0;
    if (blocks.rem) {
      memcpy(rt->pcmbuf, rt->pcmbuf +  blocks.quot * rt->to_framelength, blocks.rem);
    }
    rt->audio_end = rt->pcmbuf + blocks.rem;
  } else {
    audio_len = rt->codec_to->intern2type(rtpp_audio_offset, rt->pcmbuf, rt->audio_end - rt->pcmbuf, 
					  1, 8000, rt->handle_to);
    rt->audio_end = rt->pcmbuf; // clear buffer
  }
  if (audio_len <= 0) {
    if (audio_len < 0) 
      rtpp_log_write(RTPP_LOG_ERR, sp->log,
		     "transcode: codec_to->intern2type failed.");
    return 0;
  }

  rtpp_log_write(RTPP_LOG_INFO, sp->log,
		 "transcoded from payload %d to %d, size %d (%d) to %d (%d) (buffering %d).",
		 rt->from_payload_id, rt->to_payload_id, *len,*len - RTP_HDR_LEN(rtp),
		 audio_len + RTP_HDR_LEN(rtp), audio_len, rt->audio_end - rt->pcmbuf);
  
  *len = audio_len + RTP_HDR_LEN(rtp);
  
  rtp->pt = rt->to_payload_id;
  return 1;
}
