/*
 * $Id: IvrPython.cpp,v 1.17 2004/07/09 10:53:51 ilk Exp $
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

#define CACHE_PATH "/tmp/"

#include "IvrPython.h"
#include "AmApi.h"
#include "Ivr.h"
#include "IvrDtmfDetector.h"
#include "IvrMediaHandler.h"

#include "log.h"
#include "AmUtils.h"
#include <unistd.h>

//AmSession* amSession;
//IvrPython* pIvrPython;
//IvrDetect* pIvrDetect;
//int detKey;

//pthread_mutex_t mutexAmSession;

#ifdef IVR_WITH_TTS
extern "C" cst_voice *register_cmu_us_kal();
#endif // IVR_WITH_TTS

#define SAFE_POST_MEDIAEVENT(evt) pIvrPython->postMediaEvent(evt)

/***********************************************************************************************************
 *   script  extensions
 *
 ***********************************************************************************************************
 */
extern "C" {

  static IvrPython* getIvrPythonPointer(){
    IvrPython* pIvrPython = NULL;
#ifndef IVR_PERL
    PyObject *module = PyImport_ImportModule(PY_MOD_NAME);
    if (module != NULL) {
      PyObject *ivrPythonPointer = PyObject_GetAttrString(module, "ivrPythonPointer");
      if (ivrPythonPointer != NULL){
	if (PyCObject_Check(ivrPythonPointer))
	  pIvrPython = (IvrPython*)PyCObject_AsVoidPtr(ivrPythonPointer);
	Py_DECREF(ivrPythonPointer);
      }
    }
#else	//IVR_PERL
    SV* pivr = get_sv("Ivr::__ivrpointer__", FALSE);
    if (pivr != NULL)
	pIvrPython = (IvrPython *) SvUV(pivr);
#endif	//IVR_PERL
    return pIvrPython;
  }

  SCRIPT_DECLARE_FUNC(ivrEnqueueMediaFile) {
    SCRIPT_DECLARE_VAR;
    char* fileName;
    int front = 1;
    if(pIvrPython != NULL){
      if(SCRIPT_GET_s_optional_i(fileName, front)){
	string sFileName(fileName);
	DBG("IVR: enqueuing media file (%s) at the %s",fileName, (front==1?"front\n":"back\n") );
	//SCRIPT_BEGIN_ALLOW_THREADS
	SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_enqueueMediaFile, sFileName, front));
			     DBG("IVR: finished enqueue.\n");
	//SCRIPT_END_ALLOW_THREADS
	SCRIPT_RETURN_i(1);
      }
      else {
	SCRIPT_ERR_STRING("ivrEnqueueMediaFile: parameter mismatch!\n"
			"Wanted: filename:string, front=1 (default, optional, 0=at the back)");
	SCRIPT_RETURN_NULL; // raise exception
      }
    } else {
	SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
	SCRIPT_RETURN_NULL; // raise exception
    }
  }

  SCRIPT_DECLARE_FUNC(ivrEmptyMediaQueue) {
    SCRIPT_DECLARE_VAR;
    if(pIvrPython != NULL){
      DBG("IVR: emptying  media queue.\n");
      SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_emptyMediaQueue));
      SCRIPT_RETURN_i(1);
    }   else {
      SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
      SCRIPT_RETURN_NULL;
    }
  }

  SCRIPT_DECLARE_FUNC(ivrStartRecording) {
    SCRIPT_DECLARE_VAR;
    char* fileName;
    if(pIvrPython != NULL){
      if(SCRIPT_GET_s(fileName)){
	string sFileName(fileName);
	DBG("IVR: start recording to file (%s)\n",fileName);
	SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_startRecording, sFileName));
	SCRIPT_RETURN_i(1);
      } else {
	SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong Arguments!");
      }
    } else {
      SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
      SCRIPT_RETURN_NULL;
    }
  }

  SCRIPT_DECLARE_FUNC(ivrStopRecording) {
    SCRIPT_DECLARE_VAR;
    if(pIvrPython != NULL){
      DBG("IVR: stop recording.\n");
      SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_stopRecording));
      SCRIPT_RETURN_i(1);
    }   else {
      SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
      SCRIPT_RETURN_NULL;
    }
  }

  SCRIPT_DECLARE_FUNC(ivrEnableDTMFDetection) {
    SCRIPT_DECLARE_VAR;
    if(pIvrPython != NULL){
      DBG("IVR: enable DTMF Detection.\n");
      SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_enableDTMFDetection));
      SCRIPT_RETURN_i(1);
    }   else {
      SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
      SCRIPT_RETURN_NULL;
    }
  }

  SCRIPT_DECLARE_FUNC(ivrDisableDTMFDetection) {
    SCRIPT_DECLARE_VAR;
    if(pIvrPython != NULL){
      DBG("IVR: disable DTMF Detection.\n");
      SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_disableDTMFDetection));
      SCRIPT_RETURN_i(1);
    }   else {
      SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
      SCRIPT_RETURN_NULL;
    }
  }

  SCRIPT_DECLARE_FUNC(ivrPauseDTMFDetection) {
    SCRIPT_DECLARE_VAR;
    if(pIvrPython != NULL){
      DBG("IVR: pause DTMF Detection.\n");
      SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_pauseDTMFDetection));
      SCRIPT_RETURN_i(1);
    }   else {
      SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
      SCRIPT_RETURN_NULL;
    }
  }


  SCRIPT_DECLARE_FUNC(ivrResumeDTMFDetection) {
    SCRIPT_DECLARE_VAR;
    if(pIvrPython != NULL){
      DBG("IVR: resume DTMF Detection.\n");
      SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_resumeDTMFDetection));
      SCRIPT_RETURN_i(1);
    }   else {
      SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
      SCRIPT_RETURN_NULL;
    }
  }

  SCRIPT_DECLARE_FUNC(ivrUSleep) {
    SCRIPT_DECLARE_VAR;
    int stime;
    if(pIvrPython != NULL){
      if(SCRIPT_GET_i(stime)){
	DBG("IVR: sleeping %d useconds.\n", stime);
	int timediff = 0;
	SCRIPT_BEGIN_ALLOW_THREADS
	timeval tvStart, tvNow;
	gettimeofday(&tvStart,0);
	pIvrPython->wakeUpFromSleep.set(false);
	while((!pIvrPython->scriptStopped.get()) 
	      && (!pIvrPython->wakeUpFromSleep.get()) 
	      && (timediff < stime)){
	  usleep(10);
	  AmEventQueue* evq = pIvrPython->getScriptEventQueue();
	  if (evq)
	    evq->processEvents();
	  gettimeofday(&tvNow,0);
	  timediff = (tvNow.tv_sec - tvStart.tv_sec)* 1000000 + (tvNow.tv_usec - tvStart.tv_usec);
	}
	//	usleep(stime);
	SCRIPT_END_ALLOW_THREADS
	DBG("IVR: waking up after <= %d usec.\n", timediff);
	SCRIPT_RETURN_i(timediff);
      } else {
	SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong Arguments!");
      }
    } else {
      SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
      SCRIPT_RETURN_NULL;
    }
  }

  SCRIPT_DECLARE_FUNC(ivrmSleep) {
    SCRIPT_DECLARE_VAR;
    int stime;
    if(pIvrPython != NULL){
      if(SCRIPT_GET_i(stime)){
	DBG("IVR: sleeping %d mseconds.\n", stime);
	  int timediff = 0;
	SCRIPT_BEGIN_ALLOW_THREADS
	timeval tvStart, tvNow;
	gettimeofday(&tvStart,0);
	pIvrPython->wakeUpFromSleep.set(false);
	while((!pIvrPython->scriptStopped.get()) 
	      && (!pIvrPython->wakeUpFromSleep.get()) 
	      && (timediff < stime)){
	  usleep(10);
	  AmEventQueue* evq = pIvrPython->getScriptEventQueue();
	  if (evq)
	    evq->processEvents();
	  gettimeofday(&tvNow,0);
	  timediff = (tvNow.tv_sec - tvStart.tv_sec)* 1000 + (tvNow.tv_usec - tvStart.tv_usec)/1000;
	}
	//	usleep(stime);
	SCRIPT_END_ALLOW_THREADS
	DBG("IVR: waking up after <= %d msec.\n", timediff);
	SCRIPT_RETURN_i(timediff);
      } else {
	SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong Arguments!");
      }
    } else {
      SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
      SCRIPT_RETURN_NULL;
    }
  }



  SCRIPT_DECLARE_FUNC(ivrSleep) {
    SCRIPT_DECLARE_VAR;
    int stime, real_sleep_time;
    if(pIvrPython != NULL){
      if(SCRIPT_GET_i(stime)){
	DBG("IVR: sleeping %d seconds.\n", stime);
	SCRIPT_BEGIN_ALLOW_THREADS
	  real_sleep_time = pIvrPython->doSleep(stime);
	  //	sleep(stime);
	SCRIPT_END_ALLOW_THREADS
	DBG("IVR: waking up after <= %d sec.\n", real_sleep_time);
	SCRIPT_RETURN_i(real_sleep_time);
      } else {
	SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong Arguments!");
      }
    } else {
      SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
      SCRIPT_RETURN_NULL;
    }
  }

  SCRIPT_DECLARE_FUNC(ivrWakeUp) {
    SCRIPT_DECLARE_VAR;
    if(pIvrPython != NULL){
      DBG("IVR: waking up from sleep.\n");
      pIvrPython->wakeUpFromSleep.set(true);
      SCRIPT_RETURN_i(1);
    }   else {
      SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
      SCRIPT_RETURN_NULL;
    }
  }

  /**
   * Python extension for getting time
   */
  SCRIPT_DECLARE_FUNC(ivrGetTime) {
#ifdef IVR_PERL
    SCRIPT_DECLARE_VAR;
#endif
    SCRIPT_RETURN_i((int)time(NULL));
  } 


  /**
   * Python extension for caller name
   */
  SCRIPT_DECLARE_FUNC(ivrGetFrom) {
    SCRIPT_DECLARE_VAR;
    if(pIvrPython != NULL){
      DBG("ivrGetFrom: returning %s\n", pIvrPython->pCmd->from.c_str());
      SCRIPT_RETURN_s(pIvrPython->pCmd->from.c_str());
    }
    else {
      SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
    }
  }


  /**
   * Python extension for callee name
   */
  SCRIPT_DECLARE_FUNC(ivrGetTo) {
    SCRIPT_DECLARE_VAR;
    if(pIvrPython != NULL){
      SCRIPT_RETURN_s(pIvrPython->pCmd->to.c_str());
    }
    else
      SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
  }


  /**
      * Python extension for caller uri
      */
  SCRIPT_DECLARE_FUNC(ivrGetFromURI) {
    SCRIPT_DECLARE_VAR;
    if(pIvrPython != NULL){
      SCRIPT_RETURN_s(pIvrPython->pCmd->from_uri.c_str());
    }
    else
      SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
  }


  /**
   * Python extension for callee uri
   */
  SCRIPT_DECLARE_FUNC(ivrGetToURI) {
    SCRIPT_DECLARE_VAR;
    if(pIvrPython != NULL){
      SCRIPT_RETURN_s(pIvrPython->pCmd->r_uri.c_str());
    }
    else
      SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
  }


  /**
   * Python extension for domain name
   */
  SCRIPT_DECLARE_FUNC(ivrGetDomain) {
    SCRIPT_DECLARE_VAR;
    if(pIvrPython != NULL){
      SCRIPT_RETURN_s(pIvrPython->pCmd->domain.c_str());
    }
    else
      SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
  }


  /**
   * Python extension for redirection
   */
  SCRIPT_DECLARE_FUNC(ivrRedirect) {
#ifdef IVR_PERL
         SCRIPT_DECLARE_VAR;
#endif
   //  char* uri;

//     if(pIvrPython != NULL){
//       if(SCRIPT_GET_s(uri)){
// 	string refer_to(uri);
// 	SCRIPT_BEGIN_ALLOW_THREADS
// 	  AmCmd refer_cmd = AmRequestUAC::refer(*pIvrPython->pCmd,refer_to);
// 	SCRIPT_END_ALLOW_THREADS
// 	  // was send
// 	  if(pIvrPython->waitForEvent(30000))
//             SCRIPT_RETURN_i(1);
//       }
//       // error
//       DBG("Redirect timeout\n");
//       SCRIPT_RETURN_(0);
//     }
//     else
//       SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
    SCRIPT_RETURN_NULL;
  }


#ifdef IVR_WITH_TTS

SCRIPT_DECLARE_FUNC(ivrSay) {
  SCRIPT_DECLARE_VAR;
  char* ttsText;

  int front = 1;
  if(pIvrPython != NULL){
    if(SCRIPT_GET_s_optional_i(ttsText, front)){
      string message(ttsText);
      string msg_filename = string("/tmp/") +  pIvrPython->pCmd->callid + message + string(".wav");
      string cache_filename = pIvrPython->tts_cache_path + message + string(".wav");
      if (pIvrPython->tts_caching) {
	DBG(" trying cache \"%s\" .. ", cache_filename.c_str());
	if (file_exists(cache_filename)) {
	  DBG("hit. Playing from cache.\n");
	  SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaHandler::IVR_enqueueMediaFile, cache_filename, front));
	  SCRIPT_RETURN_i(1);
	} else {
	  DBG("miss.\n");
	}
      }
      //-----------------
      cst_wave *w;
      cst_utterance *u;
      u = flite_synth_text(message.c_str(),pIvrPython->tts_voice);
      w = utt_wave(u);
      float durs;
      durs = (float)w->num_samples/(float)w->sample_rate;

      cst_wave_save_riff(w,msg_filename.c_str());
      if (pIvrPython->tts_caching)
	cst_wave_save_riff(w, cache_filename.c_str());
      delete_utterance(u);
      DBG("%f seconds of speech synthesized\n",durs);
      //-----------------
      SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaHandler::IVR_enqueueMediaFile, msg_filename, front));
      
      SCRIPT_RETURN_i(1);
    } else {
      SCRIPT_ERR_STRING("ivrEnqueueMediaFile: parameter mismatch!\n"
      				"Wanted: filename:string, front=1 (default, optional, 0=at the back)");
      SCRIPT_RETURN_NULL; // raise exception
    }
  } else {
    SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
    SCRIPT_RETURN_NULL; // raise exception
  }
}
#endif // IVR_WITH_TTS

//   SCRIPT_DECLARE_FUNC(ivrExit) {
//     SCRIPT_DECLARE_VAR;
//     if(pIvrPython != NULL){
//       DBG("IVR: exiting...\n");      
//       //       Py_EndInterpreter(pIvrPython->mainInterpreterThreadState);
//       // pthread_exit(0);
//        SCRIPT_RETURN_i(1);//wont be called
//     }   else {
//       SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
//       SCRIPT_RETURN_NULL;
//     }
//   }

/*   Old sequential IVR functions: play(filename), record(filename, timeout), playAndDetect(filename, timeout)
 *   Warning: do not mix these with the event based functions above.
 *
 */
  SCRIPT_DECLARE_FUNC(ivrPlay) {
    SCRIPT_DECLARE_VAR;
    char* fileName;
    if(pIvrPython != NULL){
      if(SCRIPT_GET_s(fileName)){
	pIvrPython->isMediaQueueEmpty.set(false);
	string sFileName(fileName);
	DBG("IVR: enqueuing media file (%s) at the front.\n", fileName);
	//SCRIPT_BEGIN_ALLOW_THREADS
	SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_emptyMediaQueue));
	SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_enqueueMediaFile, sFileName, true));
	DBG("IVR: finished enqueue. waiting for isMediaQueueEmpty.\n");
	while((!pIvrPython->scriptStopped.get()) && 
	      (!pIvrPython->isMediaQueueEmpty.get()) ) {
	  usleep(1000);
	  AmEventQueue* evq = pIvrPython->getScriptEventQueue();
	  if (evq)
	    evq->processEvents();
	}
	DBG("IVR: isMediaQueueEmpty. returning to script.\n");
	//SCRIPT_END_ALLOW_THREADS
	SCRIPT_RETURN_i(1);
      }
      else {
	SCRIPT_ERR_STRING("ivrPlay: parameter mismatch!\n"
			"Wanted: filename:string");
	SCRIPT_RETURN_NULL; // raise exception
      }
    } else {
	SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
	SCRIPT_RETURN_NULL; // raise exception
    }
  }

  SCRIPT_DECLARE_FUNC(ivrRecord) {
    SCRIPT_DECLARE_VAR;
    char* fileName;
    int timeout = 0, real_sleep_time;
    if(pIvrPython != NULL){
      if(SCRIPT_GET_s_optional_i(fileName, timeout)) {
	DBG("IVR: start record to file (%s).\n", fileName);
	string sFileName(fileName);
	SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_startRecording, sFileName));
	SCRIPT_BEGIN_ALLOW_THREADS
	real_sleep_time = pIvrPython->doSleep(timeout);
	SCRIPT_END_ALLOW_THREADS
	  DBG("IVR: waking up after <= %d sec. Stopping Record\n", real_sleep_time);
	SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_stopRecording));
	SCRIPT_RETURN_i(real_sleep_time);
      }
      else {
	SCRIPT_ERR_STRING("ivrPlay: parameter mismatch!\n"
			"Wanted: filename:string");
	SCRIPT_RETURN_NULL; // raise exception
      }
    } else {
	SCRIPT_ERR_STRING("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
	SCRIPT_RETURN_NULL; // raise exception
    }
  }

  SCRIPT_DECLARE_FUNC(ivrDetect) {
    SCRIPT_DECLARE_VAR;
    int timeout = 0;
    if(pIvrPython != NULL){
      if (SCRIPT_GET_optional_i(timeout)) {
	pIvrPython->dtmfKey.set(-1);
	SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_enableDTMFDetection));
	DBG("IVR: finished enqueue. waiting for isMediaQueueEmpty.\n");
	unsigned int timediff = 0;
	timeval tvStart, tvNow;
	gettimeofday(&tvStart,0);
	pIvrPython->wakeUpFromSleep.set(false);
	while((!pIvrPython->scriptStopped.get()) 
	      && (!pIvrPython->wakeUpFromSleep.get()) 
	      && ((!timeout) || (timediff < (unsigned int) timeout*1000000))
	      && (pIvrPython->dtmfKey.get() == -1)){
	  usleep(100);
	  AmEventQueue* evq = pIvrPython->getScriptEventQueue();
	  if (evq)
	    evq->processEvents();
	  gettimeofday(&tvNow,0);
	  timediff = (tvNow.tv_sec - tvStart.tv_sec)* 1000000 + (tvNow.tv_usec - tvStart.tv_usec);
	}
      DBG("IVR: finished waiting. returning %d to script.\n", pIvrPython->dtmfKey.get());
      SCRIPT_RETURN_i(pIvrPython->dtmfKey.get());
      } else {
	SCRIPT_ERR_STRING("ivrDetect: parameter mismatch!\n"
			  "Wanted: timeout = 0:int");
	SCRIPT_RETURN_NULL; // raise exception
      }
    }
    else
      SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
  }


  SCRIPT_DECLARE_FUNC(ivrPlayAndDetect) {
    SCRIPT_DECLARE_VAR;
    int timeout = 0;
    char* fileName;
    if(pIvrPython != NULL){
      if (SCRIPT_GET_s_optional_i(fileName, timeout)) {
	string sFileName(fileName);
	pIvrPython->dtmfKey.set(-1);
	pIvrPython->isMediaQueueEmpty.set(false);
	SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_emptyMediaQueue));
	SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_enableDTMFDetection));
	SAFE_POST_MEDIAEVENT(new IvrMediaEvent(IvrMediaEvent::IVR_enqueueMediaFile, sFileName, true));
	DBG("IVR: finished enqueue. waiting for isMediaQueueEmpty.\n");
	unsigned int timediff = 0;
	timeval tvStart, tvNow;
	gettimeofday(&tvStart,0);
	pIvrPython->wakeUpFromSleep.set(false);
	while((!pIvrPython->scriptStopped.get()) 
	      && (!pIvrPython->wakeUpFromSleep.get()) 
	      && ((!timeout) || (timediff < (unsigned int) timeout*1000000))
	      && (pIvrPython->dtmfKey.get() == -1)
	      && (!pIvrPython->isMediaQueueEmpty.get())) {
	  usleep(100);
	  AmEventQueue* evq = pIvrPython->getScriptEventQueue();
	  if (evq)
	    evq->processEvents();
	  gettimeofday(&tvNow,0);
	  timediff = (tvNow.tv_sec - tvStart.tv_sec)* 1000000 + (tvNow.tv_usec - tvStart.tv_usec);
	}
      
      DBG("IVR: isMediaQueueEmpty. returning %d to script.\n", pIvrPython->dtmfKey.get());
      SCRIPT_RETURN_i(pIvrPython->dtmfKey.get());
      } else {
	SCRIPT_ERR_STRING("ivrDetect: parameter mismatch!\n"
			  "Wanted: timeout = 0:int");
	SCRIPT_RETURN_NULL; // raise exception
      }
    }
    else
      SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
  }


  SCRIPT_DECLARE_FUNC(setCallback) {
    SCRIPT_DECLARE_VAR;
    DBG("setCallback !\n");
#ifndef IVR_PERL
    PyObject *result = NULL;
    PyObject *temp;
    char* callbackName;

    if(pIvrPython != NULL){
      if (SCRIPT_GET_Os(temp, callbackName)) {
					if (!PyCallable_Check(temp)) {
							SCRIPT_ERR_STRING("setCallback: first parameter must be callable");
							SCRIPT_RETURN_NULL;
					}
					DBG("setting %s.\n", callbackName);
					PyObject** wantedCallback = 0;
					if (!strcasecmp(callbackName, "onBye")) {
							wantedCallback = &pIvrPython->onByeCallback;
					} else if (!strcasecmp(callbackName, "onNotify")) {
							wantedCallback = &pIvrPython->onNotifyCallback;
					} else if (!strcasecmp(callbackName, "onDTMF")) {
							wantedCallback = &pIvrPython->onDTMFCallback;
					} else if (!strcasecmp(callbackName, "onMediaQueueEmpty")) {
							wantedCallback = &pIvrPython->onMediaQueueEmptyCallback;
					}
					DBG("asd\n");
					if (!wantedCallback) {
							SCRIPT_ERR_STRING("setCallback: second parameter must be event name.");
							SCRIPT_RETURN_NULL;
					}

					Py_XINCREF(temp);         /* Add a reference to new callback */
					Py_XDECREF(*wantedCallback);  /* Dispose of previous callback */
					*wantedCallback = temp;       /* Remember new callback */

					/* Boilerplate to return "None" */
					Py_INCREF(Py_None);
					result = Py_None;
					DBG("setcallback finished.\n");
      }
      return result;
    } else
      return PyString_FromString("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");

#else   //IVR_PERL
	int result = 0;
    char *temp;
    char* callbackName;

    if(pIvrPython != NULL){
      if (SCRIPT_GET_Os(temp, callbackName)) {
					DBG("setting %s.\n", callbackName);
					char** wantedCallback = 0;
					if (!strcasecmp(callbackName, "onBye")) {
							wantedCallback = &pIvrPython->onByeCallback;
					} else if (!strcasecmp(callbackName, "onNotify")) {
							wantedCallback = &pIvrPython->onNotifyCallback;
					} else if (!strcasecmp(callbackName, "onDTMF")) {
							wantedCallback = &pIvrPython->onDTMFCallback;
					} else if (!strcasecmp(callbackName, "onMediaQueueEmpty")) {
							wantedCallback = &pIvrPython->onMediaQueueEmptyCallback;
					}
					DBG("function is %s.\n", temp);
					if (!wantedCallback) {
							SCRIPT_ERR_STRING("setCallback: second parameter must be event name.");
							SCRIPT_RETURN_NULL;
					}

//					Py_XINCREF(temp);         /* Add a reference to new callback */
//					Py_XDECREF(*wantedCallback);  /* Dispose of previous callback */
					*wantedCallback = temp;       /* Remember new callback */

//					/* Boilerplate to return "None" */
//					Py_INCREF(Py_None);
					result = 1;
					DBG("setcallback finished.\n");
      }
      SCRIPT_RETURN_i(result);
    } else
      SCRIPT_RETURN_STR("IVR" SCRIPT_TYPE "Error: Wrong pointer to IvrPython!");
#endif  //IVR_PERL

}


#ifdef IVR_PERL

XS(boot_DynaLoader) ;
XS(boot_Ivr) { };

void xs_init(pTHX)
{
	char *file = __FILE__;
	dXSUB_SYS;

	/* DynaLoader is a special case */
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
	newXS(PY_MOD_NAME"::""bootstrap", boot_Ivr, file);
	newXS(PY_MOD_NAME"::""enqueueMediaFile", ivrEnqueueMediaFile, file);
	newXS(PY_MOD_NAME"::""emptyMediaQueue", ivrEmptyMediaQueue, file);
	newXS(PY_MOD_NAME"::""startRecording", ivrStartRecording, file);
	newXS(PY_MOD_NAME"::""stopRecording", ivrStopRecording, file);
#ifdef IVR_WITH_TTS
	newXS(PY_MOD_NAME"::""say", ivrSay, file);
#endif //IVR_WITH_TTS
       // DTMF functions
	newXS(PY_MOD_NAME"::""enableDTMFDetection", ivrEnableDTMFDetection, file);
	newXS(PY_MOD_NAME"::""disableDTMFDetection", ivrDisableDTMFDetection, file);
	newXS(PY_MOD_NAME"::""pauseDTMFDetection", ivrPauseDTMFDetection, file);
	newXS(PY_MOD_NAME"::""resumeDTMFDetection", ivrResumeDTMFDetection, file);

       // informational
	newXS(PY_MOD_NAME"::""getTime", ivrGetTime, file);
	newXS(PY_MOD_NAME"::""getFrom", ivrGetFrom, file);
	newXS(PY_MOD_NAME"::""getTo", ivrGetTo, file);
	newXS(PY_MOD_NAME"::""getFromURI", ivrGetFromURI, file);
	newXS(PY_MOD_NAME"::""getToURI", ivrGetToURI, file);
	newXS(PY_MOD_NAME"::""getDomain", ivrGetDomain, file);

       // call transfer functions
	newXS(PY_MOD_NAME"::""redirect", ivrRedirect, file);

       // setting callbacks
	newXS(PY_MOD_NAME"::""setCallback", setCallback, file);

	newXS(PY_MOD_NAME"::""sleep", ivrSleep, file);
	newXS(PY_MOD_NAME"::""usleep", ivrUSleep, file);
	newXS(PY_MOD_NAME"::""msleep", ivrmSleep, file);
	newXS(PY_MOD_NAME"::""wakeUp", ivrWakeUp, file);

	// legacy from old (sequential) ivr
	newXS(PY_MOD_NAME"::""play", ivrPlay, file);
	newXS(PY_MOD_NAME"::""record", ivrRecord, file);
	newXS(PY_MOD_NAME"::""playAndDetect", ivrPlayAndDetect, file);
	newXS(PY_MOD_NAME"::""detect", ivrDetect, file);
}
#endif
} // end of extern "C"

/***********************************************************************************************************
 *   IvrPython class functions
 *
 ***********************************************************************************************************
*/

IvrPython::IvrPython()
		: onByeCallback(NULL), onNotifyCallback(0), 
		  onDTMFCallback(0), onMediaQueueEmptyCallback(0),
		  mediaEventQueue(0), regScriptEventProducer(0),
		  isMediaQueueEmpty(false), scriptStopped(false)
#ifdef IVR_WITH_TTS
    , tts_voice(0)
#endif //IVR_WITH_TTS
{
  scriptEventQueue.reset(new AmEventQueue(this));
}

IvrPython::~IvrPython() {
//   if (regScriptEventProducer) {
//     DBG("unregistering with script event producer\n");
//     regScriptEventProducer->unregisterForeignEventQueue();
//   }
}

void IvrPython::registerWith(IvrEventProducer* scriptEventProducer) {
  scriptEventProducer->registerForeignEventQueue(scriptEventQueue.get());
  regScriptEventProducer = scriptEventProducer;
}

void IvrPython::setNoUnregisterScriptQueue() {
  regScriptEventProducer = 0;
}

#ifndef IVR_PERL
extern "C" {
  int pythonTrace(PyObject* mobj, PyFrameObject *mframe, int mwhat, PyObject *marg) {
    // DBG("Python trace\n");
    IvrPython* pIvrPython = 0; //getIvrPythonPointer();
    if (mobj != NULL){
      if (PyCObject_Check(mobj)) {
	  pIvrPython = (IvrPython*)PyCObject_AsVoidPtr(mobj);
	  //	  Py_DECREF(mobj);
      }
    }

    if (pIvrPython) {
      AmEventQueue* evq = pIvrPython->getScriptEventQueue();
      if (evq)
	evq->processEvents();
    } else {
      ERROR("IvrPython pointer not found in Trace!\n");
      return 1;
    }
    return 0; 
  }
}
#endif

void IvrPython::run(){
   FILE* fp;
   int retval;

#ifndef	IVR_PERL
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
#endif	//IVR_PERL
//   static PyThreadState* pyMainThreadState;
   fp = fopen((char*)fileName,"r");
   if(fp != NULL){
#ifndef	IVR_PERL
     PyMethodDef extIvrPython[] = {
       // media functions
       {"enqueueMediaFile", ivrEnqueueMediaFile, METH_VARARGS, "ivr: enqueue media file. "
	"filename: string, front : int = 1(default true)  "},
       {"emptyMediaQueue", ivrEmptyMediaQueue, METH_VARARGS, "ivr: empty the media queue."},
       {"startRecording", ivrStartRecording, METH_VARARGS, "ivr: start recording to file. filename : string."},
       {"stopRecording", ivrStopRecording, METH_VARARGS, "ivr: stop recording to file."},
#ifdef IVR_WITH_TTS
       {"say", ivrSay, METH_VARARGS, "ivr tts and enqueue. msg: string, front: boolean  "},
#endif //IVR_WITH_TTS
       // DTMF functions
       {"enableDTMFDetection", ivrEnableDTMFDetection, METH_VARARGS, "enable DTMF detection. "
	"setCallback(onDTMF_FUNC, \"onDTMF\") first!"},
       {"disableDTMFDetection", ivrDisableDTMFDetection, METH_VARARGS, "disable DTMF detection permanently"},
       {"pauseDTMFDetection", ivrPauseDTMFDetection, METH_VARARGS, "pause DTMF detection temporarily, can be resumed"},
       {"resumeDTMFDetection", ivrResumeDTMFDetection, METH_VARARGS, "resume DTMF detection"},

       // informational
       {"getTime", ivrGetTime, METH_VARARGS, "Example Module"},
       {"getFrom", ivrGetFrom, METH_VARARGS, "Example Module"},
       {"getTo", ivrGetTo, METH_VARARGS, "Example Module"},
       {"getFromURI", ivrGetFromURI, METH_VARARGS, "Example Module"},
       {"getToURI", ivrGetToURI, METH_VARARGS, "Example Module"},
       {"getDomain", ivrGetDomain, METH_VARARGS, "Example Module"},

       // call transfer functions
       {"redirect", ivrRedirect, METH_VARARGS, "Example Module"},

       // setting callbacks
       {"setCallback", setCallback, METH_VARARGS, "Example Module"},

       {"sleep", ivrSleep, METH_VARARGS, "Sleep n seconds, or until wakeUp"},
       {"usleep", ivrUSleep, METH_VARARGS, "Sleep n microseconds, or until wakeUp"},
       {"msleep", ivrmSleep, METH_VARARGS, "Sleep n milliseconds, or until wakeUp"},
       {"wakeUp", ivrWakeUp, METH_VARARGS, "wake Up from sleep"},

       // legacy from old ivr: sequential functions
       {"play", ivrPlay, METH_VARARGS, "play and wait for the end of the file (queue empty)"},
       {"record", ivrRecord, METH_VARARGS, "record maximum of time secs. Parameter: filename : string, timeout = 0 : int"},
       {"playAndDetect", ivrPlayAndDetect, METH_VARARGS, "play and wait for the end of the file (queue empty) or keypress"},
       {"detect", ivrDetect, METH_VARARGS, "detect until timeout Parameter: timeout = 0 : int"},

       
       {NULL, NULL, 0, NULL},
     };

     if(!Py_IsInitialized()){
       DBG("Start" SCRIPT_TYPE "\n");
       Py_Initialize();
       PyEval_InitThreads();
       pyMainThreadState = PyEval_SaveThread();
     }
     DBG("Start new" SCRIPT_TYPE "interpreter\n");
     PyEval_AcquireLock();
//     PyThreadState* pyThreadState;
     if ( (mainInterpreterThreadState = Py_NewInterpreter()) != NULL){

       PyObject* ivrPyInitModule = Py_InitModule(PY_MOD_NAME, extIvrPython);
       PyObject* ivrPythonPointer = PyCObject_FromVoidPtr((void*)this,NULL);
       if (ivrPythonPointer != NULL)
	 PyModule_AddObject(ivrPyInitModule, "ivrPythonPointer", ivrPythonPointer);
       
       Py_tracefunc tmp_t = pythonTrace;
       PyEval_SetTrace(tmp_t, PyCObject_FromVoidPtr((void*)this,NULL));
       if(!PyRun_SimpleFile(fp,(char*)fileName)){
	 fclose(fp);
	 retval = 0;// true;
       }
       else{
            PyErr_Print();
            ERROR("IVR" SCRIPT_TYPE "Error: Failed to run \"%s\"\n", (char*)fileName);
	    retval = -1;// false;
       }

       Py_EndInterpreter(mainInterpreterThreadState);
     }
     else{
       ERROR("IVR" SCRIPT_TYPE "Error: Failed to start new interpreter.\n");
     }
     PyEval_ReleaseLock();
#else	//IVR_PERL

	DBG("Start" SCRIPT_TYPE ", about to alloc\n");
	my_perl_interp = perl_alloc();
	printf("interp is %ld\n", (long) my_perl_interp);
	printf("filename is %s\n", fileName);
	DBG("finished alloc Perl, about to construct Perl\n");
	perl_construct(my_perl_interp);
	PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
	char *embedding[] = { "", (char*)fileName};
	DBG("finished construct Perl, about to parse Perl\n");
	perl_parse(my_perl_interp, xs_init, 2, embedding, (char **)NULL);
	DBG("finished parse Perl, about to run Perl\n");
	SV *pivr = get_sv("Ivr::__ivrpointer__", TRUE);
	DBG("Ivr::__ivrpointer__ is %lx.\n", (unsigned long int) pivr);
	sv_setuv(pivr, (unsigned int) this);
	perl_run(my_perl_interp);
	DBG("finished run Perl, about to sleep 5 seconds to let callback event catch up\n");
	sleep(5);
	DBG("after sleep, about to destruct\n");
	perl_destruct(my_perl_interp);
	DBG("finished destruct Perl, about to free\n");
	perl_free(my_perl_interp);
#endif	//IVR_PERL
   }
   else{
     ERROR("IVR" SCRIPT_TYPE "Error: Can not open file \"%s\"\n",(char*) fileName);
     retval = -1;// false;
   }
   DBG("IVR: run finished. stopping rtp stream...\n");
   pAmSession->rtp_str.pause();
}

int IvrPython::registerForeignEventQueue(AmEventQueue* MediaEventQueue) {
  mutexForeignEventQueue.lock();
  if (mediaEventQueue) { //already have one
    mutexForeignEventQueue.unlock(); 
    return 1;
  }
  mediaEventQueue = MediaEventQueue;
  mutexForeignEventQueue.unlock();  
  return 0;
}

void IvrPython::unregisterForeignEventQueue() {
  mediaEventQueue = 0; 
}

// this goes in the foreign  event queue
void IvrPython::postMediaEvent(AmEvent* evt) {
  if (mediaEventQueue) 
    mediaEventQueue->postEvent(evt);
  else { 
    DBG("unable to post media event: no media event queue.\n"); 
  } 
}

// this goes in our own event queue
void IvrPython::postScriptEvent(AmEvent* evt) {
  DBG("posting a script event with id %d\n", evt->event_id);
  //  mutexMediaEventQueue.lock();
  if (scriptEventQueue.get()) {
    scriptEventQueue->postEvent(evt);  
  } else {
    ERROR("could not post script event: no registered eventqueue.\n");
  }
  //  mutexMediaEventQueue.unlock();
}

void IvrPython::process(AmEvent* event){
  DBG("IvrPython processing event...\n");
  IvrScriptEvent* evt = dynamic_cast<IvrScriptEvent* >(event);
  if (evt) { // this one is for us
    DBG("IvrDialog processing event...\n");
    
    switch (evt->event_id) {
	case IvrScriptEvent::IVR_Bye: {
	  onBye(evt->req);
	}; break;
	case IvrScriptEvent::IVR_Notify: {
	    onNotify(evt->event);
	}; break;
	case IvrScriptEvent::IVR_DTMF: {
	    onDTMFEvent(evt->DTMFKey);
	}; break;
	case IvrScriptEvent::IVR_MediaQueueEmpty: {
	    onMediaQueueEmpty();
	}; break;
    }
  } else {
    ERROR("IvrPython: invalid event (non-script) received.\n");
  }
  event->processed = true;
}

void IvrPython::onBye(AmRequest* req) {
  if (onByeCallback == NULL) {
    DBG("Python script did not set onBye callback!\n");
    return;
  }
  DBG("IvrPython::onBye(): calling onByeCallback ...\n");

#ifndef IVR_PERL
  PyThreadState *tstate;

  tstate = PyThreadState_New(mainInterpreterThreadState->interp);
  PyEval_AcquireThread(tstate);

  /* Perform Python actions here.  */
  PyObject *arglist = Py_BuildValue("()");
  PyObject *result = PyEval_CallObject(onByeCallback, arglist);
  Py_DECREF(arglist);

  if (result == NULL) {
      DBG("Calling IVR" SCRIPT_TYPE "onMediaQueueEmpty failed.\n");
      // PyErr_Print();
      //return ;
  } else {
      Py_DECREF(result);
  }

  /* Release the thread. No Python API allowed beyond this point. */
  PyEval_ReleaseThread(tstate);
  PyThreadState_Delete(tstate);
#else   //IVR_PERL
	PERL_SET_CONTEXT(my_perl_interp);
	DBG("context is %ld\n", (long) Perl_get_context());

	dSP ;
	PUSHMARK(SP) ;
	call_pv(onByeCallback, G_DISCARD|G_NOARGS) ;
#endif	//IVR_PERL

  DBG("IvrPython::onBye done...\n");
}


void IvrPython::onNotify(AmSessionEvent* event) {
   if (onNotifyCallback == NULL) {
    DBG("IvrPython::onNotify, but script did not set onNotify callback!\n");
    return;
  }
  DBG("IvrPython::onNotify(): calling onNotifyCallback ...\n");
#ifndef IVR_PERL
  PyThreadState* pyThreadState;
  if ( (pyThreadState = Py_NewInterpreter()) != NULL){
    PyObject *arglist;
    PyObject *result;
    arglist = Py_BuildValue("(s)", event->request.getBody().c_str());;
    result = PyEval_CallObject(onNotifyCallback, arglist);
    Py_DECREF(arglist);
    if (result == NULL) {
      DBG("Calling IVR" SCRIPT_TYPE "onNotify failed.\n");
      // PyErr_Print();
      return ;
    }
    Py_DECREF(result);
  }
  Py_EndInterpreter(pyThreadState);
#else   //IVR_PERL
	PERL_SET_CONTEXT(my_perl_interp);
	DBG("context is %ld\n", (long) Perl_get_context());

	dSP ;
	ENTER ;
	SAVETMPS ;
	PUSHMARK(SP) ;
	XPUSHs(sv_2mortal(newSVpv((event->request.getBody().c_str()), 0)));
	PUTBACK ;
	call_pv(onNotifyCallback, G_DISCARD);
	FREETMPS ;
	LEAVE ;
#endif	//IVR_PERL
}

// sleeps for n seconds, while checking stopped and wakeup
int IvrPython::doSleep(int seconds) {
  int timediff = 0;
  timeval tvStart, tvNow;
  gettimeofday(&tvStart,0);
  wakeUpFromSleep.set(false);
  while((!scriptStopped.get()) 
	&& (!wakeUpFromSleep.get()) 
	&& ((!seconds) || (timediff < seconds))) {
    usleep(10);
    AmEventQueue* evq = getScriptEventQueue();
    if (evq)
      evq->processEvents();
    gettimeofday(&tvNow,0);
	// calculate the time from start, in seconds, round down
    timediff = tvNow.tv_sec - tvStart.tv_sec + ((tvNow.tv_usec < tvStart.tv_usec) ? -1 : 0) ;
  }
  // calculate the time from start, in seconds, round up
  return (tvNow.tv_sec - tvStart.tv_sec + ((tvNow.tv_usec > tvStart.tv_usec) ? 1 : 0));
}

void IvrPython::on_stop()
{
  scriptStopped.set(true);
  if (regScriptEventProducer) {
    DBG("unregistering with script event producer\n");
    regScriptEventProducer->unregisterForeignEventQueue();
  }
}

void IvrPython::onDTMFEvent(int detectedKey) {
  dtmfKey.set(detectedKey); // wake up waiting functions...

   if (onDTMFCallback == NULL) {
    DBG("IvrPython::onDTMFEvent, but script did not set onDTMF callback!\n");
    return;
  }
  DBG("IvrPython::onDTMFEvent(): calling onDTMFCallback key is %d...\n", detectedKey);

#ifndef IVR_PERL
  PyThreadState *tstate;

  /* interp is your reference to an interpreter object. */
  tstate = PyThreadState_New(mainInterpreterThreadState->interp);
  PyEval_AcquireThread(tstate);

  /* Perform Python actions here.  */
  PyObject *arglist = Py_BuildValue("(i)", detectedKey);
  PyObject *result = PyEval_CallObject(onDTMFCallback, arglist);
  Py_DECREF(arglist);

  if (result == NULL) {
      DBG("Calling IVR" SCRIPT_TYPE "onDTMF failed.\n");
      // PyErr_Print();
      //return ;
  } else {
      Py_DECREF(result);
  }

  /* Release the thread. No Python API allowed beyond this point. */
  PyEval_ReleaseThread(tstate);

  /* You can either delete the thread state, or save it
     until you need it the next time. */
  PyThreadState_Delete(tstate);
#else   //IVR_PERL
  DBG("IvrPython::onDTMFEvent(): calling onDTMFCallback func is %s...\n", onDTMFCallback);

	PERL_SET_CONTEXT(my_perl_interp);
	DBG("context is %ld\n", (long) Perl_get_context());
	dSP ;
	ENTER ;
	SAVETMPS ;
	PUSHMARK(SP) ;
	XPUSHs(sv_2mortal(newSViv(detectedKey)));
	PUTBACK ;
	call_pv(onDTMFCallback, G_DISCARD);
	FREETMPS ;
	LEAVE ;
#endif	//IVR_PERL
  DBG("IvrPython::onDTMFEvent done...\n");
}


void IvrPython::onMediaQueueEmpty() {
  isMediaQueueEmpty.set(true);

    DBG("executiong MQE callback...\n");
    if (onMediaQueueEmptyCallback == NULL) {
	DBG("IvrPython::onMediaQueueEmpty, but script did not set onMediaQueueEmpty callback.\n");
	return;
    }

#ifndef IVR_PERL
    PyThreadState *tstate;

    /* interp is your reference to an interpreter object. */
    tstate = PyThreadState_New(mainInterpreterThreadState->interp);
    PyEval_AcquireThread(tstate);

    /* Perform Python actions here.  */
    PyObject *arglist = Py_BuildValue("()");
    PyObject *result = PyEval_CallObject(onMediaQueueEmptyCallback, arglist);
    Py_DECREF(arglist);

    if (result == NULL) {
	DBG("Calling IVR" SCRIPT_TYPE "onMediaQueueEmpty failed.\n");
	    // PyErr_Print();
	//return ;
    } else {
	Py_DECREF(result);
    }

    /* Release the thread. No Python API allowed beyond this point. */
    PyEval_ReleaseThread(tstate);

    /* You can either delete the thread state, or save it
       until you need it the next time. */
    PyThreadState_Delete(tstate);
#else   //IVR_PERL
	PERL_SET_CONTEXT(my_perl_interp);
	DBG("context is %ld\n", (long) Perl_get_context());

	dSP ;
	PUSHMARK(SP) ;
	call_pv(onMediaQueueEmptyCallback, G_DISCARD|G_NOARGS) ;
#endif	//IVR_PERL
    DBG("IvrPython::onMediaQueueEmpty done.\n");
}



