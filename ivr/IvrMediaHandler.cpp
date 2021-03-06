/*
 * $Id: IvrMediaHandler.cpp,v 1.20 2004/07/29 10:01:07 sayer Exp $
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
#include "IvrPython.h"
#include "IvrMediaHandler.h"
#include "log.h"

// uncomment the following line if you have 
// applied the AmAudio.formatlock.patch to answer_machine directory
// you can get the patch from http://developer.berlios.de/patch/?group_id=653
//#define LOCK_INOUT_FMT

IvrMediaHandler::IvrMediaHandler()
  : closed(false),
    recordConnector(this, false), 
    playConnector(this, true)  
{
}

int IvrMediaHandler::registerForeignEventQueue(AmEventQueue* newScriptEventQueue) {
  int ret = 0;
  //  mutexScriptEventQueue.lock();
  scriptEventQueue = newScriptEventQueue;
  recordConnector.setScriptEventQueue(newScriptEventQueue);
  playConnector.setScriptEventQueue(newScriptEventQueue);
  //   mutexScriptEventQueue.unlock();
  return ret;
}

void IvrMediaHandler::unregisterForeignEventQueue() {
  mutexScriptEventQueue.lock();
  scriptEventQueue = 0;
  recordConnector.setScriptEventQueue(0);
  playConnector.setScriptEventQueue(0);
  mutexScriptEventQueue.unlock();
}

IvrMediaHandler::~IvrMediaHandler()
{
  DBG("Media Handler  being destroyed...\n");
  emptyMediaQueue();
}

int IvrMediaHandler::enqueueMediaFile(string fileName, bool front, bool loop) {
  IvrMediaWrapper* wav_file = new IvrMediaWrapper(string("Wav"));
  //  DBG("Queue: enqueuing %d with out = %d.\n", (int)wav_file, (int)wav_file->out.get());
  wav_file->loop.set(loop);
  if(wav_file->open(fileName.c_str(),AmAudioFile::Read)){
    ERROR("IvrMediaHandler::enqueueMediaFile: Cannot open file %s\n", fileName.c_str());
    return -1;
  }
  
  if (front || mediaOutQueue.empty())
    playConnector.setActiveMedia(wav_file);
  
  if (front) {
    mediaOutQueue.push_front(wav_file);
  } else {
    mediaOutQueue.push_back(wav_file);
  }
  return 0; //ok
}

int IvrMediaHandler::emptyMediaQueue() {
  playConnector.setActiveMedia(0); 
  // we own all the MediaWrappers
  for (std::deque<IvrMediaWrapper*>::iterator it = mediaOutQueue.begin(); 
       it != mediaOutQueue.end(); it++) {
    delete *it;
  }
  mediaOutQueue.clear();
  return 0;
}

int IvrMediaHandler::startRecording(string& filename) {
    return recordConnector.startRecording(filename);
}

int IvrMediaHandler::stopRecording() {
    return recordConnector.stopRecording();
}

int IvrMediaHandler::pauseRecording() {
    return recordConnector.pauseRecording();   
}

int IvrMediaHandler::resumeRecording() {
    return recordConnector.resumeRecording();
}

int IvrMediaHandler::enableDTMFDetection(){ 
  return recordConnector.enableDTMFDetection();
}

int IvrMediaHandler::disableDTMFDetection() {
    return recordConnector.disableDTMFDetection();
}

int IvrMediaHandler::pauseDTMFDetection() {
    return recordConnector.pauseDTMFDetection();
}

int IvrMediaHandler::resumeDTMFDetection() {
    return recordConnector.resumeDTMFDetection();
}

void IvrMediaHandler::close(){
  DBG("IvrMediaHandler::close(). closing record connector...\n");
  recordConnector.close();
  DBG("closing play connector...");
  playConnector.close();
  DBG("done.");
//   detectionRunning = false;
//    if (dtmfDetector) {
//     delete dtmfDetector;
//     dtmfDetector = 0;
//   }
		// *TODO: delete mediaIn? 
  closed = true;
}

IvrAudioConnector* IvrMediaHandler::getPlayConnector() {
    return &playConnector;
}

IvrAudioConnector* IvrMediaHandler::getRecordConnector() {
    return &recordConnector;
}

// this is called by the out connector
IvrMediaWrapper* IvrMediaHandler::getNewOutMedia() { 
  if (mediaOutQueue.empty()) {
    DBG("Empty queue.\n");
    return 0; // return -1;
  }
  
  if (!mediaOutQueue.empty()) {
    delete mediaOutQueue.front();
    mediaOutQueue.pop_front();
  }

  if (mediaOutQueue.empty()) {
	DBG("Empty media queue after pop.\n");
	if (scriptEventQueue) {
	  DBG("Posting IvrScriptEvent::IVR_MediaQueueEmpty into scriptEventQueue.\n");
	  scriptEventQueue->postEvent(new IvrScriptEvent(IvrScriptEvent::IVR_MediaQueueEmpty)); 
	} else {
	  DBG("no scriptEventQueue to notify available.\n");
	}
	  
	// TODO: wait until event processed? 
	if (mediaOutQueue.empty())
	    return 0;
	// if queue filled in onEmpty callback return new 
    }
    IvrMediaWrapper* res = mediaOutQueue.front();

    return res;
}

/* the IvrMediaWrapper wraps so far AmAudio only */
IvrMediaWrapper::IvrMediaWrapper(const string& file_fmt) 
		: AmAudioFile(file_fmt, -1)
{
}

IvrMediaWrapper::~IvrMediaWrapper() {
}

int IvrMediaWrapper::streamPutRaw(unsigned int user_ts, unsigned int size) {
//  DBG("IvrMediaWrapper::streamPutRaw(%d, %d)", user_ts, size);
		return AmAudioFile::streamPut(user_ts, size);
}

int IvrMediaWrapper::streamGetRaw(unsigned int user_ts, unsigned int size) {
//  DBG("IvrMediaWrapper::streamGetRaw(%d, %d)", user_ts, size);
		return AmAudioFile::streamGet(user_ts, size);
}

void IvrMediaWrapper::getSamples(unsigned char* dest, int count) {
		memcpy(dest, (unsigned char*)samples, count);
}


/* IvrAudioConnector connects the MediaHandler to RtpStream. 
 * 
 */
IvrAudioConnector::IvrAudioConnector(IvrMediaHandler* mh, bool isPlay) 
  : AmAudio(),  scriptEventQueue(0), mediaHandler(mh),
      isPlayConnector(isPlay), activeMedia(0),
    detectionRunning(false), dtmfDetector(0), closed(false),
    mediaInRunning(false), mediaIn(0), 
    myInternalFormat(new AmAudioSimpleFormat(IVR_AUDIO_CODEC))
{
    myInternalFormat->rate = IVR_AUDIO_RATE;
    myInternalFormat->sample = IVR_AUDIO_SAMPLE;
    myInternalFormat->channels = IVR_AUDIO_CHANNELS;

    setDefaultFormat();
}

IvrAudioConnector::~IvrAudioConnector() 
{
  close();
  // we do not own the AmAudioFormat !
  if (isPlayConnector) {
    in.release();
    delete myInternalFormat;
  } else {
    out.release(); // internal fmt of rec destroyed by close
  }
  
  if (dtmfDetector)
    delete dtmfDetector;
}

void IvrAudioConnector::close() {
  if (!closed) {
    if ((!isPlayConnector) && mediaIn) { // recording file is ours
      mediaIn->close();
      mediaIn->in.release();
      delete mediaIn;
      mediaIn = 0;
    }
    closed = true;
  }
}

void IvrAudioConnector::setScriptEventQueue(AmEventQueue* newScriptEventQueue) {
  scriptEventQueue = newScriptEventQueue;
  if (dtmfDetector)
    dtmfDetector->setDestinationEventQueue(newScriptEventQueue);
}


void IvrAudioConnector::setDefaultFormat() {
    if (isPlayConnector) {
      DBG("Setting in format of play connector to default format.\n");
#ifdef LOCK_INOUT_FMT
      lockInFmt();
#endif
      in.release();
      in.reset(myInternalFormat);
#ifdef LOCK_INOUT_FMT
      unlockInFmt();
#endif
    } else {
      DBG("Setting out format of record connector to default format.\n");
#ifdef LOCK_INOUT_FMT
      lockOutFmt();
#endif
      out.release();
      out.reset(myInternalFormat);
#ifdef LOCK_INOUT_FMT
      unlockOutFmt();
#endif
    }
}

void IvrAudioConnector::setActiveMedia(IvrMediaWrapper* newMedia) {
    DBG("setting active media...\n");
    activeMedia = newMedia;
    refreshFormat();
}

void IvrAudioConnector::refreshFormat() {
  if (activeMedia) {
    if (isPlayConnector) {
#ifdef LOCK_INOUT_FMT
      lockInFmt();
#endif
      in.release(); // we don't own the fmt!
      in.reset(activeMedia->in.get());
#ifdef LOCK_INOUT_FMT
      unlockInFmt();
#endif
    } else {
#ifdef LOCK_INOUT_FMT
      lockOutFmt();
#endif
      out.release(); // we don't own the fmt!
      out.reset(activeMedia->out.get());
#ifdef LOCK_INOUT_FMT
      unlockOutFmt();
#endif
    }
  } else {
    setDefaultFormat();
  }
}

void IvrAudioConnector::unsafe_refreshFormat() {
  if (activeMedia) {
    if (isPlayConnector) {
      in.release(); // we don't own the fmt!
      in.reset(activeMedia->in.get());
    } else {
      out.release(); // we don't own the fmt!
      out.reset(activeMedia->out.get());
    }
  } else {
    unsafe_setDefaultFormat();
  }
}

void IvrAudioConnector::unsafe_setDefaultFormat() {
    if (isPlayConnector) {
      DBG("Setting in format of play connector to default format.\n");
      in.release();
      in.reset(myInternalFormat);
    } else {
      DBG("Setting out format of record connector to default format.\n");
      out.release();
      out.reset(myInternalFormat);
    }
}

int IvrAudioConnector::streamGet(unsigned int user_ts, unsigned int size) {
  //  DBG("IvrAudioConnector::streamGet(%d, %d)\n", user_ts, size);
  if (!isPlayConnector) {
    ERROR("streamGet of IvrAudioConnector with type \"record\" called. There must be something wrong here.\n");
    return -1;
  }
  
  if (closed)
    return -1;
  
  if (!activeMedia) {
    //	 DBG("no active mnedia\n");
    return 0; // TODO: check if return 0 is correct
  }
  
//   if (100 == deb_cnt++) {
//       DBG("usleeping 500000\n");
//       usleep(500000);
//       DBG("usleeping 500000 done\n");
//   }

  int ret = activeMedia->streamGetRaw(user_ts, size);
  //		DBG("Got %d.", ret);
  while (ret<0) {
    activeMedia = mediaHandler->getNewOutMedia();
    unsafe_refreshFormat(); // need to use unsafe function here as we are called from convert
    if (!activeMedia)
      return 0;

    ret = activeMedia->streamGetRaw(user_ts, size);
  }
  
  if (ret>0) {
    activeMedia->getSamples((unsigned char*)samples, ret );
  }
  return ret;
}

int IvrAudioConnector::streamPut(unsigned int user_ts, unsigned int size) {
  if (closed)
    return -1;
  
  if (isPlayConnector) {
    ERROR("streamPut of IvrAudioConnector with type \"play\" called. There must be something wrong here.\n");
    return -1;
  }
  
  int ret = 0;    //*TODO: -1 ?
  
  if (detectionRunning && dtmfDetector) {
    //DBG("IvrMediaHandler::streamPut : detecting DTMF.\n");
    dtmfDetector->streamPut((unsigned char*)samples, size, user_ts );
    ret = size;
  }
    
  if (mediaIn && mediaInRunning) {
    ret = mediaIn->put(user_ts, (unsigned char*)samples, size/(out->sample * out->channels));
  }
  return ret;
}


int IvrAudioConnector::startRecording(string& filename) {  
  if (mediaIn) {
//     ERROR("already recording. Call Stop Record first.\n");
//     return 1;
    DBG("closing previous recording...\n");
    stopRecording();
  }

  
   
  unsigned int dot_pos = filename.rfind('.');
  string ext = filename.substr(dot_pos+1);

  // this could be done a bit nicer...
  if ((ext == "mp3") || (ext == "MP3")) {
      mediaIn = new AmAudioFile("MP3", 1);
  } else {
      mediaIn = new AmAudioFile("Wav", 1);
  }
  
  if(mediaIn->open(filename.c_str(),AmAudioFile::Write)){
    ERROR("AmRtpStream::record(): Cannot open file\n");
    delete mediaIn;
    mediaIn = 0;
    return -1;
  }
  mediaIn->in.reset(out.get()); // transfer our out format to the file's in format 
  // so mediaIn->put will convert from our format to mediaIn's format
  mediaInRunning = true;
  return 0;
}

int IvrAudioConnector::stopRecording() { 
    mediaInRunning = false;
    if (mediaIn) {
	mediaIn->close();
	mediaIn->in.release();
	delete mediaIn;
	mediaIn = 0;
    }
    refreshFormat();
    return 0;
}

int IvrAudioConnector::pauseRecording() { 
    mediaInRunning = false;
    return 0;
}

int IvrAudioConnector::resumeRecording() { 
    if (mediaIn)
	mediaInRunning = true;
    else 
      return -1; // error
    return 0;
}

int IvrAudioConnector::enableDTMFDetection() {
  DBG("record connnector enabling dtmf detection...\n");
  if (!dtmfDetector)
      dtmfDetector = new IvrDtmfDetector();
    if (!scriptEventQueue) {
      DBG("missing script event ueue!!!!!\n");
    } 
    dtmfDetector->setDestinationEventQueue(scriptEventQueue);
    detectionRunning = true;
    return 0;
}

int IvrAudioConnector::disableDTMFDetection() {
    detectionRunning = false;    
    if (dtmfDetector)
      delete dtmfDetector;
    dtmfDetector = 0;
    return 0;
}

int IvrAudioConnector::pauseDTMFDetection() {
    if (!detectionRunning)
      return -1;
    detectionRunning = false;
    return 0;
}

int IvrAudioConnector::resumeDTMFDetection() {
    if (dtmfDetector)
	detectionRunning = true;
    else 
      return -1;
    return 0;
}
