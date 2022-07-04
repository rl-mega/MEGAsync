#include "StalledIssuesDialog.h"
#include "ui_StalledIssuesDialog.h"

#include "MegaApplication.h"
#include "StalledIssuesModel.h"
#include "StalledIssuesProxyModel.h"
#include "StalledIssueDelegate.h"
#include "StalledIssuesProxyModel.h"

#include "Utilities.h"

StalledIssuesDialog::StalledIssuesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StalledIssuesDialog),
    mCurrentTab(StalledIssueFilterCriterion::ALL_ISSUES),
    mProxyModel(nullptr),
    mDelegate(nullptr)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose, true);

    mProxyModel = new StalledIssuesProxyModel(this);
    mProxyModel->setSourceModel(MegaSyncApp->getStalledIssuesModel());

    ui->stalledIssuesTree->setModel(mProxyModel);
    mViewHoverManager.setView(ui->stalledIssuesTree);

    mDelegate = new StalledIssueDelegate(mProxyModel, ui->stalledIssuesTree);
    ui->stalledIssuesTree->setItemDelegate(mDelegate);
    mLoadingScene.setView(ui->stalledIssuesTree);

    connect(MegaSyncApp->getStalledIssuesModel(), &StalledIssuesModel::uiBlocked,
            this,  &StalledIssuesDialog::onUiBlocked);
    connect(MegaSyncApp->getStalledIssuesModel(), &StalledIssuesModel::uiUnblocked,
            this,  &StalledIssuesDialog::onUiUnblocked);

    connect(MegaSyncApp->getStalledIssuesModel(), &StalledIssuesModel::stalledIssuesReceived,
            this,  &StalledIssuesDialog::onStalledIssuesLoaded);

    connect(MegaSyncApp->getStalledIssuesModel(), &StalledIssuesModel::globalSyncStateChanged,
            this,  &StalledIssuesDialog::onGlobalSyncStateChanged);

    connect(MegaSyncApp->getStalledIssuesModel(), &StalledIssuesModel::stalledIssuesCountChanged,
            this,  &StalledIssuesDialog::onStalledIssuesModelCountChanged);

    //Init all categories
    auto tabs = ui->header->findChildren<StalledIssueTab*>();
    foreach(auto tab, tabs)
    {
        connect(tab, &StalledIssueTab::tabToggled, this, &StalledIssuesDialog::toggleTab);
    }

    ui->allIssuesTab->setItsOn(true);

    on_updateButton_clicked();
}

StalledIssuesDialog::~StalledIssuesDialog()
{
    delete ui;
}

bool StalledIssuesDialog::eventFilter(QObject* obj, QEvent* event)
{
    return QDialog::eventFilter(obj, event);
}

void StalledIssuesDialog::on_doneButton_clicked()
{
    close();
}

void StalledIssuesDialog::on_updateButton_clicked()
{
    mProxyModel->updateStalledIssues();
}

void StalledIssuesDialog::onStalledIssuesModelCountChanged()
{
    if(auto proxyModel = dynamic_cast<StalledIssuesProxyModel*>(ui->stalledIssuesTree->model()))
    {
        auto isEmpty = proxyModel->rowCount(QModelIndex()) == 0;
        ui->TreeViewContainer->setCurrentWidget(isEmpty ? ui->EmptyViewContainerPage : ui->TreeViewContainerPage);
    }
}

void StalledIssuesDialog::toggleTab(StalledIssueFilterCriterion filterCriterion)
{
  if(auto proxyModel = dynamic_cast<StalledIssuesProxyModel*>(ui->stalledIssuesTree->model()))
  {
      proxyModel->filter(filterCriterion);
  }
}

void StalledIssuesDialog::onUiBlocked()
{
    if(!mLoadingScene.isLoadingViewSet())
    {
        setDisabled(true);
        mLoadingScene.setLoadingScene(true);

        //Show the view
        ui->TreeViewContainer->setCurrentWidget(ui->TreeViewContainerPage);
    }
}

void StalledIssuesDialog::onUiUnblocked()
{
    if(mLoadingScene.isLoadingViewSet())
    {
        setDisabled(false);
        mLoadingScene.setLoadingScene(false);
        onStalledIssuesModelCountChanged();
    }
}

void StalledIssuesDialog::onStalledIssuesLoaded()
{
    mDelegate->resetCache();
    mProxyModel->updateFilter();
    onUiUnblocked();
}

void StalledIssuesDialog::onGlobalSyncStateChanged(bool state)
{
    if(state)
    {
        onUiUnblocked();
        mProxyModel->updateFilter();
    }
    else
    {
        mProxyModel->invalidate();
    }
}