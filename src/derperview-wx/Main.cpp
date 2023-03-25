#include "derperview-wx.hpp"

#include "wx/dnd.h"

#include <sstream>
#include <thread>

#include "libderperview.hpp"
#include "version.hpp"
#include "ProgressDialog.hpp"

using namespace std;

enum
{
    GO_BUTTON_ID = 100
};

class DerperViewFrame : public wxFrame
{
public:
    DerperViewFrame();

    void FilesAdded(vector<string> filenames);
 
private:
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnGo(wxCommandEvent& event);

    wxTextCtrl* fileListControl_;
    vector<string> filenameList_;

    wxDECLARE_EVENT_TABLE();
};

class DerperViewApp : public wxApp
{
public:
    virtual bool OnInit();
};

wxBEGIN_EVENT_TABLE(DerperViewFrame, wxFrame)
    EVT_MENU(wxID_EXIT, DerperViewFrame::OnExit)
    EVT_MENU(wxID_ABOUT, DerperViewFrame::OnAbout)
    EVT_BUTTON(GO_BUTTON_ID, DerperViewFrame::OnGo)
wxEND_EVENT_TABLE()

bool DerperViewApp::OnInit()
{
    auto frame = new DerperViewFrame();
    frame->Show(true);
    return true;
}

class FileDropTarget : public wxFileDropTarget
{
public:
    FileDropTarget(DerperViewFrame* parent) : parent_(parent) { }
    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);

private:
    DerperViewFrame* parent_;
};

DerperViewFrame::DerperViewFrame()
    : wxFrame(nullptr, wxID_ANY, "DerperView")
{
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(wxID_EXIT);
 
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
 
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");
 
    SetMenuBar( menuBar );
 
    CreateStatusBar();
    SetStatusText("Ready");

    fileListControl_ = new wxTextCtrl(this, wxID_ANY, _("Drop files onto me!"), wxDefaultPosition, wxSize(400, 200), wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
    fileListControl_->SetDropTarget(new FileDropTarget(this));

    auto goButton = new wxButton(this, GO_BUTTON_ID, "Go");

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(fileListControl_, wxSizerFlags().Expand());
    sizer->Add(goButton);
    SetSizerAndFit(sizer);
}

void DerperViewFrame::FilesAdded(vector<string> filenames)
{
    filenameList_.insert(filenameList_.end(), filenames.begin(), filenames.end());

    ostringstream joined;
    copy(filenameList_.begin(), filenameList_.end(),
        ostream_iterator<string>(joined, "\r\n"));
    fileListControl_->SetValue(joined.str());
}

bool FileDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
    vector<string> filenameList;
    for (int i = 0; i < filenames.Count(); i++)
    {
        filenameList.push_back(filenames[i].ToStdString());
    }

    parent_->FilesAdded(filenameList);

    return true;
}

void DerperViewFrame::OnExit(wxCommandEvent& event)
{
    Close(true);
}

void DerperViewFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("This is a wxWidgets Hello World example",
                 "About Hello World", wxOK | wxICON_INFORMATION);
}

void WorkerThread(ProgressDialog& progressDialog, vector<string> filenames)
{
    auto callback = [&dialog = progressDialog](int p)
    {
        dialog.UpdateFileProgressCallback(p);
    };

    ostringstream outputStream;
    for (auto filename : filenames)
    {
        progressDialog.UpdateCurrentFile(filename);
        Go(filename, filename + "out.mp4", 4, outputStream, callback);
        progressDialog.UpdateTotalProgress();
    }
}

void DerperViewFrame::OnGo(wxCommandEvent& event)
{
    ProgressDialog progressDialog(this, filenameList_.size());

    auto workerThread = thread(WorkerThread, ref(progressDialog), ref(filenameList_));

    progressDialog.ShowModal();

    workerThread.join();
}

wxIMPLEMENT_APP(DerperViewApp);