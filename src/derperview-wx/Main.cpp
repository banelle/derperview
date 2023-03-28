#include "derperview-wx.hpp"

#include "wx/dnd.h"
#include "wx/dataview.h"

#include <sstream>
#include <thread>

#include "version.hpp"
#include "ProgressDialog.hpp"
#include "WorkerThread.hpp"

using namespace std;

enum
{
    GO_BUTTON_ID = 100,
    ADD_FILES_BUTTON_ID,
    REMOVE_FILE_BUTTON_ID,
    REMOVE_ALL_BUTTON_ID
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
    void OnAddFiles(wxCommandEvent& event);
    void OnRemoveFile(wxCommandEvent& event);
    void OnRemoveAll(wxCommandEvent& event);

    void OnUpdateFileProgress(wxThreadEvent& ev);
    void OnFileStarted(wxThreadEvent& ev);
    void OnFileCompleted(wxThreadEvent& ev);
    void OnBatchStarted(wxThreadEvent& ev);
    void OnBatchCompleted(wxThreadEvent& ev);

    ProgressDialog* progressDialog_;
    wxDataViewListCtrl* fileListControl_;
    vector<string> filenameList_;
    bool cancelThread_;
};

class DerperViewApp : public wxApp
{
public:
    virtual bool OnInit();
};

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
    : wxFrame(nullptr, wxID_ANY, "DerperView"), cancelThread_(false)
{
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(wxID_EXIT);
 
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
 
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");

    progressDialog_ = new ProgressDialog(this, cancelThread_);
 
    SetMenuBar( menuBar );
 
    CreateStatusBar();
    SetStatusText("Ready");

    // fileListControl_ = new wxTextCtrl(this, wxID_ANY, _("Drop files here"), wxDefaultPosition, wxSize(400, 200), wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);

    fileListControl_ = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(400, 200), wxDV_SINGLE | wxDV_NO_HEADER | wxDV_ROW_LINES);
    fileListControl_->AppendTextColumn("Text");
    fileListControl_->SetDropTarget(new FileDropTarget(this));

    auto goButton = new wxButton(this, GO_BUTTON_ID, "Go");
    auto addFilesButton = new wxButton(this, ADD_FILES_BUTTON_ID, "Add Files");
    auto removeFileButton = new wxButton(this, REMOVE_FILE_BUTTON_ID, "Remove File");
    auto clearButton = new wxButton(this, REMOVE_ALL_BUTTON_ID, "Remove All");

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(fileListControl_, wxSizerFlags(1).Expand().Border(wxALL, 10));
    auto buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(goButton);
    buttonSizer->Add(addFilesButton);
    buttonSizer->Add(removeFileButton);
    buttonSizer->Add(clearButton);
    mainSizer->Add(buttonSizer, wxSizerFlags(0).Center().Border(wxBOTTOM, 10));
    SetSizerAndFit(mainSizer);

    Bind(wxEVT_MENU, &DerperViewFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &DerperViewFrame::OnAbout, this, wxID_ABOUT);

    Bind(wxEVT_BUTTON, &DerperViewFrame::OnGo, this, GO_BUTTON_ID);
    Bind(wxEVT_BUTTON, &DerperViewFrame::OnAddFiles, this, ADD_FILES_BUTTON_ID);
    Bind(wxEVT_BUTTON, &DerperViewFrame::OnRemoveFile, this, REMOVE_FILE_BUTTON_ID);
    Bind(wxEVT_BUTTON, &DerperViewFrame::OnRemoveAll, this, REMOVE_ALL_BUTTON_ID);

    Bind(DERPERVIEW_THREAD_PROGRESS_UPDATE, &DerperViewFrame::OnUpdateFileProgress, this);
    Bind(DERPERVIEW_THREAD_FILE_STARTED, &DerperViewFrame::OnFileStarted, this);
    Bind(DERPERVIEW_THREAD_FILE_COMPLETED, &DerperViewFrame::OnFileCompleted, this);
    Bind(DERPERVIEW_THREAD_BATCH_STARTED, &DerperViewFrame::OnBatchStarted, this);
    Bind(DERPERVIEW_THREAD_BATCH_COMPLETED, &DerperViewFrame::OnBatchCompleted, this);
}

void DerperViewFrame::FilesAdded(vector<string> filenames)
{
    for (auto filename : filenames)
    {
        if (find(filenameList_.begin(), filenameList_.end(), filename) == filenameList_.end())
        {
            filenameList_.push_back(filename);

            wxVector<wxVariant> data;
            data.push_back(wxVariant(filename));
            fileListControl_->AppendItem(data);
        }
    }
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

void DerperViewFrame::OnUpdateFileProgress(wxThreadEvent& ev)
{
    progressDialog_->UpdateFileProgress(ev.GetPayload<int>());
}

void DerperViewFrame::OnFileStarted(wxThreadEvent& ev)
{
    progressDialog_->StartFile(ev.GetPayload<string>());
}

void DerperViewFrame::OnFileCompleted(wxThreadEvent& ev)
{
    progressDialog_->CompleteFile();
}

void DerperViewFrame::OnBatchStarted(wxThreadEvent& ev)
{
    progressDialog_->StartBatch(ev.GetPayload<size_t>());
    progressDialog_->Show();
    Disable();
    SetStatusText("Derpin'");
}

void DerperViewFrame::OnBatchCompleted(wxThreadEvent& ev)
{
    progressDialog_->CompleteBatch();
    progressDialog_->Hide();
    Enable();
    SetStatusText("Ready");
}

void DerperViewFrame::OnGo(wxCommandEvent& event)
{
    cancelThread_ = false;
    auto workerThread = new WorkerThread(this, filenameList_, cancelThread_);
    workerThread->Run();
}

void DerperViewFrame::OnAddFiles(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, _("Add file(s)"), "", "", "Video files (*.mp4)|*.mp4", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    wxArrayString filenames;
    openFileDialog.GetFilenames(filenames);
    vector<string> filenameList;
    for (int i = 0; i < filenames.Count(); i++)
    {
        filenameList.push_back(filenames[i].ToStdString());
    }

    FilesAdded(filenameList);
}

void DerperViewFrame::OnRemoveFile(wxCommandEvent& event)
{
    auto selectedRow = fileListControl_->GetSelectedRow();
    if (selectedRow == wxNOT_FOUND)
        return;

    wxVariant value;
    fileListControl_->GetValue(value, selectedRow, 0);
    fileListControl_->DeleteItem(selectedRow);

    string filename = value.GetString().ToStdString();
    filenameList_.erase(remove(filenameList_.begin(), filenameList_.end(), filename), filenameList_.end());
}

void DerperViewFrame::OnRemoveAll(wxCommandEvent& event)
{
    filenameList_.clear();
    fileListControl_->DeleteAllItems();
}

wxIMPLEMENT_APP(DerperViewApp);
