/*
 * $Id: IvrDtmfDetector.cpp,v 1.4 2004/06/18 19:51:59 sayer Exp $
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
 */

#include <math.h>
#include <sys/time.h>
#include "log.h"
#include "IvrDtmfDetector.h"
#include "IvrMediaHandler.h"
#include "IvrPython.h"


    
// -------------------------------------------------------------------------------------------
#define IVR_DTMF_ASTERISK 10
#define IVR_DTMF_HASH     11
#define IVR_DTMF_A        12
#define IVR_DTMF_B        13 
#define IVR_DTMF_C        14 
#define IVR_DTMF_D        15
/* // returns these values */

static int IVR_dtmf_matrix[4][4] =
{
    {                1, 2, 3,             IVR_DTMF_A},
    {                4, 5, 6,             IVR_DTMF_B},
    {                7, 8, 9,             IVR_DTMF_C},
    {IVR_DTMF_ASTERISK, 0, IVR_DTMF_HASH, IVR_DTMF_D}
};


#define NCOEFF            8     /* number of frequencies to be analyzed       */
#define DTMF_TRESH     4000     /* above this is dtmf                         */
#define SILENCE_TRESH   200     /* below this is silence                      */
#define AMP_BITS          9     /* bits per sample, reduced to avoid overflow */
#define LOGRP             0
#define HIGRP             1

typedef struct {
	int grp;                /* low/high group     */
	int k;                  /* k                  */
	int k2;                 /* k fuer 2. harmonic */
} dtmf_t;

/* For DTMF recognition:
 * 2 * cos(2 * PI * k / N) precalculated for all k
 */
static int cos2pik[NCOEFF] =
{
    55813, 53604, 51193, 48591,      // low group
    38114, 33057, 25889, 18332       // high group
};

static dtmf_t dtmf_tones[8] =
{
	{LOGRP, 0, 1},          /*  697 Hz */
	{LOGRP, 2, 3},          /*  770 Hz */
	{LOGRP, 4, 5},          /*  852 Hz */
	{LOGRP, 6, 7},          /*  941 Hz */
	{HIGRP, 8, 9},          /* 1209 Hz */
	{HIGRP, 10, 11},        /* 1336 Hz */
	{HIGRP, 12, 13},        /* 1477 Hz */
	{HIGRP, 14, 15}         /* 1633 Hz */
};

static char dtmf_matrix[4][4] =
{
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};

dtmf_state *
IvrDtmfDetector::isdn_audio_dtmf_init(dtmf_state * s)
{
	if (!s)
		s = (dtmf_state *) malloc(sizeof(dtmf_state));
	if (s) {
		s->idx = 0;
		s->last = ' ';
		s->wait_after_dtmf = 0;
		s->dtmf_signal_length = 0;
		s->different_dtmf_length = 0;
		s->last_detected = '.';
	}
	return s;
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
    int thresh;

    grp[LOGRP] = grp[HIGRP] = -1;
    silence = 0;
    thresh = 0;

    for (i = 0; i < NCOEFF; i++) {
	if (result[i] > DTMF_TRESH) {
	    if (result[i] > thresh)
		thresh = result[i];
	}
	else if (result[i] < SILENCE_TRESH)
	    silence++;
    }
    if (silence == NCOEFF)
	what = ' ';
    else {
	if (thresh > 0)	{
	    thresh = thresh >> 4;  /* touchtones must match within 12 dB */

	    for (i = 0; i < NCOEFF; i++) {
		if (result[i] < thresh)
		    continue;  /* ignore */
		
		/* good level found. This is allowed only one time per group */
		if (i < NCOEFF / 2) {
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
			grp[HIGRP] = i - NCOEFF/2;
		}
	    }

	    if ((grp[LOGRP] == 1) && (grp[HIGRP] == -1)) { // fix for 4: 4 and 7 over thresh
		if ((result[4]>=thresh)&&(result[7]>=thresh) && ((result[4] >> 2) > result[7])) {
		    grp[HIGRP] = 0;
		}
	    }

	    if ((grp[LOGRP] >= 0) && (grp[HIGRP] >= 0)) {
		what = dtmf_matrix[grp[LOGRP]][grp[HIGRP]];
		IVR_what = IVR_dtmf_matrix[grp[LOGRP]][grp[HIGRP]];
		
		if (what == s->last) // current signal continued
		    s->dtmf_signal_length++;
		else { // signal change
		    s->dtmf_signal_length = 0;
		    if ((s->last != ' ')&&(s->last != '.')) { // no non-DTMF between this and the last DTMF
			what = '.';
			s->last = '.';
		    }
		}
		// if (s->dtmf_signal_length && (what != '.')&& (what!=' ')) {
// 		    DBG("DTMF signal length = %d. different length = %d, last = %c\n",s->dtmf_signal_length, 
// 			s->different_dtmf_length, s->last_detected);
// 		}
	    } else
		what = '.';
	}
	else
	    what = '.';
    }
    if ((what != ' ') && (what != '.') && (s->dtmf_signal_length >= DTMF_SIGNAL_LENGTH_THRESHOLD)) {
	if ((!s->wait_after_dtmf) && (s->different_dtmf_length >= DTMF_DIFFERENT_SIGNAL_MINIMUM)) {
	    DBG("dtmf: tt='%c'\n", what);
	    s->wait_after_dtmf = DTMF_WAIT_AFTER_DTMF; // skip next three blocks (3*NPOINTS == 3*93 == 35 ms)
	    s->different_dtmf_length = 0;
	    s->last_detected = what;
	    destinationEventQueue->postEvent(new IvrScriptEvent(IvrScriptEvent::IVR_DTMF, IVR_what));
	}
	
    } 
    s->last = what;
    if (what != s->last_detected) {
	if (s->different_dtmf_length <= 10) {
	    s->different_dtmf_length++;
	    //DBG("%d\n ", s->different_dtmf_length);
	}
    }
    else {
	if (s->different_dtmf_length <= 10)
	    s->different_dtmf_length = 0;
    }


//    DBG("%c",what);
    if (s->wait_after_dtmf) {
	s->wait_after_dtmf--;
//	DBG("s->wait_after_dtmf is now %d\n", s->wait_after_dtmf);
    }
}

/*
 * Decode DTMF tones, queue result in separate sk_buf for
 * later examination.
 * Parameters:
 *   s    = pointer to state-struct.
 *   buf  = input audio data
 *   len  = size of audio data.
 *   fmt  = audio data format (0 = ulaw, 1 = alaw)
 */
void
IvrDtmfDetector::isdn_audio_calc_dtmf(dtmf_state *s, unsigned short *buf, int len, int* result)
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
	    s->buf[s->idx++] = 
		((*buf++) -  (1 << 15)) >> (15 - AMP_BITS);
	    
	}
	if (s->idx == DTMF_NPOINTS) {
	    isdn_audio_goertzel(s->buf, result);
	    // for (i=0;i<16;i++)
// 		printf("%7d ", result[i]);	    
// 	    printf ("\n");
	    s->idx = 0;
	    isdn_audio_eval_dtmf(result, s);
	}
	len -= c;
    }
}


int IvrDtmfDetector::streamPut(unsigned char* samples, unsigned int size, unsigned int user_ts)
{
    isdn_audio_calc_dtmf(state, (unsigned short *)samples, size/2, result);
    return size;
}

 IvrDtmfDetector::IvrDtmfDetector(AmEventQueue* destinationEventQueue)
     :  destinationEventQueue(destinationEventQueue), errorWrongPacketSizePrinted(false)
 {
     state = 0;
     state = isdn_audio_dtmf_init(state);
     max_val = 0;
 }

 IvrDtmfDetector::~IvrDtmfDetector()
 {
 }
