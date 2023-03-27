#include "derperview-wx.hpp"

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
