#include "derperview-wx.hpp"

wxDECLARE_EVENT(DERPERVIEW_THREAD_PROGRESS_UPDATE, wxThreadEvent);
wxDECLARE_EVENT(DERPERVIEW_THREAD_FILE_STARTED, wxThreadEvent);
wxDECLARE_EVENT(DERPERVIEW_THREAD_FILE_COMPLETED, wxThreadEvent);
wxDECLARE_EVENT(DERPERVIEW_THREAD_BATCH_STARTED, wxThreadEvent);
wxDECLARE_EVENT(DERPERVIEW_THREAD_BATCH_COMPLETED, wxThreadEvent);

class WorkerThread : public wxThread
{
public:
    WorkerThread(wxFrame* parent, std::vector<std::string>& filenames, bool& cancelThread)
        : wxThread(wxTHREAD_DETACHED), parent_(parent), filenames_(filenames), cancelThread_(cancelThread) { }

protected:
    virtual ExitCode Entry();

    wxFrame* parent_;
    std::vector<std::string> filenames_;
    bool& cancelThread_;
};
