/*
 * $Id: IvrDtmfDetector.cpp,v 1.2 2004/06/07 21:59:37 sayer Exp $
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

// #define NCOEFF           16     /* number of frequencies to be analyzed       */
// #define DTMF_TRESH    25000     /* above this is dtmf                         */
// #define SILENCE_TRESH   200     /* below this is silence                      */
// #define H2_TRESH      20000     /* 2nd harmonic                               */
// #define AMP_BITS          16     /* bits per sample, reduced to avoid overflow */
// #define LOGRP             0
// #define HIGRP             1

// typedef struct {
// 	int grp;                /* low/high group     */
// 	int k;                  /* k                  */
// 	int k2;                 /* k fuer 2. harmonic */
// } dtmf_t;

// // /* For DTMF recognition:
// //  * 2 * cos(2 * PI * k / N) precalculated for all k
// //  */
// // static int cos2pik[NCOEFF] =
// // {
// // 	55812, 29528, 53603, 24032, 51193, 14443, 48590, 6517,
// // 	38113, -21204, 33057, -32186, 25889, -45081, 18332, -55279
// // };
// //
// // see gen_constants/gen_constants.c for the calculation of these 
// static int cos2pik_240[16] = 
// {
//     58392,   38520,   56755,   34242,   54963,   26655,   53019,   20251,  
//     45111,   -3430,   41243,   -13626,   35693,   -26656,   29752,   -38520  
// };
// static int cos2pik_160[16] = 
// {
//     49833,   10251,   46340,   2573,   42562,   -10252,   38521,   -20252,  
//     22683,   -49833,   15299,   -58392,   5142,   -64728,   -5142,   -64728  
// };

// static dtmf_t dtmf_tones[8] =
// {
// 	{LOGRP, 0, 1},          /*  697 Hz */
// 	{LOGRP, 2, 3},          /*  770 Hz */
// 	{LOGRP, 4, 5},          /*  852 Hz */
// 	{LOGRP, 6, 7},          /*  941 Hz */
// 	{HIGRP, 8, 9},          /* 1209 Hz */
// 	{HIGRP, 10, 11},        /* 1336 Hz */
// 	{HIGRP, 12, 13},        /* 1477 Hz */
// 	{HIGRP, 14, 15}         /* 1633 Hz */
// };

// // returns these values
// static int dtmf_matrix[4][4] =
// {
//     {                1, 2, 3,             IVR_DTMF_A},
//     {                4, 5, 6,             IVR_DTMF_B},
//     {                7, 8, 9,             IVR_DTMF_C},
//     {IVR_DTMF_ASTERISK, 0, IVR_DTMF_HASH, IVR_DTMF_D}
// };


 
// float coeff[] = { 
//    1.7077378035, 1.6452810764, 1.5686869621, 1.4782046080, 1.1641039848, 0.9963701963, 0.7986183763,
//    0.9164258242, 0.7070164084, 0.4608556628, 0.1851755679, -0.6447557807, -1.0071394444, -1.3621084690
// };

// IvrDtmfDetector::IvrDtmfDetector(IvrPython* parent_) //ivr_dtmf_callback_t onDTMFCallback)
//     : /*DTMFCallback(onDTMFCallback),*/ parent(parent_), errorWrongPacketSizePrinted(false)
// {
//   dtmfDetectorState.last = -1;
//   dtmfDetectorState.silence_time = 0;
//   fp = fopen("/tmp/dtmf.raw", "w");
// }

// IvrDtmfDetector::~IvrDtmfDetector()
// {
//     fclose(fp);
// }

// /*
//  * Goertzel algorithm.
//  * See http://ptolemy.eecs.berkeley.edu/~pino/Ptolemy/papers/96/dtmf_ict/
//  * for more info.
//  * Result is stored into an sk_buff and queued up for later
//  * evaluation.
//  */
// void IvrDtmfDetector::goertzel(pcm *sample, int sample_npoints, int* result)
// {
//     int sk,
// 	sk1,
// 	sk2;
//     int k,
// 	n;

//     int* cos2pik = cos2pik_160; // default packet size
//     if (sample_npoints == 240)   // otherwise use the other constants
// 	cos2pik = cos2pik_240;
    
// //     int buf[240];
// //     for (int i=0;i<sample_npoints;i++) // convert to int...
// // 	buf[i] = (sample[i] - (1 << 15)) << 1;

// //     printf("\nsamples: ");
// //     for (int i=0;i<10;i++) {
// // 	printf("%d ",sample[i]);
// //     }

// //     printf("\nbuf: ");
// //     for (int i=0;i<10;i++) {
// // 	printf("%d ",buf[i]);
// //     }
// //     printf("\n");
//     int* buf = (int*)sample;
//     for (k = 0; k < NCOEFF; k++) {
// 	sk = sk1 = sk2 = 0;
// 	for (n = 0; n < sample_npoints; n++) {
// 	    sk = buf[n] + ((cos2pik[k] * sk1) >> 15) - sk2;
// 	    sk2 = sk1;
// 	    sk1 = sk;
// 	}
// 	result[k] =
// 	    ((sk * sk) >> AMP_BITS) -
// 	    ((((cos2pik[k] * sk) >> 15) * sk2) >> AMP_BITS) +
// 	    ((sk2 * sk2) >> AMP_BITS);
//     }
// }

// int IvrDtmfDetector::streamPut(unsigned char* samples, unsigned int size, unsigned int user_ts)
// {
// //   // *TODO: check user_ts and dropped packets!
// //   if ((mediaInFormat->channels != 1)||(mediaInFormat->rate != 8000)||(mediaInFormat->sample != 2)) {
// //     //*TODO support rescaling, channels ...
// //     ERROR("resampling or scaling of input data to DTMF decoder not supported yet!\n");
// //     return -1;
// //   }
  
//     fwrite(samples, 1, size, fp);

//     if (( size != 160*sizeof(pcm) ) && ( size != 240*sizeof(pcm))) {
// 	if (!errorWrongPacketSizePrinted) {
// 	    ERROR("IvrDtmfDetector only support packet size 30 ms and 20 ms.\n");
// 	    errorWrongPacketSizePrinted = true; // only print err msg once
// 	}
//  	return -1;
//     }

//     // calculate the power of the selected frequencies
//     int result[NCOEFF];
//     for (int i=0;i<NCOEFF;i++)
// 	result[i]=0;
//     goertzel((pcm*)samples, size/sizeof(pcm), result); 
    
//     printf("--%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d --\n", 
// 	   result[0],
// 	   result[1],
// 	   result[2],
// 	   result[3],
// 	   result[4],
// 	   result[5],
// 	   result[6],
// 	   result[7],
// 	   result[8],
// 	   result[9],
// 	   result[10],
// 	   result[11],
// 	   result[12],
// 	   result[13],
// 	   result[14],
// 	   result[15]
// 	   );
	
//     int silence = 0;
//     int grp[2];
//     int what;
        
//     grp[LOGRP] = grp[HIGRP] = -2;
//     silence = 0;
//     for (int i = 0; i < 8; i++) {
// 	if ((result[dtmf_tones[i].k] > DTMF_TRESH) &&
// 	    (result[dtmf_tones[i].k2] < H2_TRESH))
// 	    grp[dtmf_tones[i].grp] = (grp[dtmf_tones[i].grp] == -2) ? i : -1;
// 	else if ((result[dtmf_tones[i].k] < SILENCE_TRESH) &&
// 		 (result[dtmf_tones[i].k2] < SILENCE_TRESH))
// 	    silence++;
//     }
//     if (silence == 8)
// 	what = -1;
//     else {
// 	if ((grp[LOGRP] >= 0) && (grp[HIGRP] >= 0)) {
// 	    what = dtmf_matrix[grp[LOGRP]][grp[HIGRP] - 4];
// 	    if (dtmfDetectorState.last != -1 && dtmfDetectorState.last != -2)
// 		dtmfDetectorState.last = what;	/* min. 1 non-DTMF between DTMF */
// 	} else
// 	    what = -2;
//     }
//     if ((what != dtmfDetectorState.last) && (what != -1) && (what != -2)) {
// 	DBG("dtmf: tt='%c'\n", what);
// 	parent->onDTMFEvent(what);
//     } else {
// 	dtmfDetectorState.last = what;
//     }
    
//     return size;
// }


    
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
		IVR_what = IVR_dtmf_matrix[grp[LOGRP]][grp[HIGRP] - 4];
		
		
		if (what == s->last) // current signal continued
		    s->dtmf_signal_length++;
		else { // signal change
		    s->dtmf_signal_length = 0;
		    if ((s->last != ' ')&&(s->last != '.')) { // no non-DTMF between this and the last DTMF
			what = '.';
			s->last = '.';
		    }
		}
		if (s->dtmf_signal_length && (what != '.')&& (what!=' ')) {
		    DBG("DTMF signal length = %d.\n",s->dtmf_signal_length);
		}
	    } else
		what = '.';
	}
	else
	    what = '.';
    }
    if ((what != ' ') && (what != '.') && (s->dtmf_signal_length >= DTMF_SIGNAL_LENGTH_THRESHOLD)) {
	if (!s->wait_after_dtmf) {
	    DBG("dtmf: tt='%c'\n", what);
	    s->wait_after_dtmf = DTMF_WAIT_AFTER_DTMF; // skip next three blocks (3*NPOINTS == 3*93 == 35 ms)
	    parent->onDTMFEvent(IVR_what);    
	}
	
    } 
    s->last = what;
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

 IvrDtmfDetector::IvrDtmfDetector(IvrPython* parent_) //ivr_dtmf_callback_t onDTMFCallback)
     : /*DTMFCallback(onDTMFCallback),*/ parent(parent_), errorWrongPacketSizePrinted(false)
 {
     state = 0;
     state = isdn_audio_dtmf_init(state);
     max_val = 0;
 }

 IvrDtmfDetector::~IvrDtmfDetector()
 {
 }
