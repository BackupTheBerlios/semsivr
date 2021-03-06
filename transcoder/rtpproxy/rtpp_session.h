/*
 * Copyright (c) 2004 Maxim Sobolev
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
 * $Id: rtpp_session.h,v 1.1 2006/02/16 19:33:05 sayer Exp $
 *
 */

#ifndef _RTPP_SESSION_H_
#define _RTPP_SESSION_H_

#include <sys/types.h>
#include <sys/socket.h>

#if defined(__FreeBSD__)
#include <sys/queue.h>
#else
#include "myqueue.h"
#endif

#include "rtp_server.h"
#include "rtp_transcoder.h"

#include "rtpp_log.h"

struct rtpp_session {
    LIST_ENTRY(rtpp_session) link;
    int ttl;
    unsigned long pcount[6];
    char *call_id;
    char *tag;
    rtpp_log_t log;
    struct rtpp_session* rtcp;
    struct rtpp_session* rtp;
    /* Remote source addresses, one for caller and one for callee */
    struct sockaddr *addr[2];
    /* Flag which tells if we are allowed to update address with RTP src IP */
    int canupdate[2];
    /* Local listen addresses/ports */
    struct sockaddr *laddr[2];
    int ports[2];
    /* Descriptors */
    int fds[2];
    /* Session is complete, that is we received both request and reply */
    int complete;
    int asymmetric[2];
    /* Flags: strong create/delete; weak ones */
    int strong;
    int weak[2];
    /* Pointers to rtpp_record's opaque data type */
    void *rrcs[2];
    struct rtp_server *rtps[2];
    /* rtp transcoder for caller and callee */   
    struct rtp_transcoder* rtpt[2];
    /* which codecs are already initialized */
    int rtp_transcoder_init[2];

    char req_payload_id;
    char req_codec_id;
    char* req_fmt_parameters;
};

#endif
