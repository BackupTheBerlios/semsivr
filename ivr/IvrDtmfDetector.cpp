/*
 * $Id: IvrDtmfDetector.cpp,v 1.7 2004/06/29 15:50:59 sayer Exp $
 * Copyright (C) 2002-2003 Fhg Fokus
 *
 * This file is part of sems, a free SIP media server.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  DTMF recognition code taken from isdn4linux with kind permission by 
 *  Christian Mock <cm at tahina.priv.at>
 *
 */

#include "IvrPython.h"
#include "IvrDtmfDetector.h"
#include "IvrMediaHandler.h"

#include <math.h>
#include <sys/time.h>
#include "log.h"

// -------------------------------------------------------------------------------------------
#define IVR_DTMF_ASTERISK 10
#define IVR_DTMF_HASH     11
#define IVR_DTMF_A        12
#define IVR_DTMF_B        13 
#define IVR_DTMF_C        14 
#define IVR_DTMF_D        15

/* the detector returns these values */

static int IVR_dtmf_matrix[4][4] =
  {
    {                1, 2, 3,             IVR_DTMF_A},
    {                4, 5, 6,             IVR_DTMF_B},
    {                7, 8, 9,             IVR_DTMF_C},
    {IVR_DTMF_ASTERISK, 0, IVR_DTMF_HASH, IVR_DTMF_D}
  };


#define DTMF_TRESH   100000     /* above this is dtmf                         */
#define SILENCE_TRESH   100     /* below this is silence                      */
#define H2_TRESH      20000	/* 2nd harmonic                               */
#define DTMF_DUR         25	/* nr of samples to detect a DTMF tone        */
#define AMP_BITS          9     /* bits per sample, reduced to avoid overflow */
#define NCOEFF           16     /* number of frequencies to be analyzed       */
#define LOGRP             0
#define HIGRP             1

typedef struct {
  int freq;			/* frequency */
  int grp;			/* low/high group */
  int k;			/* k */
  int k2;			/* k for 1st harmonic */
} dtmf_t;


static int cos2pik[NCOEFF] = {		/* 2 * cos(2 * PI * k / N), precalculated */
  55812,      29528,      53603,      24032,      
  51193,      14443,      48590,       6517 
};

static dtmf_t dtmf_tones[8] = 
  {
    { 697, LOGRP,  0,  1 },
    { 770, LOGRP,  2,  3 },
    { 852, LOGRP,  4,  5 },
    { 941, LOGRP,  6,  7 },
    {1209, HIGRP,  8,  9 },
    {1336, HIGRP, 10, 11 },
    {1477, HIGRP, 12, 13 },
    {1633, HIGRP, 14, 15 }
  };

static char dtmf_matrix[4][4] =
  {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
  };

dtmf_state::dtmf_state() {
  idx = 0;
  last = ' ';
}

/*
 * Goertzel algorithm.
 * See http://ptolemy.eecs.berkeley.edu/~pino/Ptolemy/papers/96/dtmf_ict/
 * for more info.
 */
void
IvrDtmfDetector::isdn_audio_goertzel(int *sample, int* result)
{
  int sk,
    sk1,
    sk2;
  int k,
    n;
  for (k = 0; k < NCOEFF; k++) {
    sk = sk1 = sk2 = 0;
    for (n = 0; n < DTMF_NPOINTS; n++) {
      sk = sample[n] + ((cos2pik[k] * sk1) >> 15) - sk2;
      sk2 = sk1;
      sk1 = sk;
    }
    result[k] =
      ((sk * sk) >> AMP_BITS) -
      ((((cos2pik[k] * sk) >> 15) * sk2) >> AMP_BITS) +
      ((sk2 * sk2) >> AMP_BITS);
  }
}

void
IvrDtmfDetector::isdn_audio_eval_dtmf(int* result, dtmf_state *s)
{
    int silence;
    int i;
    int grp[2];
    char what;
    int IVR_what;

    grp[LOGRP] = grp[HIGRP] = -2;
    silence = 0;
 
    for (i = 0; i < 8; i++) {
      if ((result[dtmf_tones[i].k] > DTMF_TRESH) &&
	  (result[dtmf_tones[i].k2] < H2_TRESH))
	grp[dtmf_tones[i].grp] = ((grp[dtmf_tones[i].grp] == -2) ? i : -1);
      else if ((result[dtmf_tones[i].k] < SILENCE_TRESH) &&
	       (result[dtmf_tones[i].k2] < SILENCE_TRESH))
	silence++;
    }
    if (silence == 8)
      what = ' ';
    else {
      if ((grp[LOGRP] >= 0) && (grp[HIGRP] >= 0)) {
	what = dtmf_matrix[grp[LOGRP]][grp[HIGRP] - 4];
	IVR_what = IVR_dtmf_matrix[grp[LOGRP]][grp[HIGRP] - 4];
	if (s->last != ' ' && s->last != '.')
	  s->last = what;	/* min. 1 non-DTMF between DTMF */
      } else
	what = '.';
    }
    if ((what != s->last) && (what != ' ') && (what != '.')) {
      if (destinationEventQueue) {
	DBG("Posting DTMF: %c (%d)\n",what, IVR_what);
 	destinationEventQueue->postEvent(new IvrScriptEvent(IvrScriptEvent::IVR_DTMF, IVR_what));
      } else {
	DBG("DTMF: %c (%d) [no script to notify]\n",what, IVR_what);
      }
    }
	
    s->last = what;
}

void
IvrDtmfDetector::isdn_audio_calc_dtmf(dtmf_state *s, pcm* buf, int len)
{
    int i;
    int c;
    
    while (len) {
//	c = min(len, (DTMF_NPOINTS - s->idx));
	if (len<(DTMF_NPOINTS - s->idx))
	    c = len;
	else
	    c = (DTMF_NPOINTS - s->idx);
	
	if (c <= 0)
	    break;
	for (i = 0; i < c; i++) {
	  s->buf[s->idx++] = (*buf++) >> (15 - AMP_BITS);;
	}
	if (s->idx == DTMF_NPOINTS) {
	    isdn_audio_goertzel(s->buf, s->result);
	    s->idx = 0;
	    isdn_audio_eval_dtmf(s->result, s);
	}
	len -= c;
    }
}

int IvrDtmfDetector::streamPut(unsigned char* samples, unsigned int size, unsigned int user_ts)
{
  isdn_audio_calc_dtmf(&state, (signed short *)samples, size/2);
  return size;
}

IvrDtmfDetector::IvrDtmfDetector()
  :  destinationEventQueue(0)
{
  /* precalculate 2 * cos (2 PI k / N) */
  int i, kp, k;
  kp = 0;
  for(i = 0; i < 8; i++) {
    k = (int)rint((double)dtmf_tones[i].freq * DTMF_NPOINTS / SAMPLERATE);
    cos2pik[kp] = (int)(2 * 32768 * cos(2 * PI * k / DTMF_NPOINTS));
    dtmf_tones[i].k = kp++;
    
    k = (int)rint((double)dtmf_tones[i].freq * 2 * DTMF_NPOINTS / SAMPLERATE);
    cos2pik[kp] = (int)(2 * 32768 * cos(2 * PI * k / DTMF_NPOINTS));
    dtmf_tones[i].k2 = kp++;
  }
}

IvrDtmfDetector::~IvrDtmfDetector()
{
}

void IvrDtmfDetector::setDestinationEventQueue(AmEventQueue* eventQueue) {
  destinationEventQueue = eventQueue;
}
