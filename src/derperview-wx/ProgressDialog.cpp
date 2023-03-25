#include "ProgressDialog.hpp"

using namespace std;

ProgressDialog::ProgressDialog(wxWindow* parent, int fileCount)
    : wxDialog(parent, wxID_ANY, "DerperView - working!")
{
    fileProgress_ = new wxGauge(this, wxID_ANY, 100);
    totalProgress_ = new wxGauge(this, wxID_ANY, fileCount);
    currentFile_ = new wxTextCtrl(this, wxID_ANY, _(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(currentFile_, wxSizerFlags().Expand());
    sizer->Add(fileProgress_, wxSizerFlags().Expand());
    sizer->Add(totalProgress_, wxSizerFlags().Expand());
    SetSizerAndFit(sizer);
}

void ProgressDialog::UpdateFileProgressCallback(int percentage) const
{
    fileProgress_->SetValue(percentage);
}

void ProgressDialog::UpdateTotalProgress() const
{
    totalProgress_->SetValue(totalProgress_->GetValue() + 1);
}

void ProgressDialog::UpdateCurrentFile(string filename) const
{
    fileProgress_->SetValue(0);
    currentFile_->SetLabelText(filename);
}
