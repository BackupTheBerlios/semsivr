/*
 * $Id: IvrMediaHandler.cpp,v 1.1 2004/06/07 13:00:23 sayer Exp $
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


IvrMediaHandler::IvrMediaHandler(IvrPython* parent_)
		: closed(false), parent(parent_), 
		  recordConnector(parent_, this, false), playConnector(parent_, this, true),
		  eventQueue(this)
    
{
}

IvrMediaHandler::~IvrMediaHandler()
{
}

void IvrMediaHandler::process(AmEvent* event) {

    DBG("Processing event...\n");
    IvrMediaEvent* evt = dynamic_cast<IvrMediaEvent*>(event);
    if (!evt) {
	ERROR("IvrMediaHandler got event with wrong type.\n");
	return;
    }

    switch (evt->event_id) {
	case IVR_enqueueMediaFile: {
	    enqueueMediaFile(evt->MediaFile, evt->front);
	}; break;
	case IVR_emptyMediaQueue: {
	    emptyMediaQueue();
	}; break;
	case IVR_startRecording: {
	    startRecording(evt->MediaFile);
	}; break;
	case IVR_stopRecording: {
	    stopRecording();
	}; break;
	case IVR_enableDTMFDetection: {
	    enableDTMFDetection();
	}; break;
	case IVR_disableDTMFDetection: {
	    disableDTMFDetection();
	}; break;
	case IVR_resumeDTMFDetection: {
	    resumeDTMFDetection();
	}; break;
	case IVR_pauseDTMFDetection: {
	    pauseDTMFDetection();
	}; break;
    }
    event->processed = true;
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

void IvrMediaHandler::emptyMediaQueue() {
    playConnector.setActiveMedia(0);
    mediaOutQueue.clear();
}

void IvrMediaHandler::startRecording(string& filename) {
    recordConnector.startRecording(filename);
}

void  IvrMediaHandler::stopRecording() {
    recordConnector.stopRecording();
}

void  IvrMediaHandler::pauseRecording() {
    recordConnector.pauseRecording();   
}
void  IvrMediaHandler::resumeRecording() {
    recordConnector.resumeRecording();
}


void IvrMediaHandler::enableDTMFDetection(){ 
		recordConnector.enableDTMFDetection();
}

void IvrMediaHandler::disableDTMFDetection() {
    recordConnector.disableDTMFDetection();
		//  detectionRunning = false;
		//delete dtmfDetector;
		//dtmfDetector = 0;
}

void IvrMediaHandler::pauseDTMFDetection() {
    recordConnector.pauseDTMFDetection();
		//detectionRunning = false;
}

void IvrMediaHandler::resumeDTMFDetection() {
    recordConnector.resumeDTMFDetection();
		// if (dtmfDetector) 
		//detectionRunning = true;
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
	DBG("Empty media queue after pop. Notifying parent IvrPython.\n");
	parent->onMediaQueueEmpty();
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
IvrAudioConnector::IvrAudioConnector(IvrPython* parent, IvrMediaHandler* mh, bool isPlay) 
    : AmAudio(), activeMedia(0), mediaHandler(mh), isPlayConnector(isPlay),
      detectionRunning(false), dtmfDetector(0), closed(false), parent(parent)
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


void IvrAudioConnector::startRecording(string& filename) {  
   mediaIn = new AmAudioFile("Wav", 1);
   
   if(mediaIn->open(filename.c_str(),AmAudioFile::Write)){
       ERROR("AmRtpStream::record(): Cannot open file\n");
       delete mediaIn;
       mediaIn = 0;
       return;
   }
   mediaIn->in.reset(out.get()); // transfer our out format to the file's in format 
   // so mediaIn->put will convert from our format to mediaIn's format
   mediaInRunning = true;
}

void IvrAudioConnector::stopRecording() { 
    mediaInRunning = false;
    if (mediaIn) {
	mediaIn->close();
	delete mediaIn;
	mediaIn = 0;
    }
    refreshFormat();
}

void IvrAudioConnector::pauseRecording() { 
    mediaInRunning = false;
}

void IvrAudioConnector::resumeRecording() { 
    if (mediaIn)
	mediaInRunning = true;
}

void IvrAudioConnector::enableDTMFDetection() {
    dtmfDetector = new IvrDtmfDetector(parent);
    detectionRunning = true;
}

void IvrAudioConnector::disableDTMFDetection() {
    detectionRunning = false;    
    delete dtmfDetector;
    dtmfDetector = 0;
}

void IvrAudioConnector::pauseDTMFDetection() {
    detectionRunning = false;
}

void IvrAudioConnector::resumeDTMFDetection() {
    if (dtmfDetector)
	detectionRunning = true;
}
