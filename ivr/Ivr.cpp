/*
 * $Id: Ivr.cpp,v 1.16 2004/08/10 12:13:40 sayer Exp $
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
#include "Ivr.h"

#include "SemsConfiguration.h"
#include "log.h"
#include "AmApi.h"
#include "AmUtils.h"

#include <unistd.h>
#include <pthread.h>

#ifdef IVR_WITH_TTS
#define CACHE_PATH "/tmp/"
extern "C" cst_voice *register_cmu_us_kal();
#endif //ivr_with_tts
// is defined in Ivr.h #define MOD_NAME "ivr"

EXPORT_FACTORY(IvrFactory,MOD_NAME);

IvrFactory:: IvrFactory(const string& _app_name)
  : AmStateFactory(_app_name){
}

/**
 * Loads python script path and default script file from configuration file
 */
int IvrFactory::onLoad(){
    if(mIvrConfig.reloadModuleConfig(MOD_NAME) == 1){
      char* pSF = 0;
      int flagpythonScriptPath = 0, flagdefaultPythonScriptFile = 0;
      if(((pSF = mIvrConfig.getValueForKey("python_script_path")) != NULL) && ( *pSF != '\0') ){
	setenv("PYTHONPATH",pSF,0);
	pythonScriptPath = pSF;
	flagpythonScriptPath = 1;
	if(pythonScriptPath.rfind('/') != pythonScriptPath.length() - 1)
	  pythonScriptPath += "/";
      }
      if(((pSF = mIvrConfig.getValueForKey("python_script_file")) != NULL) && ( *pSF != '\0') ){
	defaultPythonScriptFile = pSF;
	flagdefaultPythonScriptFile = 1;

      }
      if(((pSF = mIvrConfig.getValueForKey("ivr_script_path")) != NULL) && ( *pSF != '\0') ){
	setenv("PYTHONPATH",pSF,0);
	pythonScriptPath = pSF;
	if(pythonScriptPath.rfind('/') != pythonScriptPath.length() - 1)
	  pythonScriptPath += "/";
      } else {
	if (flagpythonScriptPath == 0) {
	  pythonScriptPath = "./";
	  WARN("no ivr_script_path in configuration\n");
	}
      }

      if(((pSF = mIvrConfig.getValueForKey("ivr_script_file")) != NULL) && ( *pSF != '\0') ){
	defaultPythonScriptFile = pSF;
	
      } else {
	if (flagdefaultPythonScriptFile == 0) {
	  WARN("no ivr_script_file in configuration\n");
#ifndef IVR_PERL
	  defaultPythonScriptFile = "ivr.py";
#else 
	  defaultPythonScriptFile = "ivr.pl";
#endif
	}
      }

#ifdef IVR_WITH_TTS
      char* p =0;
      if( ((p = mIvrConfig.getValueForKey("tts_caching")) != NULL) && (*p != '\0') ) {
	tts_caching = ((*p=='y') || (*p=='Y'));
      } else {
	WARN("no tts_caching (y/n) specified in configuration\n");
	WARN("file for module ivr.\n");
	tts_caching = true;
      }

      if( ((p = mIvrConfig.getValueForKey("tts_cache_path")) != NULL) && (*p != '\0') )
	tts_cache_path = p;
      else {
	WARN("no cache_path specified in configuration\n");
	WARN("file for module semstalkflite.\n");
	tts_cache_path  = CACHE_PATH;
      }
      if( !tts_cache_path.empty()
	  && tts_cache_path[tts_cache_path.length()-1] != '/' )
	tts_cache_path += "/";
#endif
    } // success loading configuration

    DBG("** IVR compile time configuration:\n");
#ifndef IVR_PERL
    DBG("**     built with PYTHON support.\n");
#else 
    DBG("**     built with PERL support.\n");
#endif

#ifdef IVR_WITH_TTS
    DBG("**     Text-To-Speech enabled\n");
#else
    DBG("**     Text-To-Speech disabled\n");
#endif

    DBG("** IVR run time configuration:\n");
    DBG("**     script path:         \'%s\'\n", pythonScriptPath.c_str());
    DBG("**     default script file: \'%s\'\n", defaultPythonScriptFile.c_str());
#ifdef IVR_WITH_TTS
    DBG("**     TTS caching:          %s\n", tts_caching ? "yes":"no");
    DBG("**     TTS cache path:      \'%s\'\n", tts_cache_path.c_str());
#endif // IVR_WITH_TTS

    return 0;
}

/**
 * chooses script according user name
 * if there is no special script default script will be used.
 */
AmDialogState* IvrFactory::onInvite(AmCmd& cmd){
   string pythonScriptFile = pythonScriptPath + cmd.domain	+ "/" + cmd.user + SCRIPT_FILE_EXT;
	if(!file_exists(pythonScriptFile)) {
		pythonScriptFile = pythonScriptPath + cmd.domain + "/" + defaultPythonScriptFile;
		if(!file_exists(pythonScriptFile)) {
			pythonScriptFile = pythonScriptPath + cmd.user + SCRIPT_FILE_EXT;
			if(!file_exists(pythonScriptFile))
				pythonScriptFile = pythonScriptPath + defaultPythonScriptFile;
		}
	}
#ifndef IVR_WITH_TTS
   return new IvrDialog(pythonScriptFile);
#else
   return new IvrDialog(pythonScriptFile, tts_cache_path, tts_caching);
#endif
}


#ifndef IVR_WITH_TTS
IvrDialog::IvrDialog(string scriptFile)
  : mediaHandler(0)
#else
IvrDialog::IvrDialog(string scriptFile, string tts_cache_path_, bool tts_caching_)
				: mediaHandler(0), tts_cache_path(tts_cache_path_), 
				  tts_caching(tts_caching_)
#endif
{
   pythonScriptFile = scriptFile;
   ivrPython = new IvrPython();
   ivrPython->fileName = (char*)pythonScriptFile.c_str();

#ifdef IVR_WITH_TTS
    flite_init();
    tts_voice = register_cmu_us_kal();
// set these to modifiy basic features of the voice
//    feat_set_float(tts_voice->features,"int_f0_target_mean",90.0);
//    feat_set_float(tts_voice->features,"duration_stretch",1.1);
#endif
}

IvrDialog::~IvrDialog()
{
// #ifndef IVR_PERL
//   if (!ivrPython->getStopped())
//     ivrPython->cancel();  // kill the interpreter thread if not already stopped
// #endif

//   ivrPython->setNoUnregisterScriptQueue();
}

void IvrDialog::onSessionStart(AmRequest* req){
  mediaHandler = new IvrMediaHandler();

  ivrPython->pAmSession = getSession();
  ivrPython->pCmd = &(req->cmd);
   
  ivrPython->registerWith(mediaHandler);
  ivrPython->registerForeignEventQueue(this);
  
#ifdef IVR_WITH_TTS
  ivrPython->tts_voice = tts_voice;
  ivrPython->tts_caching = tts_caching;
  ivrPython->tts_cache_path = tts_cache_path;
#endif //IVR_WITH_TTS
  ivrPython->start();
  
   // start new thread to process script events
#ifdef IVR_PERL
  auto_ptr<IvrScriptEventProcessor> scriptEventP;
  scriptEventP.reset(new IvrScriptEventProcessor(ivrPython->getScriptEventQueue()));
  scriptEventP->start();
#endif
  
  // plug on our media handler (in and out)
  DBG("Start duplex...\n");
  ivrPython->pAmSession->rtp_str.duplex(mediaHandler->getPlayConnector(),
					mediaHandler->getRecordConnector(), -1);
   
  DBG("End duplex.\n");
  
  mediaHandler->unregisterForeignEventQueue(); // we don't want more script events
  ivrPython->unregisterForeignEventQueue();  // we don't want more media events from the script

#ifdef IVR_PERL
  scriptEventP->stop();
  // AmThreadWatcher::instance()->add(scriptEventP.get());
#endif
   
  if(!getSession()->sess_stopped.get())
    req->bye();
 
  DBG("Waiting for the interpreter to stop\n");  
  ivrPython->stop();
  ivrPython->setNoUnregisterScriptQueue();
  for (int i=0;i<INTERPRETER_THREAD_STOP_TIMEOUT*10;i++) { // give the interpreter some time to finish
    if (ivrPython->getStopped())
      break;
    usleep(100000); 
  }

#ifdef KILL_HANGING_INTERPRETER
  if (!ivrPython->getStopped())
    ivrPython->cancel();  // kill the interpreter thread if not stopped by itself
#endif
  
  IvrMediaHandler* _mh = mediaHandler;
  mediaHandler = 0; 
  delete _mh;
  
  DBG("Handing over interpreter thread to ThreadWatcher.\n"); 
  AmThreadWatcher::instance()->add(ivrPython);

  DBG("finished.\n");
}

void IvrDialog::onBye(AmRequest* req){
  if(!ivrPython->getStopped()) {
    ivrPython->postScriptEvent(new IvrScriptEvent(IvrScriptEvent::IVR_Bye, req));
// #ifndef IVR_PERL
//     if (!ivrPython->getStopped()) {
//       ivrPython->cancel();
//     }
// #endif
   }
}

int IvrDialog::onOther(AmSessionEvent* event)
{
//       DBG("NOTIFY ---- \n\n\n\n");
//     if(event->event_id == AmSessionEvent::Notify){
// 	DBG("Notify received...\n");
// 	AmNotifySessionEvent* notifyEvent = dynamic_cast<AmNotifySessionEvent*>(event);
// 	if (!notifyEvent) {
// 	    ERROR("Invalid In-Session Notify event received.\n");
// 	    return 1;
// 	}
// 	DBG("evtpackage == %s\n", notifyEvent->getEventPackage().c_str());
// 	if (notifyEvent->getEventPackage()=="refer") { // we only handle refer-NOTIFYs
// 	    if (event->request.reply(200, "OK")) 
// 		DBG("Could not reply 200 (request was NOTIFY)\n");
// 	    DBG("NOTIFY Event Body: %s", event->request.getBody().c_str());
// 	}
	       
//     }
//   if(event->event_id == AmSessionEvent::Notify){
//         ivrPython->postScriptEvent(new IvrScriptEvent(IvrScriptEvent::IVR_Notify, event));
//         if (strstr(event->request.getBody().c_str(),"200 OK") != NULL){
//             ivrPython->isEvent.set(true);
//         }
//         event->processed = true;
//         DBG("Notify event: body= %s\n",event->request.getBody().c_str());
//
//     }
  return 0;
}

void IvrDialog::process(AmEvent* event) {
  IvrMediaEvent* evt = dynamic_cast<IvrMediaEvent* >(event);
  if (evt) { // this one is for us
    DBG("IvrDialog processing event...\n");
    if (handleMediaEvent(evt)) { 
      ERROR("while processing Media event (ID=%i).\n",event->event_id);
    }
  } else {
    //     AmDialogState::process(event);
    AmSessionEvent* session_event = dynamic_cast<AmSessionEvent*>(event);
    if(!session_event){
	ERROR("AmSession: invalid event received.\n");
	return;
    }

    DBG("in-dialog event received: %s\n",
	session_event->request.cmd.method.c_str());

    if(onOther(session_event))
	ERROR("while proceeding session event (ID=%i).\n",session_event->event_id);
  }
}

int IvrDialog::handleMediaEvent(IvrMediaEvent* evt) {
  evt->processed = true; // we eat up all media events at the moment
  if (!mediaHandler) {
    ERROR("no MediaHandler to process event.\n");
    return 1;
  }
  switch (evt->event_id) {
  case IvrMediaEvent::IVR_enqueueMediaFile: {
    return mediaHandler->enqueueMediaFile(evt->MediaFile, evt->front, evt->loop);
  }; break;
  case IvrMediaEvent::IVR_emptyMediaQueue: {
    return mediaHandler->emptyMediaQueue();
  }; break;
  case IvrMediaEvent::IVR_startRecording: {
    return mediaHandler->startRecording(evt->MediaFile);
  }; break;
  case IvrMediaEvent::IVR_stopRecording: {
    return mediaHandler->stopRecording();
  }; break;
  case IvrMediaEvent::IVR_enableDTMFDetection: {
    return mediaHandler->enableDTMFDetection();
  }; break;
  case IvrMediaEvent::IVR_disableDTMFDetection: {
    return mediaHandler->disableDTMFDetection();
  }; break;
  case IvrMediaEvent::IVR_resumeDTMFDetection: {
    return mediaHandler->resumeDTMFDetection();
  }; break;
  case IvrMediaEvent::IVR_pauseDTMFDetection: {
    return mediaHandler->pauseDTMFDetection();
  }; break;
  case IvrMediaEvent::IVR_mthr_usleep: {
      DBG("Media Thread sleeping %d usec\n", evt->usleep_time);
      usleep(evt->usleep_time);
      DBG("Media Thread: done sleeping.\n");
    return 0;
  }; break;
  }
  
  return 0;
}

#ifdef IVR_PERL
IvrScriptEventProcessor::IvrScriptEventProcessor(AmEventQueue* watchThisQueue) 
  : runcond(true), q(watchThisQueue)
{
}

void IvrScriptEventProcessor::on_stop() {
  runcond.set(false);
}
void IvrScriptEventProcessor::run() {
  usleep(500);
  while (runcond.get()) {
    q->processEvents();
    usleep(SCRIPT_EVENT_CHECK_INTERVAL_US);
  }
  DBG("IvrScriptEventProcessor exiting.\n");
}

IvrScriptEventProcessor::~IvrScriptEventProcessor() {
  DBG("IvrScriptEventProcessor destroyed.\n");
}
#endif //IVR_PERL
