/*
 * $Id: IvrPython.h,v 1.3 2004/06/18 19:51:59 sayer Exp $
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


#ifndef IVR_PYTHON_H
#define IVR_PYTHON_H

#ifndef IVR_PERL
#include <Python.h>
#include <compile.h>
#include <frameobject.h>
#else	//IVR_PERL
#include <EXTERN.h>                     /* from the Perl distribution */
#include <perl.h>                       /* from the Perl distribution */
#include <XSUB.h>                       /* from the Perl distribution */
#endif	//IVR_PERL
#include "AmThread.h"
#include "AmSession.h"
#include "IvrMediaHandler.h"
#include "IvrDtmfDetector.h"

#include <pthread.h>

#ifdef IVR_WITH_TTS
 #include "flite.h"
#endif

#define PY_MOD_NAME "ivr"

struct IvrScriptEvent: public AmEvent {
  int DTMFKey;
  AmSessionEvent* event;
  AmRequest* req;

  IvrScriptEvent(int event_id, int dtmf_Key);
  IvrScriptEvent(int event_id, AmRequest* req);
  IvrScriptEvent(int event_id, AmSessionEvent* event);
  IvrScriptEvent(int event_id);
    
  enum Action { 
    IVR_Bye,
    IVR_Notify,
    IVR_DTMF,
    IVR_MediaQueueEmpty
  };
};


class IvrPython : public AmThread, AmEventHandler
{
   private:

      void IvrPython::setPointer(const char* pythonPrName);
      //static void* pythonThread(void*);
      //pthread_t pPythonThread;

      auto_ptr<AmEventQueue> scriptEventQueue; //  this is our EvQ: we get bye, DTMF and media empty here 
      void process(AmEvent* event);  // we get events here


      void onBye(AmRequest* req);
      void onNotify(AmSessionEvent* event);
      void onDTMFEvent(int detectedKey);
      void onMediaQueueEmpty();

      //      int pythonTrace(PyObject *obj, struct _frame* /*PyFrameObject * */ frame, int what, PyObject *arg);

   public:
      AmCmd* pCmd;
      char* fileName;
      bool fileOpened;
      AmSession* pAmSession;
      AmCondition<bool> isEvent;

      AmEventQueue* mediaEventQueue;        //  we post events to be processed by media stream here

      //AmCondition<bool> pythonStopped;

#ifdef IVR_WITH_TTS
      string tts_cache_path;
      bool tts_caching;
      cst_voice* tts_voice;
#endif //IVR_WITH_TTS

      IvrPython(AmEventQueue* MediaEventQueue);
      ~IvrPython();

      void postScriptEvent(AmEvent* evt);
      AmEventQueue* getScriptEventQueue() { return scriptEventQueue.get(); }
#ifndef IVR_PERL
      PyThreadState*  mainInterpreterThreadState;
      PyThreadState*  pyMainThreadState;
#else   //IVR_PERL
      PerlInterpreter *my_perl_interp;  /***  The Perl interpreter    ***/
#endif  //IVR_PERL

      void run();
      void on_stop();

      // events to pass on to script
#ifndef IVR_PERL
      PyObject* onByeCallback;
      PyObject* onNotifyCallback;
      PyObject* onDTMFCallback;
      PyObject* onMediaQueueEmptyCallback;
#else	//IVR_PERL
      char* onByeCallback;
      char* onNotifyCallback;
      char* onDTMFCallback;
      char* onMediaQueueEmptyCallback;
#endif	//IVR_PERL

      pthread_t getThreadDescriptor();
};

#ifndef IVR_PERL
#define	SCRIPT_GET_s(sStr)	PyArg_ParseTuple(args,"s", &sStr)
#define SCRIPT_GET_i(iNum)  PyArg_ParseTuple(args,"i", &iNum)
#define SCRIPT_GET_s_i(sStr, iNum)	PyArg_ParseTuple(args,"s|i", &sStr, &iNum)
#define SCRIPT_GET_Os(oFunc, sName)	PyArg_ParseTuple(args, "Os:setCallback", &oFunc, &sName)
#define SCRIPT_RETURN_s(sStr)		return Py_BuildValue("s", (sStr))
#define SCRIPT_RETURN_i(iNum)		return Py_BuildValue("i", (iNum))
#define SCRIPT_RETURN_STR(sStr)		return PyString_FromString(sStr)
#define SCRIPT_RETURN_NULL			return NULL
#define SCRIPT_ERR_STRING(sStr)		PyErr_SetString(PyExc_TypeError, sStr)
#define SCRIPT_DECLARE_FUNC(Funcname) static PyObject* Funcname(PyObject *self, PyObject *args)
#define SCRIPT_DECLARE_VAR			IvrPython* pIvrPython = getIvrPythonPointer()
#define SCRIPT_BEGIN_ALLOW_THREADS	Py_BEGIN_ALLOW_THREADS
#define SCRIPT_END_ALLOW_THREADS	Py_END_ALLOW_THREADS
#else	//IVR_PERL
#define SCRIPT_GET_s(sStr)  ( (items!=1) ? 0 : \
			({STRLEN paralen; sStr = SvPV(ST(0), paralen); 1; }) )
#define SCRIPT_GET_i(iNum)  ( (items!=1) ? 0 : ({iNum = SvIV(ST(0)); 1; }) )
#define SCRIPT_GET_s_i(sStr,iNum)  ( ( (items<1) || (items>2) ) ? 0 : \
			({STRLEN paralen; sStr = SvPV(ST(0), paralen); (items==2) ? iNum=SvIV(ST(1)):0 ;1; }) )
#define SCRIPT_GET_Os(oFunc, sName)  ( (items!=2) ? 0 : \
			({STRLEN paralen; oFunc = SvPV(ST(0), paralen); sName = SvPV(ST(1), paralen); 1; } ) )
#define SCRIPT_RETURN_s(sStr)		XSRETURN_PV(sStr)
#define SCRIPT_RETURN_i(iNum)		XSRETURN_IV(iNum)
#define SCRIPT_RETURN_STR(sStr)		XSRETURN_PV(sStr)
#define SCRIPT_RETURN_NULL			XSRETURN_EMPTY
#define SCRIPT_ERR_STRING(sStr)		Perl_croak(aTHX_ (sStr))
#define SCRIPT_DECLARE_FUNC(Funcname) XS(Funcname)
#define SCRIPT_DECLARE_VAR		dXSARGS; IvrPython* pIvrPython = getIvrPythonPointer()
#define SCRIPT_BEGIN_ALLOW_THREADS
#define SCRIPT_END_ALLOW_THREADS
#endif	//IVR_PERL


#endif
