#include "WorkerThread.hpp"

#include <sstream>

#include "libderperview.hpp"

using namespace std;

wxDEFINE_EVENT(DERPERVIEW_THREAD_PROGRESS_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(DERPERVIEW_THREAD_FILE_STARTED, wxThreadEvent);
wxDEFINE_EVENT(DERPERVIEW_THREAD_FILE_COMPLETED, wxThreadEvent);
wxDEFINE_EVENT(DERPERVIEW_THREAD_BATCH_STARTED, wxThreadEvent);
wxDEFINE_EVENT(DERPERVIEW_THREAD_BATCH_COMPLETED, wxThreadEvent);

template <class T>
wxThreadEvent* CreateThreadEventWithPayload(wxEventType eventType, T payload)
{
    auto ev = new wxThreadEvent(eventType);
    ev->SetPayload(payload);
    return ev;
}

wxThread::ExitCode WorkerThread::Entry()
{
    auto callback = [&parent = parent_](int p)
    {
        wxQueueEvent(parent, CreateThreadEventWithPayload(DERPERVIEW_THREAD_PROGRESS_UPDATE, p));
    };

    wxQueueEvent(parent_, CreateThreadEventWithPayload(DERPERVIEW_THREAD_BATCH_STARTED, filenames_.size()));

    ostringstream outputStream;
    for (auto filename : filenames_)
    {
        wxQueueEvent(parent_, CreateThreadEventWithPayload(DERPERVIEW_THREAD_FILE_STARTED, filename));
        Go(filename, filename + ".out.mp4", 4, outputStream, callback, cancelThread_);
        wxQueueEvent(parent_, new wxThreadEvent(DERPERVIEW_THREAD_FILE_COMPLETED));
    }

    wxQueueEvent(parent_, new wxThreadEvent(DERPERVIEW_THREAD_BATCH_COMPLETED));
    return (wxThread::ExitCode)0;
}
