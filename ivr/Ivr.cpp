/*
 * $Id: Ivr.cpp,v 1.3 2004/06/15 10:05:00 sayer Exp $
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

#include "SemsConfiguration.h"
#include "log.h"
#include "AmApi.h"
#include "AmUtils.h"

#include "IvrPython.h"
#include "Ivr.h"

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
//		else {
//         pythonScriptPath = "/";
//			WARN("no python_script_path in configuration\n");
//		}
		if(((pSF = mIvrConfig.getValueForKey("python_script_file")) != NULL) && ( *pSF != '\0') ){
         defaultPythonScriptFile = pSF;
         flagdefaultPythonScriptFile = 1;
//         return 0;
      }
//		else {
//			WARN("no python_script_file in configuration\n");
//			return -1;
//		}

	if(((pSF = mIvrConfig.getValueForKey("ivr_script_path")) != NULL) && ( *pSF != '\0') ){
		setenv("PYTHONPATH",pSF,0);
		pythonScriptPath = pSF;
		if(pythonScriptPath.rfind('/') != pythonScriptPath.length() - 1)
			pythonScriptPath += "/";
	} else {
		if (flagpythonScriptPath == 0) {
			pythonScriptPath = "/";
			WARN("no ivr_script_path in configuration\n");
		}
	}

	if(((pSF = mIvrConfig.getValueForKey("ivr_script_file")) != NULL) && ( *pSF != '\0') ){
		defaultPythonScriptFile = pSF;
         return 0;
	} else {
		if (flagdefaultPythonScriptFile == 0) {
			WARN("no ivr_script_file in configuration\n");
			return -1;
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

   }
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
#else
IvrDialog::IvrDialog(string scriptFile, string tts_cache_path_, bool tts_caching_)
				: tts_cache_path(tts_cache_path_), tts_caching(tts_caching_)
#endif
{
   pythonScriptFile = scriptFile;

#ifdef IVR_WITH_TTS
    flite_init();
    tts_voice = register_cmu_us_kal();
// set these to modifiy basic features of the voice
//    feat_set_float(tts_voice->features,"int_f0_target_mean",90.0);
//    feat_set_float(tts_voice->features,"duration_stretch",1.1);
#endif
}

IvrDialog::~IvrDialog(){}

void IvrDialog::onSessionStart(AmRequest* req){
   ivrPython = new IvrPython();
   ivrPython->fileName = (char*)pythonScriptFile.c_str();
   ivrPython->pAmSession = getSession();
   ivrPython->pCmd = &(req->cmd);

#ifdef IVR_WITH_TTS
   ivrPython->tts_voice = tts_voice;
   ivrPython->tts_caching = tts_caching;
   ivrPython->tts_cache_path = tts_cache_path;
#endif //IVR_WITH_TTS
   ivrPython->start();
   AmThreadWatcher::instance()->add(ivrPython);
   // plug on our media handler (in and out)

   DBG("Start duplex...\n");
   ivrPython->pAmSession->rtp_str.duplex(ivrPython->mediaHandler->getPlayConnector(),
					 ivrPython->mediaHandler->getRecordConnector());

   DBG("End duplex.\n");

//    sleep(1);
//    while(!getSession()->sess_stopped.get() && !ivrPython->getStopped()){
//          processEvents();
//          sleep(1);
//    }
   if(!getSession()->sess_stopped.get())
      req->bye();
   getSession()->sess_stopped.wait_for();
   ivrPython->stop();
#ifndef IVR_PERL
   ivrPython->cancel();
#endif
}


void IvrDialog::onBye(AmRequest* req){
  if( !ivrPython->getStopped()) {
    ivrPython->onBye(req);
    ivrPython->stop();
#ifndef IVR_PERL
    ivrPython->cancel();
#endif
  }
}

int IvrDialog::onOther(AmSessionEvent* event)
{
    if(event->event_id == AmSessionEvent::Notify){
      ivrPython->onNotify(event);
        if (strstr(event->request.getBody().c_str(),"200 OK") != NULL){
            ivrPython->isEvent.set(true);
        }
        event->processed = true;
        DBG("Notify event: body= %s\n",event->request.getBody().c_str());

    }

    return AmDialogState::onOther(event);
}
