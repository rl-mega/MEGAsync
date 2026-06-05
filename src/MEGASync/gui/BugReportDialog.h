#ifndef BUGREPORTDIALOG_H
#define BUGREPORTDIALOG_H

#include "MegaSyncLogger.h"
#include "ProgressIndicatorDialog.h"

#include <QDialog>
#include <QPointer>

class QProgressDialog;
class BugReportController;

namespace Ui {
class BugReportDialog;
}

class BugReportDialog: public QDialog
{
    Q_OBJECT

public:
    explicit BugReportDialog(QWidget *parent, MegaSyncLogger& logger);
    ~BugReportDialog();

    void setReportObject(const QString& title);
    void setReportText(const QString& desc);

private slots:
    void onReportStarted();
    void onReportFinished();
    void onReportFailed();
    void onReportUploadedFinished();

    void onReportUpdated(int value);

private:
    bool isDescriptionValid() const;
    void setDescriptionErrorVisibility(bool visible);
    void closeProgressDialog();
    void openProgressDialog();

    Ui::BugReportDialog* ui;
    QPointer<ProgressIndicatorDialog> mProgressIndicatorDialog;
    std::unique_ptr<BugReportController> mController;

    static constexpr int mMinDescriptionLength = 30;
    static constexpr int mMaxDescriptionLength = 3000;

private slots:
    void onSubmitClicked();
    void onCancelClicked();
    void cancelSendReport();
    void onClearDescriptionClicked();
    void onDescriptionChanged();
    void onTitleChanged();
    void onDescribeBugTextChanged();
};

#endif // BUGREPORTDIALOG_H
