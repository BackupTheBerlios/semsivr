/*
 * $Id: IvrDtmfDetector.cpp,v 1.1 2004/06/07 13:00:23 sayer Exp $
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
// #define AMP_BITS          9     /* bits per sample, reduced to avoid overflow */
// #define LOGRP             0
// #define HIGRP             1

// typedef struct {
// 	int grp;                /* low/high group     */
// 	int k;                  /* k                  */
// 	int k2;                 /* k fuer 2. harmonic */
// } dtmf_t;

// /* For DTMF recognition:
//  * 2 * cos(2 * PI * k / N) precalculated for all k
//  */
// static int cos2pik[NCOEFF] =
// {
// 	55812, 29528, 53603, 24032, 51193, 14443, 48590, 6517,
// 	38113, -21204, 33057, -32186, 25889, -45081, 18332, -55279
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

// static char dtmf_matrix[4][4] =
// {
// 	{'1', '2', '3', 'A'},
// 	{'4', '5', '6', 'B'},
// 	{'7', '8', '9', 'C'},
// 	{'*', '0', '#', 'D'}
// };

// dtmf_state *
// isdn_audio_dtmf_init(dtmf_state * s)
// {
// 	if (!s)
// 		s = (dtmf_state *) kmalloc(sizeof(dtmf_state), GFP_ATOMIC);
// 	if (s) {
// 		s->idx = 0;
// 		s->last = ' ';
// 	}
// 	return s;
// }
// /*
//  * Goertzel algorithm.
//  * See http://ptolemy.eecs.berkeley.edu/~pino/Ptolemy/papers/96/dtmf_ict/
//  * for more info.
//  * Result is stored into an sk_buff and queued up for later
//  * evaluation.
//  */
// static void
// isdn_audio_goertzel(int *sample, modem_info * info)
// {
// 	int sk,
// 	 sk1,
// 	 sk2;
// 	int k,
// 	 n;
// 	struct sk_buff *skb;
// 	int *result;

// 	skb = dev_alloc_skb(sizeof(int) * NCOEFF);
// 	if (!skb) {
// 		printk(KERN_WARNING
// 		  "isdn_audio: Could not alloc DTMF result for ttyI%d\n",
// 		       info->line);
// 		return;
// 	}
// 	result = (int *) skb_put(skb, sizeof(int) * NCOEFF);
// 	for (k = 0; k < NCOEFF; k++) {
// 		sk = sk1 = sk2 = 0;
// 		for (n = 0; n < DTMF_NPOINTS; n++) {
// 			sk = sample[n] + ((cos2pik[k] * sk1) >> 15) - sk2;
// 			sk2 = sk1;
// 			sk1 = sk;
// 		}
// 		result[k] =
// 		    ((sk * sk) >> AMP_BITS) -
// 		    ((((cos2pik[k] * sk) >> 15) * sk2) >> AMP_BITS) +
// 		    ((sk2 * sk2) >> AMP_BITS);
// 	}
// 	skb_queue_tail(&info->dtmf_queue, skb);
// 	isdn_timer_ctrl(ISDN_TIMER_MODEMREAD, 1);
// }

// void
// isdn_audio_eval_dtmf(modem_info * info)
// {
// 	struct sk_buff *skb;
// 	int *result;
// 	dtmf_state *s;
// 	int silence;
// 	int i;
// 	int di;
// 	int ch;
// 	unsigned long flags;
// 	int grp[2];
// 	char what;
// 	char *p;

// 	while ((skb = skb_dequeue(&info->dtmf_queue))) {
// 		result = (int *) skb->data;
// 		s = info->dtmf_state;
// 		grp[LOGRP] = grp[HIGRP] = -2;
// 		silence = 0;
// 		for (i = 0; i < 8; i++) {
// 			if ((result[dtmf_tones[i].k] > DTMF_TRESH) &&
// 			    (result[dtmf_tones[i].k2] < H2_TRESH))
// 				grp[dtmf_tones[i].grp] = (grp[dtmf_tones[i].grp] == -2) ? i : -1;
// 			else if ((result[dtmf_tones[i].k] < SILENCE_TRESH) &&
// 			      (result[dtmf_tones[i].k2] < SILENCE_TRESH))
// 				silence++;
// 		}
// 		if (silence == 8)
// 			what = ' ';
// 		else {
// 			if ((grp[LOGRP] >= 0) && (grp[HIGRP] >= 0)) {
// 				what = dtmf_matrix[grp[LOGRP]][grp[HIGRP] - 4];
// 				if (s->last != ' ' && s->last != '.')
// 					s->last = what;	/* min. 1 non-DTMF between DTMF */
// 			} else
// 				what = '.';
// 		}
// 		if ((what != s->last) && (what != ' ') && (what != '.')) {
// 			printk(KERN_DEBUG "dtmf: tt='%c'\n", what);
// 			p = skb->data;
// 			*p++ = 0x10;
// 			*p = what;
// 			skb_trim(skb, 2);
// 			if (skb_headroom(skb) < sizeof(isdn_audio_skb)) {
// 				printk(KERN_WARNING
// 				       "isdn_audio: insufficient skb_headroom, dropping\n");
// 				kfree_skb(skb);
// 				return;
// 			}
// 			ISDN_AUDIO_SKB_DLECOUNT(skb) = 0;
// 			ISDN_AUDIO_SKB_LOCK(skb) = 0;
// 			save_flags(flags);
// 			cli();
// 			di = info->isdn_driver;
// 			ch = info->isdn_channel;
// 			__skb_queue_tail(&dev->drv[di]->rpqueue[ch], skb);
// 			dev->drv[di]->rcvcount[ch] += 2;
// 			restore_flags(flags);
// 			/* Schedule dequeuing */
// 			if ((dev->modempoll) && (info->rcvsched))
// 				isdn_timer_ctrl(ISDN_TIMER_MODEMREAD, 1);
// 			wake_up_interruptible(&dev->drv[di]->rcv_waitq[ch]);
// 		} else
// 			kfree_skb(skb);
// 		s->last = what;
// 	}
// }
 
// /*
//  * Decode DTMF tones, queue result in separate sk_buf for
//  * later examination.
//  * Parameters:
//  *   s    = pointer to state-struct.
//  *   buf  = input audio data
//  *   len  = size of audio data.
//  *   fmt  = audio data format (0 = ulaw, 1 = alaw)
//  */
// void
// isdn_audio_calc_dtmf(modem_info * info, unsigned char *buf, int len, int fmt)
// {
// 	dtmf_state *s = info->dtmf_state;
// 	isdn_audio_goertzel(s->buf, info);
	
// }


float coeff[] = { 
   1.7077378035, 1.6452810764, 1.5686869621, 1.4782046080, 1.1641039848, 0.9963701963, 0.7986183763,
   0.9164258242, 0.7070164084, 0.4608556628, 0.1851755679, -0.6447557807, -1.0071394444, -1.3621084690
};

IvrDtmfDetector::IvrDtmfDetector(IvrPython* parent_) //ivr_dtmf_callback_t onDTMFCallback)
  : /*DTMFCallback(onDTMFCallback),*/ parent(parent_)
{
  dtmfDetectorState.last = DSIL;
  dtmfDetectorState.silence_time = 0;
}

IvrDtmfDetector::~IvrDtmfDetector()
{
}

bool IvrDtmfDetector::calculate_power(pcm* data, float* power)
{
  float u0[NUMTONES],u1[NUMTONES],t,in;
  int i,j;
  const pcm middle = 1<<(sizeof(pcm)*8 -1);
  for(j=0; j<NUMTONES; j++) {
    u0[j] = 0.0;
    u1[j] = 0.0;
  }
  for(i=0; i<NUMBER_OF_SAMPLES; i++) {   
    in = (float)data[i] / (float)middle; //for signed char 128.0;
    for(j=0; j<NUMTONES; j++) {
      t = u0[j];
      u0[j] = in + coeff[j] * u0[j] - u1[j];
      u1[j] = t;
    }
  }
  for(j=0; j<NUMTONES; j++)  
    power[j] = u0[j] * u0[j] + u1[j] * u1[j] - coeff[j] * u0[j]* u1[j];
  return(0);
}


int IvrDtmfDetector::decode(pcm *data)
{
  float power[NUMTONES],thresh,thresh2nd,maxpower;
  int on[NUMTONES/2],on_count;
  int rcount, ccount;
  int row, col,i;
  int r[4],c[3];

  calculate_power(data,power);
  for(i=0, maxpower=0.0; i<NUMTONES;i++){
    if(power[i] > maxpower)
      maxpower = power[i];
      //printf("%f***", power[i]);
  }
  //printf("\n");
  if(maxpower < THRESH){  /* silence? */
    //DBG("Real silence %f \n", maxpower);
    return(DSIL);
  }
  thresh = RANGE * maxpower;    /* allowable range of powers */
  thresh2nd = thresh/20;
  for(i=0, on_count=0; i<NUMTONES/2; i++) {
    if(power[i] > thresh && power[i+NUMTONES/2]> thresh2nd) {    /* proof if frequence and its 2nd harmonic present*/
      on[i] = 1;
      on_count ++;
      //DBG("%i\n", i);
    } else
      on[i] = 0;
  }

  if(on_count == 2) {
/*    if(on[TON1] && on[TON2])
      return(DDT);
    if(on[TON2] && on[TON3])
      return(DRING);
    if(on[TON3] && on[TON4])
      return(DBUSY);
*/
    //c[0]= on[C1]; c[1]= on[C2]; c[2]= on[C3]; 
    //r[0]= on[R1]; r[1]= on[R2]; r[2]= on[R3]; r[3]= on[R4];
    c[0]=on[4]; c[1]=on[5]; c[2]= on[6];
    r[0]= on[0]; r[1]= on[1]; r[2]= on[2]; r[3]= on[3];

    for(i=0, rcount=0; i<4; i++) {
      if(r[i]) {
        rcount++;
        row = i;
      }
    }
    for(i=0, ccount=0; i<3; i++) {
      if(c[i]) {
        ccount++;
        col = i;
      }
    }
    if(rcount==1 && ccount==1) {   //DTMF
        if(row == 3 && col == 0 )
           return(DSTAR);
        if(row == 3 && col == 2 )
           return(DNUM);
        if(row == 3)
           return(D0);
        if(row == 0 && col == 2) {   /* DTMF 3 conflicts with MF 7 */
            return(D3);
        } else
          return(D1 + col + row*3);
    }
    return(-1);
  }
  if(on_count == 0)
    return(DSIL);
  return(-1);
}

int IvrDtmfDetector::streamPut(unsigned char* samples, unsigned int size, unsigned int user_ts)
{
//   // *TODO: check user_ts and dropped packets!
//   if ((mediaInFormat->channels != 1)||(mediaInFormat->rate != 8000)||(mediaInFormat->sample != 2)) {
//     //*TODO support rescaling, channels ...
//     ERROR("resampling or scaling of input data to DTMF decoder not supported yet!\n");
//     return -1;
//   }
  
  if ( size < NUMBER_OF_SAMPLES*sizeof(pcm)){
      ERROR("IvrDtmfDetector::streamPut: frame is too small:%d ( <240 samples)",size/sizeof(pcm));
      return -1;
  }
  int x = decode((pcm*)samples);
  if(x >= 0) {
    if(x == DSIL)
      dtmfDetectorState.silence_time += (dtmfDetectorState.silence_time>=0)?1:0 ;
    else
      dtmfDetectorState.silence_time= 0;
    if(dtmfDetectorState.silence_time == FLUSH_TIME) {
      dtmfDetectorState.silence_time= -1; // stops counting 
    }
    if(x != DSIL && x != dtmfDetectorState.last &&
       (dtmfDetectorState.last == DSIL || dtmfDetectorState.last==D24 || dtmfDetectorState.last == D26 ||
	dtmfDetectorState.last == D2426 || dtmfDetectorState.last == DDT || dtmfDetectorState.last == DBUSY ||
	dtmfDetectorState.last == DRING) )  {
      // valid DTMF detected! 
      //DTMFCallback(x);
      parent->onDTMFEvent(x);
    }
    dtmfDetectorState.last  = x;
  }
  return size;
}


