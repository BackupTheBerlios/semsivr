/*
 * $Id: IvrPython.cpp,v 1.12 2004/07/02 15:01:08 sayer Exp $
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

#ifdef	IVR_PERL
static IvrPython* mainIvrPython=NULL;
#endif	//IVR_PERL


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
	pIvrPython = mainIvrPython;
#endif	//IVR_PERL
    return pIvrPython;
  }

  SCRIPT_DECLARE_FUNC(ivrEnqueueMediaFile) {
    SCRIPT_DECLARE_VAR;
    char* fileName;
    int front = 1;
    if(pIvrPython != NULL){
      if(SCRIPT_GET_s_i(fileName, front)){
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
	SCRIPT_BEGIN_ALLOW_THREADS
	  unsigned int timediff = 0;
	timeval tvStart, tvNow;
	gettimeofday(&tvStart,0);
	pIvrPython->wakeUpFromSleep.set(false);
	while((!pIvrPython->wakeUpFromSleep.get()) && (timediff < (unsigned int) stime)){
	  usleep(10);
	  AmEventQueue* evq = pIvrPython->getScriptEventQueue();
	  if (evq)
	    evq->processEvents();
	  gettimeofday(&tvNow,0);
	  timediff = (tvNow.tv_sec - tvStart.tv_sec)* 1000000 + (tvNow.tv_usec - tvStart.tv_usec);
	}
	//	usleep(stime);
	SCRIPT_END_ALLOW_THREADS
	DBG("IVR: waking up after %d usec.\n", stime);
	SCRIPT_RETURN_i(1);
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
    int stime;
    if(pIvrPython != NULL){
      if(SCRIPT_GET_i(stime)){
	DBG("IVR: sleeping %d seconds.\n", stime);
	SCRIPT_BEGIN_ALLOW_THREADS
	  unsigned int timediff = 0;
	timeval tvStart, tvNow;
	gettimeofday(&tvStart,0);
	pIvrPython->wakeUpFromSleep.set(false);
	while((!pIvrPython->wakeUpFromSleep.get()) && (timediff < (unsigned int) stime*1000000)){
	  usleep(10);
 	  AmEventQueue* evq = pIvrPython->getScriptEventQueue();
 	  if (evq)
 	    evq->processEvents();
	  gettimeofday(&tvNow,0);
	  timediff = (tvNow.tv_sec - tvStart.tv_sec)* 1000000 + (tvNow.tv_usec - tvStart.tv_usec);
	}
	//	sleep(stime);
	SCRIPT_END_ALLOW_THREADS
	DBG("IVR: waking up after %d sec.\n", stime);
	SCRIPT_RETURN_i(1);
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
    if(SCRIPT_GET_s_i(ttsText, front)){
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
	newXS(PY_MOD_NAME"::""wakeUp", ivrWakeUp, file);
}
#endif
} // end of extern "C"

/***********************************************************************************************************
 *   IvrPython class functions
 *
 ***********************************************************************************************************
*/

IvrPython::IvrPython()
		: isEvent(false), onByeCallback(NULL), onNotifyCallback(0), 
		  onDTMFCallback(0), onMediaQueueEmptyCallback(0),
		  mediaEventQueue(0), regScriptEventProducer(0)
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
       {"wakeUp", ivrWakeUp, METH_VARARGS, "wake Up from sleep"},
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

	mainIvrPython = this;
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

void IvrPython::on_stop()
{
  if (regScriptEventProducer) {
    DBG("unregistering with script event producer\n");
    regScriptEventProducer->unregisterForeignEventQueue();
  }
}

void IvrPython::onDTMFEvent(int detectedKey) {
   if (onDTMFCallback == NULL) {
    DBG("IvrPython::onDTMFEvent, but script did not set onDTMF callback!\n(Caution: There must be something wrong here)\n");
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



