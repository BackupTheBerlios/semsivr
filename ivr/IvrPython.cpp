/*
 * $Id: IvrPython.cpp,v 1.1 2004/06/07 13:00:23 sayer Exp $
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

#include <Python.h>
#include "log.h"
#include "AmApi.h"
#include "Ivr.h"
#include "IvrDtmfDetector.h"
#include "IvrMediaHandler.h"
#include "IvrPython.h"

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
/***********************************************************************************************************
 *   Python  extensions
 *
 ***********************************************************************************************************
 */
extern "C" {

  static IvrPython* getIvrPythonPointer(){
    IvrPython* pIvrPython = NULL;
    PyObject *module = PyImport_ImportModule(PY_MOD_NAME);
    if (module != NULL) {
      PyObject *ivrPythonPointer = PyObject_GetAttrString(module, "ivrPythonPointer");
      if (ivrPythonPointer != NULL){
	if (PyCObject_Check(ivrPythonPointer))
	  pIvrPython = (IvrPython*)PyCObject_AsVoidPtr(ivrPythonPointer);
	Py_DECREF(ivrPythonPointer);
      }
    }
    return pIvrPython;   
  }

  static PyObject* ivrEnqueueMediaFile(PyObject *self, PyObject *args){
    char* fileName;
    int front = 1;
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){  
      if(PyArg_ParseTuple(args,"s|i", &fileName, &front)){
	string sFileName(fileName);
	DBG("IVR: enqueuing media file (%s) at the %s",fileName, (front==1?"front\n":"back\n") );
	//Py_BEGIN_ALLOW_THREADS
	pIvrPython->mediaHandler->eventQueue.postEvent(
	    new IvrMediaEvent(IvrMediaHandler::IVR_enqueueMediaFile, sFileName, front));
	pIvrPython->mediaHandler->eventQueue.processEvents();
	    DBG("IVR: finished enqueue.\n");
	//Py_END_ALLOW_THREADS 
	return Py_BuildValue("i",1);
      }
      else {
	PyErr_SetString(PyExc_TypeError, "ivrEnqueueMediaFile: parameter mismatch!\n"
			"Wanted: filename:string, front=1 (default, optional, 0=at the back)");
	return NULL; // raise exception
      }
    } else {
	PyErr_SetString(PyExc_TypeError, "IVR Python Error: Wrong pointer to IvrPython!");
	return NULL; // raise exception
    }
  }

  static PyObject* ivrEmptyMediaQueue(PyObject *self, PyObject *args){
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){  
      DBG("IVR: emptying  media queue.\n");
      pIvrPython->mediaHandler->eventQueue.postEvent(
	    new IvrMediaEvent(IvrMediaHandler::IVR_emptyMediaQueue));
      pIvrPython->mediaHandler->eventQueue.processEvents();
      return Py_BuildValue("i",1);
    }   else {
      PyErr_SetString(PyExc_TypeError, "IVR Python Error: Wrong pointer to IvrPython!");
      return NULL; 
    }
  }

  static PyObject* ivrStartRecording(PyObject *self, PyObject *args) {
    char* fileName;
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){  
      if(PyArg_ParseTuple(args,"s", &fileName)){
	string sFileName(fileName);
	DBG("IVR: start recording to file (%s)\n",fileName);
       	pIvrPython->mediaHandler->eventQueue.postEvent(
	    new IvrMediaEvent(IvrMediaHandler::IVR_startRecording, sFileName));
       	pIvrPython->mediaHandler->eventQueue.processEvents();
	return Py_BuildValue("i",1);
      } else {
	return PyString_FromString("IVR Python Error: Wrong Arguments!");
      }
    } else {
      PyErr_SetString(PyExc_TypeError, "IVR Python Error: Wrong pointer to IvrPython!");
      return NULL;
    }  
  }

  static PyObject* ivrStopRecording(PyObject *self, PyObject *args){
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){  
      DBG("IVR: stop recording.\n");
      pIvrPython->mediaHandler->eventQueue.postEvent(
	    new IvrMediaEvent(IvrMediaHandler::IVR_stopRecording));
      pIvrPython->mediaHandler->eventQueue.processEvents();
      return Py_BuildValue("i",1);
    }   else {
      PyErr_SetString(PyExc_TypeError, "IVR Python Error: Wrong pointer to IvrPython!");
      return NULL; 
    }
  }

  static PyObject* ivrEnableDTMFDetection(PyObject *self, PyObject *args){
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){  
      DBG("IVR: enable DTMF Detection.\n");
      pIvrPython->mediaHandler->eventQueue.postEvent(
	    new IvrMediaEvent(IvrMediaHandler::IVR_enableDTMFDetection));
      pIvrPython->mediaHandler->eventQueue.processEvents();
      return Py_BuildValue("i",1);
    }   else {
      PyErr_SetString(PyExc_TypeError, "IVR Python Error: Wrong pointer to IvrPython!");
      return NULL; 
    }
  }
  
  static PyObject* ivrDisableDTMFDetection(PyObject *self, PyObject *args){
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){  
      DBG("IVR: disable DTMF Detection.\n");
      pIvrPython->mediaHandler->eventQueue.postEvent(
	    new IvrMediaEvent(IvrMediaHandler::IVR_disableDTMFDetection));
      pIvrPython->mediaHandler->eventQueue.processEvents();
      return Py_BuildValue("i",1);
    }   else {
      PyErr_SetString(PyExc_TypeError, "IVR Python Error: Wrong pointer to IvrPython!");
      return NULL; 
    }
  }
  
  static PyObject* ivrPauseDTMFDetection(PyObject *self, PyObject *args){
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){  
      DBG("IVR: pause DTMF Detection.\n");
      pIvrPython->mediaHandler->eventQueue.postEvent(
	    new IvrMediaEvent(IvrMediaHandler::IVR_pauseDTMFDetection));
      pIvrPython->mediaHandler->eventQueue.processEvents();
      return Py_BuildValue("i",1);
    }   else {
      PyErr_SetString(PyExc_TypeError, "IVR Python Error: Wrong pointer to IvrPython!");
      return NULL; 
    }
  }

  
  static PyObject* ivrResumeDTMFDetection(PyObject *self, PyObject *args){
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){  
      DBG("IVR: resume DTMF Detection.\n");
      pIvrPython->mediaHandler->eventQueue.postEvent(
	    new IvrMediaEvent(IvrMediaHandler::IVR_resumeDTMFDetection));
      pIvrPython->mediaHandler->eventQueue.processEvents();
      return Py_BuildValue("i",1);
    }   else {
      PyErr_SetString(PyExc_TypeError, "IVR Python Error: Wrong pointer to IvrPython!");
      return NULL; 
    }
  }

  static PyObject* ivrUSleep(PyObject *self, PyObject *args) {
    int stime;
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){  
      if(PyArg_ParseTuple(args,"i", &stime)){
	DBG("IVR: sleeping %d useconds.\n", stime);
	Py_BEGIN_ALLOW_THREADS
	usleep(stime);
	Py_END_ALLOW_THREADS
	DBG("IVR: waking up after %d usec.\n", stime);
	return Py_BuildValue("i",1);
      } else {
	return PyString_FromString("IVR Python Error: Wrong Arguments!");
      }
    } else {
      PyErr_SetString(PyExc_TypeError, "IVR Python Error: Wrong pointer to IvrPython!");
      return NULL;
    }  
  }


  static PyObject* ivrSleep(PyObject *self, PyObject *args) {
    int stime;
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){  
      if(PyArg_ParseTuple(args,"i", &stime)){
	DBG("IVR: sleeping %d seconds.\n", stime);
	Py_BEGIN_ALLOW_THREADS
	sleep(stime);
	Py_END_ALLOW_THREADS
	DBG("IVR: waking up after %d sec.\n", stime);
	return Py_BuildValue("i",1);
      } else {
	return PyString_FromString("IVR Python Error: Wrong Arguments!");
      }
    } else {
      PyErr_SetString(PyExc_TypeError, "IVR Python Error: Wrong pointer to IvrPython!");
      return NULL;
    }  
  }

  /**
   * Python extention for getting time
   */
  static PyObject* ivrGetTime(PyObject *self, PyObject *args){
    return Py_BuildValue("i",(int)time(NULL));
  }
  

  /**
   * Python extention for caller name
   */
  static PyObject* ivrGetFrom(PyObject *self, PyObject *args){
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){ 
      DBG("ivrGetFrom: returning %s\n", pIvrPython->pCmd->from.c_str()); 
      return Py_BuildValue("s",pIvrPython->pCmd->from.c_str());
    }
    else {
      return PyString_FromString("IVR Python Error: Wrong pointer to IvrPython!");
    }
  }
  
  
  /**
   * Python extention for callee name
   */
  static PyObject* ivrGetTo(PyObject *self, PyObject *args){
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){ 
      return Py_BuildValue("s",pIvrPython->pCmd->to.c_str());
    }
    else
      return PyString_FromString("IVR Python Error: Wrong pointer to IvrPython!");
  }
  

  /**
      * Python extention for caller uri
      */
  static PyObject* ivrGetFromURI(PyObject *self, PyObject *args){
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){
      return Py_BuildValue("s",pIvrPython->pCmd->from_uri.c_str());
    }
    else
      return PyString_FromString("IVR Python Error: Wrong pointer to IvrPython!");
  }


  /**
   * Python extention for callee uri
   */
  static PyObject* ivrGetToURI(PyObject *self, PyObject *args){
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){
      return Py_BuildValue("s",pIvrPython->pCmd->r_uri.c_str());
    }
    else
      return PyString_FromString("IVR Python Error: Wrong pointer to IvrPython!");
  }


  /**
   * Python extention for domain name
   */
  static PyObject* ivrGetDomain(PyObject *self, PyObject *args){
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){
      return Py_BuildValue("s",pIvrPython->pCmd->domain.c_str());
    }
    else
      return PyString_FromString("IVR Python Error: Wrong pointer to IvrPython!");
  }

   
  /**
   * Python extention for redirection
   */
  static PyObject* ivrRedirect(PyObject *self, PyObject *args){
   //  char* uri;

//     IvrPython* pIvrPython = getIvrPythonPointer();
//     if(pIvrPython != NULL){
//       if(PyArg_ParseTuple(args,"s", &uri)){
// 	string refer_to(uri);
// 	Py_BEGIN_ALLOW_THREADS
// 	  AmCmd refer_cmd = AmRequestUAC::refer(*pIvrPython->pCmd,refer_to);
// 	Py_END_ALLOW_THREADS
// 	  // was send
// 	  if(pIvrPython->waitForEvent(30000))
//             return Py_BuildValue("i",1);
//       }
//       // error
//       DBG("Redirect timeout\n");
//       return Py_BuildValue("i",0);
//     }
//     else
//       return PyString_FromString("IVR Python Error: Wrong pointer to IvrPython!");
    return NULL;
  }


#ifdef IVR_WITH_TTS

static PyObject* ivrSay(PyObject *self, PyObject *args){
  char* ttsText;

  int front = 1;
  IvrPython* pIvrPython = getIvrPythonPointer();
  if(pIvrPython != NULL){  
    if(PyArg_ParseTuple(args,"s|i", &ttsText, &front)){
      string message(ttsText);
      string msg_filename = string("/tmp/") +  pIvrPython->pCmd->callid + string(".wav");
      string cache_filename = pIvrPython->tts_cache_path + message + string(".wav");
      if (pIvrPython->tts_caching) {
	DBG(" trying cache \"%s\" .. ", cache_filename.c_str());
	if (file_exists(cache_filename)) {
	  DBG("hit. Playing from cache.\n");
	  pIvrPython->mediaHandler->eventQueue.postEvent(
	      new IvrMediaEvent(IvrMediaHandler::IVR_enqueueMediaFile, cache_filename, front));
	  pIvrPython->mediaHandler->eventQueue.processEvents();
	  return Py_BuildValue("i",1);
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
      
      pIvrPython->mediaHandler->eventQueue.postEvent(
	  new IvrMediaEvent(IvrMediaHandler::IVR_enqueueMediaFile, cache_filename, front));
      pIvrPython->mediaHandler->eventQueue.processEvents();

      return Py_BuildValue("i",1);
    } else {
      PyErr_SetString(PyExc_TypeError, "ivrEnqueueMediaFile: parameter mismatch!\n"+
		      "Wanted: filename:string, front=1 (default, optional, 0=at the back)");
      return NULL; // raise exception
    }
  } else {
    PyErr_SetString("IVR Python Error: Wrong pointer to IvrPython!");
    return NULL; // raise exception
  }
}

#endif // IVR_WITH_TTS
  
  static PyObject* setCallback(PyObject *dummy, PyObject *args)
  {
    DBG("setCallback !\n");
    PyObject *result = NULL;
    PyObject *temp;
    char* callbackName;
    
    IvrPython* pIvrPython = getIvrPythonPointer();
    if(pIvrPython != NULL){
      if (PyArg_ParseTuple(args, "Os:setCallback", &temp, &callbackName)) {
					if (!PyCallable_Check(temp)) {
							PyErr_SetString(PyExc_TypeError, "setCallback: first parameter must be callable");
							return NULL;
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
							PyErr_SetString(PyExc_TypeError, "setCallback: second parameter must be event name.");
							return NULL;
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
      return PyString_FromString("IVR Python Error: Wrong pointer to IvrPython!");
  }

} // end of extern "C"

/***********************************************************************************************************
 *   IvrPython class functions
 *
 ***********************************************************************************************************
*/

IvrPython::IvrPython()
		: isEvent(false), onByeCallback(NULL), onNotifyCallback(0), onDTMFCallback(0), onMediaQueueEmptyCallback(0) 
#ifdef IVR_WITH_TTS
    , tts_voice(0)
#endif //IVR_WITH_TTS
{
  mediaHandler.reset(new IvrMediaHandler(this));
}

IvrPython::~IvrPython(){

}

void IvrPython::run(){
   FILE* fp;
   int retval;
   
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0); // enable cancellation of thread if session is stopped

//   static PyThreadState* pyMainThreadState;
   fp = fopen((char*)fileName,"r");
   if(fp != NULL){
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

       {"sleep", ivrSleep, METH_VARARGS, "Example Module"},
       {"usleep", ivrUSleep, METH_VARARGS, "Example Module"},

       {NULL, NULL, 0, NULL},
     };

     if(!Py_IsInitialized()){
       DBG("Start Python\n");
       Py_Initialize();
       PyEval_InitThreads();
       pyMainThreadState = PyEval_SaveThread();         
     }       
     DBG("Start new Python interpreter\n");
     PyEval_AcquireLock();
//     PyThreadState* pyThreadState;      
     if ( (mainInterpreterThreadState = Py_NewInterpreter()) != NULL){
       
       PyObject* ivrPyInitModule = Py_InitModule(PY_MOD_NAME, extIvrPython);
       PyObject* ivrPythonPointer = PyCObject_FromVoidPtr((void*)this,NULL);
       if (ivrPythonPointer != NULL)
	 PyModule_AddObject(ivrPyInitModule, "ivrPythonPointer", ivrPythonPointer);
       
       if(!PyRun_SimpleFile(fp,(char*)fileName)){
	 fclose(fp);
	 retval = 0;// true;
       }
       else{
            PyErr_Print();
            ERROR("IVR Python Error: Failed to run \"%s\"\n", (char*)fileName);
	    retval = -1;// false;
       }
       
       Py_EndInterpreter(mainInterpreterThreadState);
     }
     else{
       ERROR("IVR Python Error: Failed to start new interpreter.\n");
     }
     PyEval_ReleaseLock();
   }
   else{
     ERROR("IVR Python Error: Can not open file \"%s\"\n",(char*) fileName);
     retval = -1;// false;
   }
   DBG("IVR: run finished. stopping rtp stream...\n");
   pAmSession->rtp_str.pause();
}

void IvrPython::onBye(AmRequest* req) {
  if (onByeCallback == NULL) {
    DBG("Python script did not set onBye callback!\n");
    return;
  }
  DBG("IvrPython::onBye(): calling onByeCallback ...\n");
  
  PyThreadState *tstate;

  tstate = PyThreadState_New(mainInterpreterThreadState->interp);
  PyEval_AcquireThread(tstate);

  /* Perform Python actions here.  */
  PyObject *arglist = Py_BuildValue("()");
  PyObject *result = PyEval_CallObject(onByeCallback, arglist);
  Py_DECREF(arglist);

  if (result == NULL) {
      DBG("Calling IVR Python onMediaQueueEmpty failed.\n");
      // PyErr_Print();
      //return ;
  } else {
      Py_DECREF(result);
  }

  /* Release the thread. No Python API allowed beyond this point. */
  PyEval_ReleaseThread(tstate);
  PyThreadState_Delete(tstate);
  DBG("IvrPython::onBye done...\n");
}


void IvrPython::onNotify(AmSessionEvent* event) {
   if (onByeCallback == NULL) {
    DBG("IvrPython::onNotify, but script did not set onNotify callback!\n");
    return;
  }
  DBG("IvrPython::onNotify(): calling onNotifyCallback ...\n");
  PyThreadState* pyThreadState;      
  if ( (pyThreadState = Py_NewInterpreter()) != NULL){  
    PyObject *arglist;
    PyObject *result;
    arglist = Py_BuildValue("(s)", event->request.getBody().c_str());;
    result = PyEval_CallObject(onNotifyCallback, arglist);
    Py_DECREF(arglist);
    if (result == NULL) {
      DBG("Calling IVR Python onNotify failed.\n");
      // PyErr_Print();
      return ;
    }
    Py_DECREF(result);
  } 
  Py_EndInterpreter(pyThreadState);
}

void IvrPython::on_stop()
{
  //  pthread_cancel(0);
}

pthread_t IvrPython::getThreadDescriptor() {
  return 0; //  return _td;
}

void IvrPython::onDTMFEvent(int detectedKey) {
   if (onByeCallback == NULL) {
    DBG("IvrPython::onDTMFEvent, but script did not set onDTMF callback!\n(Caution: There must be something wrong here)\n");
    return;
  }
  DBG("IvrPython::onDTMFEvent(): calling onDTMFCallback ...\n");

  PyThreadState *tstate;

  /* interp is your reference to an interpreter object. */
  tstate = PyThreadState_New(mainInterpreterThreadState->interp);
  PyEval_AcquireThread(tstate);

  /* Perform Python actions here.  */
  PyObject *arglist = Py_BuildValue("(i)", detectedKey);
  PyObject *result = PyEval_CallObject(onDTMFCallback, arglist);
  Py_DECREF(arglist);

  if (result == NULL) {
      DBG("Calling IVR Python onMediaQueueEmpty failed.\n");
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
  DBG("IvrPython::onDTMFEvent done...\n");
}


void IvrPython::onMediaQueueEmpty() {
    DBG("executiong MQE callback...\n");
    if (onMediaQueueEmptyCallback == NULL) {
	DBG("IvrPython::onMediaQueueEmpty, but script did not set onMediaQueueEmpty callback.\n");
	return;
    }

    PyThreadState *tstate;

    /* interp is your reference to an interpreter object. */
    tstate = PyThreadState_New(mainInterpreterThreadState->interp);
    PyEval_AcquireThread(tstate);

    /* Perform Python actions here.  */
    PyObject *arglist = Py_BuildValue("()");
    PyObject *result = PyEval_CallObject(onMediaQueueEmptyCallback, arglist);
    Py_DECREF(arglist);

    if (result == NULL) {
	DBG("Calling IVR Python onMediaQueueEmpty failed.\n");
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
    DBG("IvrPython::onMediaQueueEmpty done.\n");
}



