/*
 * $Id: IvrDtmfDetector.h,v 1.10 2004/07/25 22:00:33 sayer Exp $
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

#ifndef DTMF_STANDALONE_TEST
#include "Ivr.h" 
//#include "IvrMediaHandler.h" // for AmAudioIvrInFormat
class AmAudioIvrInFormat;
#else
#include <stdio.h>
#endif

// these are returned if the non number keys are pressed

#define DTMF_NPOINTS    205        /* Number of samples for DTMF recognition */
#define REL_DTMF_NPOINTS    205        /* Number of samples for DTMF recognition */
#define SAMPLERATE     8000        
#define PI          3.14159
#define DTMF_INTERVAL     20

typedef signed short pcm;    // we get signed 16 bit  

struct dtmf_state {
  char last;
  int last_dtmf_sample;
  int idx;
  int buf[DTMF_NPOINTS];
  int result[16];
	char rel_last;
  dtmf_state();
};

class IvrDtmfDetector 
{
 private:
#ifndef DTMF_USE_RELATIVE_DETECTION
  void    isdn_audio_goertzel(int *sample, int* result);
  void    isdn_audio_eval_dtmf(int* result, dtmf_state *s);
#else // DTMF_USE_RELATIVE_DETECTION
  void    isdn_audio_goertzel_relative(int *sample, int* result);
  void    isdn_audio_eval_dtmf_relative(int* result, dtmf_state *s);
#endif //DTMF_USE_RELATIVE_DETECTION

  void    isdn_audio_calc_dtmf(dtmf_state *s, dtmf_state *s_rel,  pcm* buf, int len);

  dtmf_state state;
  dtmf_state rel_state;
#ifndef DTMF_STANDALONE_TEST
  AmEventQueue* destinationEventQueue;
#endif

 public:
  IvrDtmfDetector();
  ~IvrDtmfDetector();
  int streamPut(unsigned char* samples, unsigned int size, unsigned int user_ts);

#ifndef DTMF_STANDALONE_TEST
  void setDestinationEventQueue(AmEventQueue* eventQueue);
#endif
};

#endif // IVR_DTMF_DETECTOR_H
