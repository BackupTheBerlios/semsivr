/*
 * $Id: IvrDtmfDetector.h,v 1.4 2004/06/18 19:51:59 sayer Exp $
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
#ifndef IVR_DTMF_DETECTOR_H
#define IVR_DTMF_DETECTOR_H

#include <stdio.h>
#include "Ivr.h" 
//#include "IvrMediaHandler.h" // for AmAudioIvrInFormat

class AmAudioIvrInFormat;
// these are returned if the non number keys are pressed

#define DTMF_NPOINTS 93        /* Number of samples for DTMF recognition */
#define DTMF_SIGNAL_LENGTH_THRESHOLD  2 // dtmf must be longer than or equal to 2*93 samples 
#define DTMF_DIFFERENT_SIGNAL_MINIMUM 2 // after dtmf minumum of x frames other signal must be detected
#define DTMF_WAIT_AFTER_DTMF 0

typedef struct dtmf_state {
    char last;
    int  dtmf_signal_length;
    int  wait_after_dtmf;
    char last_detected;
    int  different_dtmf_length;
    int idx;
    int buf[DTMF_NPOINTS];
} dtmf_state;


/* values returned by detect
 *  0-9     DTMF 0 through 9 or MF 0-9
 *  10-11   DTMF *, #
 *  12-15   DTMF A,B,C,D
 *  16-20   MF last column: C11, C12, KP1, KP2, ST
 *  21      2400
 *  22      2600
 *  23      2400 + 2600
 *  24      DIALTONE
 *  25      RING
 *  26      BUSY
 *  27      silence
 *  -1      invalid
 */
typedef unsigned short pcm;    

class IvrDtmfDetector 
{
 private:
//  void goertzel(pcm *sample, int sample_npoints, int* result);

//  ivr_dtmf_callback_t DTMFCallback;
  
/*   struct DetectorState { */
/*     int last; */
/*     int silence_time; */
/*   }; */
  
/*   DetectorState dtmfDetectorState; */

  dtmf_state *
    isdn_audio_dtmf_init(dtmf_state * s);
  void    isdn_audio_goertzel(int *sample, int* result);
  void    isdn_audio_eval_dtmf(int* result, dtmf_state *s);
  void    isdn_audio_calc_dtmf(dtmf_state *s, unsigned short *buf, int len, int* result);
  int max_val;

  dtmf_state* state;
  int result[16];

  bool errorWrongPacketSizePrinted; 
  FILE* fp;
  AmEventQueue* destinationEventQueue;
 public:
  IvrDtmfDetector(AmEventQueue* destinationEventQueue);
  ~IvrDtmfDetector();
  int streamPut(unsigned char* samples, unsigned int size, unsigned int user_ts);
};

#endif // IVR_DTMF_DETECTOR_H
