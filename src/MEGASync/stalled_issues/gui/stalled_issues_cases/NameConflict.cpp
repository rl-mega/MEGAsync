#include "NameConflict.h"
#include "ui_NameConflict.h"

#include "StalledIssueHeader.h"
#include "StalledIssueActionTitle.h"

#include <MegaApplication.h>
#include <StalledIssuesModel.h>
//#include "RenameDialog.h"
#include "QMegaMessageBox.h"

#include <QDialogButtonBox>

static const int RENAME_ID = 0;
static const int REMOVE_ID = 1;

NameConflict::NameConflict(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NameConflict)
{
    ui->setupUi(this);
    ui->path->setIndent(StalledIssueHeader::GROUPBOX_CONTENTS_INDENT);
}

NameConflict::~NameConflict()
{
    delete ui;
}

void NameConflict::updateUi(NameConflictedStalledIssue::NameConflictData data)
{
    ui->path->hide();

    if(data.data)
    {
        ui->path->show();
        ui->path->updateUi(data.data);
    }

    //Fill conflict names
    auto conflictedNames = data.conflictedNames;
    auto nameConflictWidgets = ui->nameConflicts->findChildren<StalledIssueActionTitle*>();
    auto totalUpdate(nameConflictWidgets.size() >= conflictedNames.size());

    for(int index = 0; index < conflictedNames.size(); ++index)
    {
        StalledIssueActionTitle* title(nullptr);
        if(totalUpdate)
        {
            title = nameConflictWidgets.first();
            nameConflictWidgets.removeFirst();
        }
        else
        {
            title = new StalledIssueActionTitle(this);
            title->setRoundedCorners(StalledIssueActionTitle::RoundedCorners::ALL_CORNERS);
            QIcon renameIcon(QString::fromUtf8("://images/StalledIssues/rename_node_default.png"));
            QIcon removeIcon(QString::fromUtf8("://images/StalledIssues/remove_default.png"));
            title->addActionButton(renameIcon, tr("Rename"), RENAME_ID);
            title->addActionButton(removeIcon, QString(), REMOVE_ID);

            connect(title, &StalledIssueActionTitle::actionClicked, this, &NameConflict::onActionClicked);

            ui->nameConflictsLayout->addWidget(title);
        }

        title->setTitle(conflictedNames.at(index).conflictedName);
        title->showIcon();

        if(title &&
                (!mData.data
                 || (mData.conflictedNames.size() > index
                 && (data.conflictedNames.at(index).isSolved() != mData.conflictedNames.at(index).isSolved()))))
        {
            bool isSolved(conflictedNames.at(index).isSolved());
            title->setDisabled(isSolved);
            if(isSolved)
            {
                title->hideActionButton(RENAME_ID);
                title->hideActionButton(REMOVE_ID);

                if(conflictedNames.at(index).solved ==  NameConflictedStalledIssue::ConflictedNameInfo::SolvedType::REMOVE)
                {
                    title->addMessage(tr("REMOVED"));
                }
                else
                {
                    title->addMessage(tr("RENAME TO %1").arg(conflictedNames.at(index).renameTo));
                }
            }
        }
    }

    //No longer exist conflicts
    foreach(auto titleToRemove, nameConflictWidgets)
    {
        removeConflictedNameWidget(titleToRemove);
    }

    mData = data;
}

void NameConflict::removeConflictedNameWidget(QWidget* widget)
{
    ui->nameConflictsLayout->removeWidget(widget);
    widget->deleteLater();
}

void NameConflict::onActionClicked(int actionId)
{
    if(auto chooseTitle = dynamic_cast<StalledIssueActionTitle*>(sender()))
    {
        QFileInfo info;
        info.setFile(mData.data->getNativePath(), chooseTitle->title());

        auto delegateWidget = dynamic_cast<StalledIssueBaseDelegateWidget*>(parentWidget());

        //As a dialog will be shown, avoid removing the editor while the dialog is open
        if(delegateWidget)
        {
            delegateWidget->setKeepEditor(true);
        }

        if(actionId == RENAME_ID)
        {
//            QPointer<NameConflict> currentWidget = QPointer<NameConflict>(this);

//            RenameDialog dialog;
//            dialog.init(mData.data->isCloud(), info.filePath());
//            auto result = dialog.exec();

//            if(result == QDialog::Accepted)
//            {
//                auto newName(dialog.newName());

//                if(mData.isCloud)
//                {
//                    MegaSyncApp->getStalledIssuesModel()->solveCloudConflictedNameByRename(chooseTitle->title()
//                                                                                           , newName, delegateWidget->getCurrentIndex());
//                }
//                else
//                {
//                    MegaSyncApp->getStalledIssuesModel()->solveLocalConflictedNameByRename(chooseTitle->title()
//                                                                                           , newName, delegateWidget->getCurrentIndex());
//                }

//                chooseTitle->setDisabled(true);
//            }

//            if(!currentWidget)
//            {
//                return;
//            }
        }
        else if(actionId == REMOVE_ID)
        {
            QPointer<NameConflict> currentWidget = QPointer<NameConflict>(this);

            if (QMegaMessageBox::warning(nullptr, QString::fromUtf8("MEGAsync"),
                                     tr("Are you sure you want to remove the %1 %2 %3?")
                                     .arg(mData.isCloud ? tr("remote") : tr("local"))
                                     .arg(info.isFile() ? tr("file") : tr("folder"))
                                     .arg(info.fileName()),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                    == QMessageBox::Yes
                    && currentWidget)
            {
                if(mData.isCloud)
                {
                    mUtilities.removeRemoteFile(info.filePath());
                    MegaSyncApp->getStalledIssuesModel()->solveCloudConflictedNameByRemove(chooseTitle->title(), delegateWidget->getCurrentIndex());
                }
                else
                {
                    mUtilities.removeLocalFile(QDir::toNativeSeparators(info.filePath()));
                    MegaSyncApp->getStalledIssuesModel()->solveLocalConflictedNameByRemove(chooseTitle->title(), delegateWidget->getCurrentIndex());
                }
            }
        }

        //Now, close the editor beacuse the action has been finished
        if(delegateWidget)
        {
            emit refreshUi();
            delegateWidget->setKeepEditor(false);
        }
    }
}