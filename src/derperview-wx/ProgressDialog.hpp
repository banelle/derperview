#include "derperview-wx.hpp"

class ProgressDialog : public wxDialog
{
public:
    ProgressDialog(wxWindow* parent);

    void UpdateFileProgress(int p);
    void StartFile(std::string filename);
    void CompleteFile();
    void StartBatch(int fileCount);
    void CompleteBatch();

private:
    wxGauge* totalProgress_;
    wxGauge* fileProgress_;
    wxTextCtrl* currentFile_;
};
