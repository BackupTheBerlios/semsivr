/*
 * $Id: IvrMediaHandler.h,v 1.1 2004/06/07 13:00:23 sayer Exp $
 * Copyright (C) 2004 Fhg Fokus
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
 
#ifndef _IVR_MEDIA_HANDLER_H_
#define _IVR_MEDIA_HANDLER_H_

#include "AmThread.h"
#include "AmAudio.h"
#include "AmEventQueue.h"

#include "Ivr.h"
#include "IvrDtmfDetector.h"
 
#include "amci/codecs.h"
class IvrPython;

#include <queue>
#include <utility>
#include <string>

using std::auto_ptr;
using std::string;

#define IVR_AUDIO_CODEC CODEC_PCM16 // from amci/codecs.h

#define IVR_AUDIO_CHANNELS      1
#define IVR_AUDIO_RATE       8000
#define IVR_AUDIO_SAMPLE        2

class IvrMediaHandler;

struct IvrMediaEvent: public AmEvent {
    string MediaFile; 
    bool front;

    IvrMediaEvent(int event_id, string MediaFile = "" , bool front = true); 
};

class IvrMediaWrapper : public AmAudioFile {
 public: 
  IvrMediaWrapper(const string& file_fmt);
  ~IvrMediaWrapper();
  int streamPutRaw(unsigned int user_ts, unsigned int size);
  int streamGetRaw(unsigned int user_ts, unsigned int size);  
  void getSamples(unsigned char* dest, int count);
};

// data flow
// rtpstream ==> connector rec ==> MediaHandler 
// MediaHandler ==> connector play ==> rtpstream

class IvrAudioConnector : public AmAudio {
 private:
  IvrMediaHandler* mediaHandler;
  IvrMediaWrapper* activeMedia;
  bool isPlayConnector;
  void refreshFormat();
  void setDefaultFormat();

  bool closed;
    // for recording 
  bool mediaInRunning;
  AmAudioFile* mediaIn;

  // for detection
  bool detectionRunning;
  IvrDtmfDetector* dtmfDetector;
  IvrPython* parent;

 protected:
  int streamGet(unsigned int user_ts, unsigned int size);
  int streamPut(unsigned int user_ts, unsigned int size);
 public: 
  IvrAudioConnector(IvrPython* parent, IvrMediaHandler* mh, bool isPlay);
  ~IvrAudioConnector();
  void setActiveMedia(IvrMediaWrapper* newMedia);

  void startRecording(string& filename);
  void stopRecording();
  void pauseRecording();
  void resumeRecording();
  
  void enableDTMFDetection();//ivr_dtmf_callback_t onDTMFCallback);
  void disableDTMFDetection();
  void pauseDTMFDetection(); // temporarily disable detection
  void resumeDTMFDetection(); // reenable detection
  
  void close();
};

class IvrMediaHandler : public AmEventHandler
{
    void process(AmEvent* event);

 private:
    bool closed;
   
    // AmAudio (mostly AmAudioFile) to be played to caller
    std::deque<IvrMediaWrapper*> mediaOutQueue;
    AmMutex queueMutex;

    IvrPython* parent;

    IvrAudioConnector recordConnector;
    IvrAudioConnector playConnector;

    int enqueueMediaFile(string fileName, bool front = false);
    void emptyMediaQueue();
    
    void startRecording(string& filename);
    void stopRecording();
    void pauseRecording();
    void resumeRecording();


    void enableDTMFDetection();//ivr_dtmf_callback_t onDTMFCallback);
    void disableDTMFDetection();
    void pauseDTMFDetection(); // temporarily disable detection
    void resumeDTMFDetection(); // reenable detection

 public:
    enum Action { 
	IVR_enqueueMediaFile,
	IVR_emptyMediaQueue,
	IVR_startRecording, 
	IVR_stopRecording, 
	IVR_enableDTMFDetection, 
	IVR_disableDTMFDetection,
	IVR_pauseDTMFDetection,
	IVR_resumeDTMFDetection
    };

    AmEventQueue eventQueue;

    IvrMediaHandler(IvrPython* parent_);
    ~IvrMediaHandler();
    
    void close();

    IvrAudioConnector*  getPlayConnector();
    IvrAudioConnector* getRecordConnector();
    
    IvrMediaWrapper* getNewOutMedia();
};

#endif // _IVR_MEDIA_HANDLER_H_
