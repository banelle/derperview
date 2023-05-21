#include "derperview-wx.hpp"

class ProgressDialog : public wxDialog
{
public:
    ProgressDialog(wxWindow* parent, bool& threadCancelled);

    void UpdateFileProgress(int p);
    void StartFile(std::string filename);
    void CompleteFile();
    void StartBatch(int fileCount);
    void CompleteBatch();

private:
    void OnCancel(wxCommandEvent& ev);

    bool& threadCancelled_;

    wxGauge* totalProgress_;
    wxGauge* fileProgress_;
    wxStaticText* currentFile_;
    wxButton* cancelButton_;
};
