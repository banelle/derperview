#include "ProgressDialog.hpp"

using namespace std;

ProgressDialog::ProgressDialog(wxWindow* parent, bool& threadCancelled)
    : wxDialog(parent, wxID_ANY, "DerperView - working!", wxDefaultPosition, wxDefaultSize, wxCAPTION | wxSTAY_ON_TOP), threadCancelled_(threadCancelled)
{
    fileProgress_ = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(100, 20));
    totalProgress_ = new wxGauge(this, wxID_ANY, 1, wxDefaultPosition, wxSize(100, 20));
    currentFile_ = new wxStaticText(this, wxID_ANY, _(""), wxDefaultPosition, wxSize(250, 20), wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_START);

    cancelButton_ = new wxButton(this, wxID_CANCEL, "Cancel");
    Bind(wxEVT_BUTTON, &ProgressDialog::OnCancel, this, wxID_CANCEL);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(currentFile_, wxSizerFlags().Border(wxALL, 5));
    sizer->Add(fileProgress_, wxSizerFlags().Expand().Border(wxALL, 5));
    sizer->Add(totalProgress_, wxSizerFlags().Expand().Border(wxALL, 5));
    sizer->Add(cancelButton_, wxSizerFlags().Center().Border(wxALL, 5));
    SetSizerAndFit(sizer);
}

void ProgressDialog::UpdateFileProgress(int p)
{
    fileProgress_->SetValue(p);
}

void ProgressDialog::StartFile(string filename)
{
    fileProgress_->SetValue(0);
    currentFile_->SetLabelText(filename);
}

void ProgressDialog::CompleteFile()
{
    totalProgress_->SetValue(totalProgress_->GetValue() + 1);
}

void ProgressDialog::StartBatch(int fileCount)
{
    totalProgress_->SetValue(0);
    totalProgress_->SetRange(fileCount);
}

void ProgressDialog::CompleteBatch()
{

}

void ProgressDialog::OnCancel(wxCommandEvent& ev)
{
    threadCancelled_ = true;
}
