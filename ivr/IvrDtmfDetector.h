/*
 * $Id: IvrDtmfDetector.h,v 1.7 2004/06/29 15:50:59 sayer Exp $
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

#include "Ivr.h" 
//#include "IvrMediaHandler.h" // for AmAudioIvrInFormat

#include <stdio.h>

class AmAudioIvrInFormat;
// these are returned if the non number keys are pressed

#define DTMF_NPOINTS    205        /* Number of samples for DTMF recognition */
#define SAMPLERATE     8000        
#define PI          3.14159

typedef signed short pcm;    // we get signed 16 bit  

struct dtmf_state {
  char last;
  int idx;
  int buf[DTMF_NPOINTS];
  int result[16];

  dtmf_state();
};

class IvrDtmfDetector 
{
 private:
  void    isdn_audio_goertzel(int *sample, int* result);
  void    isdn_audio_eval_dtmf(int* result, dtmf_state *s);
  void    isdn_audio_calc_dtmf(dtmf_state *s, pcm* buf, int len);

  dtmf_state state;
  AmEventQueue* destinationEventQueue;
 public:
  IvrDtmfDetector();
  ~IvrDtmfDetector();
  void setDestinationEventQueue(AmEventQueue* eventQueue);
  int streamPut(unsigned char* samples, unsigned int size, unsigned int user_ts);
};

#endif // IVR_DTMF_DETECTOR_H
