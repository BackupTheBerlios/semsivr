/*
 * $Id: IvrDtmfDetector.cpp,v 1.9 2004/07/16 17:35:06 sayer Exp $
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
 *  parts of the DTMF recognition code taken from isdn4linux with kind permission by 
 *  Christian Mock <cm at tahina.priv.at>
 *
 * compile as standalone dtmf testing with e.g.
 * g++ -o dtmf_test IvrDtmfDetector.cpp -lm -DDTMF_STANDALONE_TEST
 * convert to fmt e.g. with
 * sox infile.wav -t raw -c 1 -r 8000 -w -s ofile.raw
 *
 */

// relative dtmf detection is better that absolute (works with e.g. grandstream)
#define DTMF_USE_RELATIVE_DETECTION

#include "IvrDtmfDetector.h"
#ifndef DTMF_STANDALONE_TEST
#include "IvrPython.h"
#include "IvrMediaHandler.h"
#include "log.h"
#else
#define  DBG(s...) printf(##s)
#endif

#include <math.h>

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
#define H2_TRESH      35000	/* 2nd harmonic                               */
#define DTMF_DUR         25	/* nr of samples to detect a DTMF tone        */
#define AMP_BITS          9     /* bits per sample, reduced to avoid overflow */
#define NCOEFF           16     /* number of frequencies to be analyzed       */
#define LOGRP             0
#define HIGRP             1

#define REL_NCOEFF            8     /* number of frequencies to be analyzed       */
#define REL_DTMF_TRESH     4000     /* above this is dtmf                         */
#define REL_SILENCE_TRESH   200     /* below this is silence                      */
#define REL_AMP_BITS          9     /* bits per sample, reduced to avoid overflow */


typedef struct {
  int freq;			/* frequency */
  int grp;			/* low/high group */
  int k;			/* k */
  int k2;			/* k for 1st harmonic */
} dtmf_t;


static int cos2pik[NCOEFF] = {		/* 2 * cos(2 * PI * k / N), precalculated */
  55812,      29528,      53603,      24032,      
  51193,      14443,      48590,       6517 
}; // high group missing...

/* For DTMF recognition:
 * 2 * cos(2 * PI * k / N) precalculated for all k
 */
static int rel_cos2pik[REL_NCOEFF] =
{
    55813, 53604, 51193, 48591,      // low group
    38114, 33057, 25889, 18332       // high group
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
  last_dtmf_sample=100;
}

/*
 * Goertzel algorithm.
 * See http://ptolemy.eecs.berkeley.edu/~pino/Ptolemy/papers/96/dtmf_ict/
 * for more info.
 */
#ifndef DTMF_USE_RELATIVE_DETECTION
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
#else //DTMF_USE_RELATIVE_DETECTION
void
IvrDtmfDetector::isdn_audio_goertzel_relative(int *sample, int* result)
{
	int sk,
	 sk1,
	 sk2;
	int k,
	 n;

	for (k = 0; k < REL_NCOEFF; k++) {
		sk = sk1 = sk2 = 0;
		for (n = 0; n < REL_DTMF_NPOINTS; n++) {
			sk = sample[n] + ((rel_cos2pik[k] * sk1) >> 15) - sk2;
			sk2 = sk1;
			sk1 = sk;
		}
		/* Avoid overflows */
		sk >>= 1;
		sk2 >>= 1;
		/* compute |X(k)|**2 */
		/* report overflows. This should not happen. */
		/* Comment this out if desired */
		/*if (sk < -32768 || sk > 32767)
			DBG("isdn_audio: dtmf goertzel overflow, sk=%d\n", sk);
		if (sk2 < -32768 || sk2 > 32767)
		    DBG("isdn_audio: dtmf goertzel overflow, sk2=%d\n", sk2);
*/

		result[k] =
		    ((sk * sk) >> REL_AMP_BITS) -
		    ((((rel_cos2pik[k] * sk) >> 15) * sk2) >> REL_AMP_BITS) +
		    ((sk2 * sk2) >> REL_AMP_BITS);
	}
}
#endif // DTMF_USE_RELATIVE_DETECTION

#ifndef DTMF_USE_RELATIVE_DETECTION
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

#ifdef DTMF_STANDALONE_TEST
    printf(" ");
    for (i = 0; i < 8; i++) 
      printf("%8d %8d - ", result[i*2], result[i*2+1]);
    printf("\n");
#endif

    s->last_dtmf_sample++;
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

    if (s->last_dtmf_sample >= DTMF_INTERVAL) {
#ifndef DTMF_STANDALONE_TEST
      if (destinationEventQueue) {
	DBG("Posting DTMF %d: %c (%d)\n",  s->last_dtmf_sample, what, IVR_what);
 	destinationEventQueue->postEvent(new IvrScriptEvent(IvrScriptEvent::IVR_DTMF, IVR_what));
      } else {
	DBG("DTMF %d: %c (%d) [no script to notify]\n",  s->last_dtmf_sample, what, IVR_what);
      }
      s->last_dtmf_sample=0;
#else
      printf("DTMF: %c (%d)\n", what, IVR_what);
#endif

    } else {
#ifndef DTMF_STANDALONE_TEST
	DBG("DTMF: false positive, %d too close, %c (%d)\n", 
	    s->last_dtmf_sample, what, IVR_what);
#else 
	printf("DTMF: false positive, %d too close, %c (%d)\n", 
	    s->last_dtmf_sample, what, IVR_what);
#endif
    }

    }
	
    s->last = what;
}

#else // DTMF_USE_RELATIVE_DETECTION

void
IvrDtmfDetector::isdn_audio_eval_dtmf_relative(int* result, dtmf_state *s)
{
    int silence;
    int i;
    int grp[2];
    char what;
    int IVR_what;
    int thresh;
    bool same_tone = false;

    grp[LOGRP] = grp[HIGRP] = -1;
    silence = 0;
    thresh = 0;

#ifdef DTMF_STANDALONE_TEST
    printf("*");
    for (i = 0; i < 8; i++) 
      printf("%8d %8d - ", result[i], 0);
    printf("\n");
#endif

    for (i = 0; i < REL_NCOEFF; i++) {
	if (result[i] > REL_DTMF_TRESH) {
	    if (result[i] > thresh)
		thresh = result[i];
	}
	else if (result[i] < REL_SILENCE_TRESH)
	    silence++;
    }
    if (silence == REL_NCOEFF)
	what = ' ';
    else {
	if (thresh > 0)	{
	    thresh = thresh >> 4;  /* touchtones must match within 12 dB */
	    
	    for (i = 0; i < REL_NCOEFF; i++) {
		if (result[i] < thresh)
		    continue;  /* ignore */
		
		/* good level found. This is allowed only one time per group */
		if (i < REL_NCOEFF / 2) {
		    /* lowgroup*/
		    if (grp[LOGRP] >= 0) {
			// Bad. Another tone found. */
			grp[LOGRP] = -1;
			break;
		    }
		    else
			grp[LOGRP] = i;
		}
		else { /* higroup */
		    if (grp[HIGRP] >= 0) { // Bad. Another tone found. */
			grp[HIGRP] = -1;
			break;
		    }
		    else
			grp[HIGRP] = i - REL_NCOEFF/2;
		}
	    }

	    if ((grp[LOGRP] >= 0) && (grp[HIGRP] >= 0)) {
		what = dtmf_matrix[grp[LOGRP]][grp[HIGRP]];
		IVR_what = IVR_dtmf_matrix[grp[LOGRP]][grp[HIGRP]];
		
		if (what == s->last) {
		    same_tone = true;
//		    what = '.';
		} else if ((s->last != ' ')&&(s->last != '.')) { // no non-DTMF between this and the last DTMF
		    what = '.';
		    s->last = '.';
		}
	    } else
		what = '.';
	}
	else
	    what = '.';
    }
    if ((!same_tone)&&(what != ' ') && (what != '.')) {
#ifndef DTMF_STANDALONE_TEST
      if (destinationEventQueue) {
	DBG("Posting DTMF %d: %c (%d)\n",  s->last_dtmf_sample, what, IVR_what);
 	destinationEventQueue->postEvent(new IvrScriptEvent(IvrScriptEvent::IVR_DTMF, IVR_what));
      } else {
	DBG("DTMF %d: %c (%d) [no script to notify]\n",  s->last_dtmf_sample, what, IVR_what);
      }
      s->last_dtmf_sample=0;
#else
	    printf("dtmf: tt='%c' (%d)\n", what, IVR_what);
#endif


    }
    
    s->last = what;
}

#endif //DTMF_USE_RELATIVE_DETECTION

void
IvrDtmfDetector::isdn_audio_calc_dtmf(dtmf_state *s, dtmf_state* s_rel, pcm* buf, int len)
{
    int i;
    int c;
    
    while (len) {
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
#ifndef DTMF_USE_RELATIVE_DETECTION
	    isdn_audio_goertzel(s->buf, s->result);
	    isdn_audio_eval_dtmf(s->result, s);
#else
	    isdn_audio_goertzel_relative(s->buf, s_rel->result);
            isdn_audio_eval_dtmf_relative(s_rel->result, s_rel);
#endif
	    s->idx = 0;
	}
	len -= c;
    }
}

int IvrDtmfDetector::streamPut(unsigned char* samples, unsigned int size, unsigned int user_ts)
{
  isdn_audio_calc_dtmf(&state, &rel_state, (signed short *)samples, size/2);
  return size;
}

IvrDtmfDetector::IvrDtmfDetector()
#ifndef DTMF_STANDALONE_TEST
  :  destinationEventQueue(0)
#endif
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

#ifndef DTMF_STANDALONE_TEST
void IvrDtmfDetector::setDestinationEventQueue(AmEventQueue* eventQueue) {
  destinationEventQueue = eventQueue;
}
#endif

#ifdef DTMF_STANDALONE_TEST
int main(int argc, char* argv[])
{
  if (argc!=2) {
    printf("Usage: %s <filename>\n"
	   "<filename> is assumed to be 16 bit signed raw audio data,\n"
	   "1 channel, sample rate 8000.\n", argv[0]);
    return 1;
  }

#define pack_size 200 // no effect on detection...
  IvrDtmfDetector det;
  pcm buf[pack_size];
  FILE* fp = fopen(argv[1], "r");
  for (;;) {
    if (fread(buf, 1, pack_size*2, fp)!=pack_size*2) // read samples
      break;
    det.streamPut((unsigned char*) buf, pack_size*2, 0);
  }
  fclose(fp);  
}
#endif
