#pragma once

#define WXUSINGDLL

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

wxDEFINE_EVENT(DERPERVIEW_THREAD_PROGRESS_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(DERPERVIEW_THREAD_FILE_STARTED, wxThreadEvent);
wxDEFINE_EVENT(DERPERVIEW_THREAD_FILE_COMPLETED, wxThreadEvent);
wxDEFINE_EVENT(DERPERVIEW_THREAD_BATCH_STARTED, wxThreadEvent);
wxDEFINE_EVENT(DERPERVIEW_THREAD_BATCH_COMPLETED, wxThreadEvent);