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
 * $Id: rtp_transcoder.h,v 1.4 2006/02/22 21:15:17 sayer Exp $
 *
 */

#ifndef _RTP_TRANSCODER_H_
#define _RTP_TRANSCODER_H_

#include "amci/amci.h"

#include <sys/types.h>

struct rtpp_session;

struct rtp_transcoder {
  char to_payload_id;
  char from_payload_id;

  long handle_from;
  struct amci_codec_t* codec_from;
  unsigned int from_framelength;
  unsigned int from_encodedsize;

  long handle_to;
  struct amci_codec_t* codec_to;  
  unsigned int to_framelength;
  unsigned int to_encodedsize;

  char pcmbuf[1024*10];
  unsigned short last_seq;
  unsigned short to_seq;
  uint32_t begin_ts;
  uint32_t end_ts;
  char* audio_end;

  int had_packet;
};

struct rtp_transcoder *rtp_transcoder_new(char from_payload_id, int from_codec_id, 
					  char* format_parameters_from,
					  char to_payload_id,   int to_codec_id,
					  char* format_parameters_to);
int rtp_transcoder_update(struct rtp_transcoder* rt, 
			  char from_payload_id, int from_codec_id, 
			  char* format_parameters_from,
			  char to_payload_id,   int to_codec_id,
			  char* format_parameters_to);
void rtp_transcoder_free(struct rtp_transcoder* rt);
int rtp_server_transcode(struct rtp_transcoder* rt, struct rtpp_session* sp, char* buf);

#endif
