#include "derperview-wx.hpp"

class ProgressDialog : public wxDialog
{
public:
    ProgressDialog(wxWindow* parent, int fileCount);

    void UpdateFileProgressCallback(int percentage) const;
    void UpdateTotalProgress() const;
    void UpdateCurrentFile(std::string filename) const;

private:
    wxGauge* totalProgress_;
    wxGauge* fileProgress_;
    wxTextCtrl* currentFile_;
};
