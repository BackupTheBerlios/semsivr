/*
 * $Id: IvrMediaHandler.cpp,v 1.3 2004/06/22 14:02:11 sayer Exp $
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
#include "log.h"
#include "IvrMediaHandler.h"
#include "IvrPython.h"


IvrMediaEvent::IvrMediaEvent(int event_id, string MediaFile, bool front) 
    :    AmEvent(event_id), MediaFile(MediaFile), front(front)
{
    DBG("New Media Event: %d, %s\n", event_id, MediaFile.c_str());
}


IvrMediaHandler::IvrMediaHandler(AmEventQueue* scriptEventQueue)
  : closed(false), scriptEventQueue(scriptEventQueue),
    recordConnector(scriptEventQueue, this, false), 
    playConnector(scriptEventQueue, this, true)  
{
}

void IvrMediaHandler::setScriptEventQueue(AmEventQueue* newScriptEventQueue) {
  scriptEventQueue = newScriptEventQueue;
  recordConnector.setScriptEventQueue(newScriptEventQueue);
  playConnector.setScriptEventQueue(newScriptEventQueue);
}

IvrMediaHandler::~IvrMediaHandler()
{
}

int IvrMediaHandler::enqueueMediaFile(string fileName, bool front) {
		IvrMediaWrapper* wav_file = new IvrMediaWrapper(string("Wav"));
		DBG("Queue: enqueuing %d with out = %d.\n", (int)wav_file, (int)wav_file->out.get());
		
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
  
    mediaOutQueue.pop_front();
    if (mediaOutQueue.empty()) {
	DBG("Empty media queue after pop.\n");
	DBG("Posting IvrScriptEvent::IVR_MediaQueueEmpty into scriptEventQueue.\n");
	scriptEventQueue->postEvent(new IvrScriptEvent(IvrScriptEvent::IVR_MediaQueueEmpty)); 
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
IvrAudioConnector::IvrAudioConnector(AmEventQueue* scriptEventQueue, IvrMediaHandler* mh, bool isPlay) 
  : AmAudio(),  scriptEventQueue(scriptEventQueue), mediaHandler(mh),
      isPlayConnector(isPlay), activeMedia(0),
      detectionRunning(false), dtmfDetector(0), closed(false)
{
    setDefaultFormat();
}

IvrAudioConnector::~IvrAudioConnector() 
{
    if (dtmfDetector)
	delete dtmfDetector;
}

void IvrAudioConnector::close() {
    //  if (activeMedia)
    //     activeMedia->close();
    closed = true;
}

void IvrAudioConnector::setScriptEventQueue(AmEventQueue* newScriptEventQueue) {
  scriptEventQueue = newScriptEventQueue;
  if (dtmfDetector)
    dtmfDetector->setDestinationEventQueue(newScriptEventQueue);
}


void IvrAudioConnector::setDefaultFormat() {
    AmAudioSimpleFormat* fmt = new AmAudioSimpleFormat(IVR_AUDIO_CODEC);
    fmt->rate = IVR_AUDIO_RATE;
    fmt->sample = IVR_AUDIO_SAMPLE;
    fmt->channels = IVR_AUDIO_CHANNELS;
    if (isPlayConnector) {
      DBG("Setting in format of play connector to default format.\n");
      in.reset(fmt);
    } else {
      DBG("Setting out format of record connector to default format.\n");
      out.reset(fmt);
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
      in.reset(activeMedia->in.get());
    } else {
      out.reset(activeMedia->out.get());
    }
  } else {
    setDefaultFormat();
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
    //				    DBG("no active mnedia\n");
    return 0; // TODO: check if return 0 is correct
  }
  int ret = activeMedia->streamGetRaw(user_ts, size);
  //		DBG("Got %d.", ret);
  while (ret<0) {
    activeMedia = mediaHandler->getNewOutMedia();
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
   mediaIn = new AmAudioFile("Wav", 1);
   
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
    dtmfDetector = new IvrDtmfDetector(scriptEventQueue);
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
