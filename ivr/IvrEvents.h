/*
 * $Id: IvrEvents.h,v 1.1 2004/06/29 15:50:59 sayer Exp $
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

#ifndef _IVR_EVENTS_H_
#define _IVR_EVENTS_H_

#include "AmThread.h"
#include "AmSession.h"

/*
 * "interface" EventProducer
 * events generated will be fed into the registered event queue
 */
class IvrEventProducer {
 public: 
  IvrEventProducer();
  virtual ~IvrEventProducer() { };
  virtual int registerForeignEventQueue(AmEventQueue* destinationEventQueue)=0;
  virtual void unregisterForeignEventQueue()=0;
};


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


struct IvrMediaEvent: public AmEvent {
  string MediaFile; 
  bool front;
  
  IvrMediaEvent(int event_id, string MediaFile = "" , bool front = true); 
    
  enum Action { 
    IVR_enqueueMediaFile,
    IVR_emptyMediaQueue,
    IVR_startRecording, 
    IVR_stopRecording, 
    IVR_enableDTMFDetection, 
    IVR_disableDTMFDetection,
    IVR_pauseDTMFDetection,
    IVR_resumeDTMFDetection
  };
};


/*
 * class IvrScriptEventProcessor 
 * Another thread to process events in the script interpreter. 
 */
#ifdef IVR_PERL
#define SCRIPT_EVENT_CHECK_INTERVAL_US 500 
class IvrScriptEventProcessor : public AmThread {
  AmSharedVar<bool> runcond;
  AmEventQueue* q;
 public:
  IvrScriptEventProcessor(AmEventQueue* watchThisQueue);
  ~IvrScriptEventProcessor();
  void run();
  void on_stop();
};
#endif // IVR_PERL



#endif
