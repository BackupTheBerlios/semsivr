/*
 * $Id: IvrDtmfDetector.h,v 1.1 2004/06/07 13:00:23 sayer Exp $
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

#define NUMBER_OF_SAMPLES        240

#define TON1    0    // 350 dialtone 
#define TON2    1    // 440 ring, dialtone 
#define TON3    2    // 480 ring, busy 
#define TON4    3    // 620 busy 

#define R1    4    // 697, dtmf row 1 
#define R2    5    // 770, dtmf row 2 
#define R3    6    // 852, dtmf row 3 
#define R4    8    // 941, dtmf row 4 
#define C1   10    // 1209, dtmf col 1
#define C2   12    // 1336, dtmf col 2
#define C3   13    // 1477, dtmf col 3
#define C4   14    // 1633, dtmf col 4

#define B1    4    // 700, blue box 1 
#define B2    7    // 900, bb 2 
#define B3    9    // 1100, bb 3
#define B4   11    // 1300, bb4
#define B5   13    // 1500, bb5
#define B6   15    // 1700, bb6
#define B7   16    // 2400, bb7
#define B8   17    // 2600, bb8

#define NUMTONES 14

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
#define D0    0
#define D1    1
#define D2    2
#define D3    3
#define D4    4
#define D5    5
#define D6    6
#define D7    7
#define D8    8
#define D9    9
#define DSTAR 10
#define DNUM  11
#define DA    12
#define DB    13
#define DC    14
#define DD    15
#define DC11  16
#define DC12  17
#define DKP1  18
#define DKP2  19
#define DST   20
#define D24   21
#define D26   22
#define D2426 23                     
#define DDT   24
#define DRING 25
#define DBUSY 26
#define DSIL  27


#define RANGE  0.1 //0.1           // any thing higher than RANGE*peak is "on"
#define THRESH 500.0 //100.0        // minimum level for the tone 
#define FLUSH_TIME 100       // 100 frames = 3 seconds 

typedef unsigned short pcm;    

class IvrDtmfDetector 
{
 private:
  // decode numbers from data
  int decode(pcm* data);
  // calculates power of each ton according to a goertzel algorithm
  bool calculate_power(pcm* data, float* power);
  
  ivr_dtmf_callback_t DTMFCallback;
  
  struct DetectorState {
    int last;
    int silence_time;
  };
  
  DetectorState dtmfDetectorState;
  IvrPython* parent;

 public:
  IvrDtmfDetector(IvrPython* parent_);// ivr_dtmf_callback_t onDTMFCallback);
  ~IvrDtmfDetector();
  int streamPut(unsigned char* samples, unsigned int size, unsigned int user_ts);
};

#endif // IVR_DTMF_DETECTOR_H
