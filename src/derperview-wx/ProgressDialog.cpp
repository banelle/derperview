#include "ProgressDialog.hpp"

using namespace std;

ProgressDialog::ProgressDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "DerperView - working!")
{
    fileProgress_ = new wxGauge(this, wxID_ANY, 100);
    totalProgress_ = new wxGauge(this, wxID_ANY, 1);
    currentFile_ = new wxTextCtrl(this, wxID_ANY, _(""), wxDefaultPosition, wxSize(250, 40), wxTE_READONLY);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(currentFile_, wxSizerFlags().Expand());
    sizer->Add(fileProgress_, wxSizerFlags().Expand());
    sizer->Add(totalProgress_, wxSizerFlags().Expand());
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
    totalProgress_->SetRange(fileCount);
}

void ProgressDialog::CompleteBatch()
{

}
