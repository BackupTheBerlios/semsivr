/*
 * $Id: IvrPython.h,v 1.1 2004/06/07 13:00:23 sayer Exp $
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

#include <Python.h>
#include "AmThread.h"
#include "AmSession.h"
#include "IvrMediaHandler.h"
#include "IvrDtmfDetector.h"

#include <pthread.h>

#ifdef IVR_WITH_TTS
 #include "flite.h"
#endif

#define PY_MOD_NAME "ivr"

class IvrPython : public AmThread 
{
   private:
      
      void IvrPython::setPointer(const char* pythonPrName);
      //static void* pythonThread(void*);
      //pthread_t pPythonThread;

   public:
      AmCmd* pCmd;
      char* fileName;
      bool fileOpened;
      AmSession* pAmSession;
      AmCondition<bool> isEvent;

      //AmCondition<bool> pythonStopped;             

#ifdef IVR_WITH_TTS
      string tts_cache_path; 
      bool tts_caching;
      cst_voice* tts_voice;
#endif //IVR_WITH_TTS

      IvrPython();
      ~IvrPython();

      PyThreadState*  mainInterpreterThreadState;
      PyThreadState*  pyMainThreadState;

      auto_ptr<IvrMediaHandler> mediaHandler;

      void run();
      void on_stop();

      // events to pass on to script
      PyObject* onByeCallback;
      PyObject* onNotifyCallback;
      PyObject* onDTMFCallback;
      PyObject* onMediaQueueEmptyCallback;

      void onBye(AmRequest* req);
      void onNotify(AmSessionEvent* event);
      void onDTMFEvent(int detectedKey);
      void onMediaQueueEmpty();

      pthread_t getThreadDescriptor();
      /**
       * Python extension for file playing
       */
     // PyObject* ivrPlay(PyObject *self, PyObject *args);

      /**
       * Python extention for file recording
       */
     // PyObject* ivrRecord(PyObject *self, PyObject *args);

      /**
       * Python extention for file playing & DTMF detection
       */
     // PyObject* ivrPlayAndDetect(PyObject *self, PyObject *args);

      /**
       * Python extention for DTMF detection
       */
    //  PyObject* ivrDetect(PyObject *self, PyObject *args);

      /**
       * Python extention for getting time
       */
     // PyObject* ivrGetTime(PyObject *self, PyObject *args);

      /**
       * Python extention for caller name
       */
     // PyObject* ivrGetFrom(PyObject *self, PyObject *args);

      /**
       * Python extention for callee name
       */
     // PyObject* ivrGetTo(PyObject *self, PyObject *args);

      /**
       * Python extention for callee uri
       */
      // static PyObject* ivrGetToURI(PyObject *self, PyObject *args);

      /**
       * Python extention for caller uri
       */
      // static PyObject* ivrGetFromURI(PyObject *self, PyObject *args);

      /**
       * Python extention for domain name
       */
      //static PyObject* ivrGetDomain(PyObject *self, PyObject *args);

      /**
       * Python extention for redirection
       */
     // PyObject* ivrRedirect(PyObject *self, PyObject *args);

       
};



#endif
