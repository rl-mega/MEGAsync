#ifndef SOMEISSUESOCCURREDMESSAGE_H
#define SOMEISSUESOCCURREDMESSAGE_H

#include <DialogGeometryRetainer.h>
#include <StalledIssuesDialog.h>

#include <QWidget>
#include <QPointer>

namespace Ui {
class SomeIssuesOccurredMessage;
}

class StalledIssuesDialog;

class SomeIssuesOccurredMessage : public QWidget
{
    Q_OBJECT

public:
    explicit SomeIssuesOccurredMessage(QWidget *parent = nullptr);
    ~SomeIssuesOccurredMessage();

private slots:
    void on_viewIssuesButton_clicked();

private:
    Ui::SomeIssuesOccurredMessage *ui;
    static QPointer<StalledIssuesDialog> mStalledIssuesDialog;
    DialogGeometryRetainer<StalledIssuesDialog> mStalledIssuesGeometryRetainer;
    bool mFoundStalledIssues;
};

#endif // SOMEISSUESOCCURREDMESSAGE_H