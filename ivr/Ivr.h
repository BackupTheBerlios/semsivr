/*
 * $Id: Ivr.h,v 1.4 2004/06/18 19:51:59 sayer Exp $
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

#ifndef _IVR_H_
#define _IVR_H_

// typedefs of the C callback functions
typedef void (*ivr_dtmf_callback_t)( int detectedKey );
typedef void (*ivr_media_queue_empty_callback_t)( void );


#define MOD_NAME "ivr"

#ifndef IVR_PERL
#define	SCRIPT_FILE_EXT			".py"
#define	SCRIPT_TYPE				" PYTHON "
#else	//IVR_PERL
#define	SCRIPT_FILE_EXT			".pl"
#define	SCRIPT_TYPE				" PERL "
#endif	//IVR_PERL

#include "AmApi.h"
#include "SemsConfiguration.h"

#ifdef IVR_WITH_TTS
 #include "flite.h"
#endif

#include <string>
#include <utility>

using std::string;
using std::auto_ptr;

class IvrPython;
class IvrMediaHandler;
struct IvrMediaEvent;

class IvrFactory: public AmStateFactory
{
	 SemsConfiguration mIvrConfig;
    string pythonScriptPath;
    string defaultPythonScriptFile;

#ifdef IVR_WITH_TTS
    bool tts_caching;
    string tts_cache_path;
#endif

public:
    IvrFactory(const string& _app_name);
    int onLoad();
    AmDialogState* onInvite(AmCmd&);
};


class IvrDialog : public AmDialogState
{
    string pythonScriptFile;
    //IvrPython ivrPython;
#ifdef IVR_WITH_TTS
    cst_voice* tts_voice;
    bool tts_caching;
    string tts_cache_path;
#endif

    void process(AmEvent* event); // we override this to process also mediaEvents
    int handleMediaEvent(IvrMediaEvent* evt); // which come here then

    auto_ptr<IvrMediaHandler> mediaHandler;
 public:
    IvrPython* ivrPython;
#ifndef IVR_WITH_TTS
    IvrDialog(string scriptFile);
#else
   IvrDialog(string scriptFile, string tts_cache_path_, bool tts_caching_);
#endif
    ~IvrDialog();

    void onSessionStart(AmRequest* req);
    void onBye(AmRequest* req);
    int onOther(AmSessionEvent* event);
};

#endif
