/*
 * $Id: IvrEvents.cpp,v 1.1 2004/06/29 15:50:59 sayer Exp $
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

#include "IvrEvents.h"
#include "log.h"

IvrEventProducer::IvrEventProducer() {
}

IvrMediaEvent::IvrMediaEvent(int event_id, string MediaFile, bool front) 
    :    AmEvent(event_id), MediaFile(MediaFile), front(front)
{
    DBG("New Media Event: %d, %s\n", event_id, MediaFile.c_str());
}

// the events we get
IvrScriptEvent::IvrScriptEvent(int event_id, int dtmf_Key)
  : AmEvent(event_id), DTMFKey(dtmf_Key)
{
  assert(event_id == IVR_DTMF);
}
IvrScriptEvent::IvrScriptEvent(int event_id, AmRequest* req)
  : AmEvent(event_id), req(req)
{
  assert(event_id == IVR_Bye);
}

IvrScriptEvent::IvrScriptEvent(int event_id, AmSessionEvent* event)
  : AmEvent(event_id), event(event)
{
  assert(event_id == IVR_Notify);
}

IvrScriptEvent::IvrScriptEvent(int event_id)
  : AmEvent(event_id)
{
  assert(event_id == IVR_MediaQueueEmpty);
}

