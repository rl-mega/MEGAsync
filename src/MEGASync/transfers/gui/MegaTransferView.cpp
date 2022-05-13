#include "MegaTransferView.h"
#include "MegaApplication.h"
#include "platform/Platform.h"
#include "control/Utilities.h"
#include "gui/QMegaMessageBox.h"
#include "TransfersWidget.h"

#include <QScrollBar>
#include <QtConcurrent/QtConcurrent>
#include <QIcon>

using namespace mega;

const QColor MegaTransferView::UPLOAD_DRAG_COLOR = QColor("#2BA6DE");
const QColor MegaTransferView::DOWNLOAD_DRAG_COLOR = QColor("#31B500");

MegaTransferView::MegaTransferView(QWidget* parent) :
    QTreeView(parent),
    mDisableLink(false),
    mDisableMenus(false),
    mKeyNavigation(false),
    mParentTransferWidget(nullptr),
    mContextMenu(nullptr),
    mPauseAction(nullptr),
    mResumeAction(nullptr),
    mMoveToTopAction(nullptr),
    mMoveUpAction(nullptr),
    mMoveDownAction(nullptr),
    mMoveToBottomAction(nullptr),
    mCancelAction(nullptr),
    mGetLinkAction(nullptr),
    mOpenInMEGAAction(nullptr),
    mOpenItemAction(nullptr),
    mShowInFolderAction(nullptr),
    mClearAction(nullptr)
{
    setMouseTracking(true);
    setAutoScroll(false);

    verticalScrollBar()->installEventFilter(this);
}

void MegaTransferView::setup()
{
    setContextMenuPolicy(Qt::CustomContextMenu);
}

void MegaTransferView::setup(TransfersWidget* tw)
{
    mParentTransferWidget = tw;
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &MegaTransferView::customContextMenuRequested,
            this, &MegaTransferView::onCustomContextMenu);

    createContextMenu();
}

void MegaTransferView::disableGetLink(bool disable)
{
    mDisableLink = disable;
    mGetLinkAction->setEnabled(!disable);
    mOpenInMEGAAction->setEnabled(!disable);
}

QModelIndexList MegaTransferView::getTransfers(bool onlyVisible, TransferData::TransferStates state)
{
    QModelIndexList indexes;
    auto proxy(qobject_cast<QSortFilterProxyModel*>(model()));
    auto sourceModel(qobject_cast<TransfersModel*>(proxy->sourceModel()));

    auto rowCount = onlyVisible ? proxy->rowCount(QModelIndex()) : sourceModel->rowCount(QModelIndex());

    for (auto row (0); row < rowCount; ++row)
    {
        auto index (model()->index(row, 0, QModelIndex()));
        auto d (qvariant_cast<TransferItem>(index.data()).getTransferData());
        if(state == TransferData::TRANSFER_NONE || (d && d->mState & state))
        {
            if (proxy)
            {
                index = proxy->mapToSource(index);
            }
            indexes.push_back(index);
        }
    }

    return indexes;
}

bool MegaTransferView::isSingleTransfer(bool onlyVisible, TransferData::TransferStates state)
{
    QModelIndexList indexes;
    auto proxy(qobject_cast<QSortFilterProxyModel*>(model()));
    auto sourceModel(qobject_cast<TransfersModel*>(proxy->sourceModel()));

    auto rowCount = onlyVisible ? proxy->rowCount(QModelIndex()) : sourceModel->rowCount(QModelIndex());

    for (auto row (0); row < rowCount; ++row)
    {
        auto index (model()->index(row, 0, QModelIndex()));
        auto d (qvariant_cast<TransferItem>(index.data()).getTransferData());
        if(state == TransferData::TRANSFER_NONE || (d && d->mState & state))
        {
            indexes.push_back(index);

            if(indexes.size() > 1)
            {
                return false;
            }
        }
    }

    return true;
}

QModelIndexList MegaTransferView::getSelectedTransfers()
{
    auto proxy(qobject_cast<QSortFilterProxyModel*>(model()));
    auto selection = selectionModel()->selection();
    QModelIndexList indexes;

    if (selection.size() > 0)
    {
        if (proxy)
        {
            selection = proxy->mapSelectionToSource(selection);
        }
        indexes = selection.indexes();
    }

    return indexes;
}

bool MegaTransferView::isSingleSelectedTransfers()
{
    auto selection = selectionModel()->selection();
    return !(selection.size() > 1);
}

void MegaTransferView::onPauseResumeVisibleRows(bool pauseState)
{
    QModelIndexList indexes = getTransfers(true);

    auto proxy (qobject_cast<QSortFilterProxyModel*>(model()));
    auto sourceModel(qobject_cast<TransfersModel*>(proxy->sourceModel()));
    sourceModel->pauseTransfers(indexes, pauseState);

    //Use to repaint and update the transfers state
    update();
}

void MegaTransferView::onPauseResumeSelection(bool pauseState)
{
    QModelIndexList indexes = getSelectedTransfers();
    auto proxy (qobject_cast<QSortFilterProxyModel*>(model()));
    auto sourceModel(qobject_cast<TransfersModel*>(proxy->sourceModel()));

    sourceModel->pauseTransfers(indexes, pauseState);

    //Use to repaint and update the transfers state
    update();
}

void MegaTransferView::onCancelVisibleTransfers()
{
    bool singleTransfer = isSingleTransfer(true);

    QPointer<MegaTransferView> dialog = QPointer<MegaTransferView>(this);

    if (QMegaMessageBox::warning(this, QString::fromUtf8("MEGAsync"),
                             tr("Are you sure you want to cancel this(these) transfer(s)?", "", !singleTransfer),
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            != QMessageBox::Yes
            || !dialog)
    {
        return;
    }

    auto indexes = getTransfers(true);

    auto proxy (qobject_cast<QSortFilterProxyModel*>(model()));
    auto sourceModel(qobject_cast<TransfersModel*>(proxy->sourceModel()));
    sourceModel->cancelTransfers(indexes, this);
}

void MegaTransferView::onCancelClearSelectedTransfers()
{
    bool singleTransfer = isSingleSelectedTransfers();

    QPointer<MegaTransferView> dialog = QPointer<MegaTransferView>(this);

    if (QMegaMessageBox::warning(this, QString::fromUtf8("MEGAsync"),
                             tr("Are you sure you want to cancel or clear this(these) transfer(s)?", "", !singleTransfer),
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            != QMessageBox::Yes
            || !dialog)
    {
        return;
    }

    QModelIndexList indexes = getSelectedTransfers();

    auto proxy (qobject_cast<QSortFilterProxyModel*>(model()));
    auto sourceModel(qobject_cast<TransfersModel*>(proxy->sourceModel()));
    sourceModel->cancelTransfers(indexes, this);
}

void MegaTransferView::onCancelAllTransfers()
{
    bool singleTransfer = isSingleTransfer(true);

    QPointer<MegaTransferView> dialog = QPointer<MegaTransferView>(this);

    if (QMegaMessageBox::warning(this, QString::fromUtf8("MEGAsync"),
                             tr("Are you sure you want to cancel this(these) transfer(s)?", "", !singleTransfer),
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            != QMessageBox::Yes
            || !dialog)
    {
        return;
    }

    cancelAndClearAllTransfers(true, false);
}

void MegaTransferView::onClearAllTransfers()
{
    bool singleTransfer = isSingleTransfer(true);

    QPointer<MegaTransferView> dialog = QPointer<MegaTransferView>(this);

    if (QMegaMessageBox::warning(this, QString::fromUtf8("MEGAsync"),
                             tr("Are you sure you want to clear this(these) transfer(s)?", "", !singleTransfer),
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            != QMessageBox::Yes
            || !dialog)
    {
        return;
    }

    cancelAndClearAllTransfers(false, true);
}

void MegaTransferView::onCancelAndClearAllTransfers()
{
    bool singleTransfer = isSingleTransfer(true);

    QPointer<MegaTransferView> dialog = QPointer<MegaTransferView>(this);

    if (QMegaMessageBox::warning(this, QString::fromUtf8("MEGAsync"),
                             tr("Are you sure you want to cancel and clear this(these) transfer(s)?", "", !singleTransfer),
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            != QMessageBox::Yes
            || !dialog)
    {
        return;
    }

    cancelAndClearAllTransfers(true, true);
}

void MegaTransferView::onCancelAndClearVisibleTransfers()
{
    bool singleTransfer = isSingleTransfer(true);

    QPointer<MegaTransferView> dialog = QPointer<MegaTransferView>(this);

    if (QMegaMessageBox::warning(this, QString::fromUtf8("MEGAsync"),
                             tr("Are you sure you want to cancel and clear this(these) transfer(s)?", "", !singleTransfer),
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            != QMessageBox::Yes
            || !dialog)
    {
        return;
    }

    auto proxy (qobject_cast<QSortFilterProxyModel*>(model()));
    auto sourceModel(qobject_cast<TransfersModel*>(proxy->sourceModel()));

    auto indexes = getTransfers(true, TransferData::FINISHED_STATES_MASK);
    sourceModel->clearTransfers(indexes);

    //Cancel transfers
    auto cancelIndexes = getTransfers(true);
    sourceModel->cancelTransfers(cancelIndexes, this);
}

void MegaTransferView::cancelAndClearAllTransfers(bool cancel, bool clear)
{
    if(cancel | clear)
    {
        auto proxy(qobject_cast<QSortFilterProxyModel*>(model()));
        auto sourceModel(qobject_cast<TransfersModel*>(proxy->sourceModel()));
        if(cancel && clear)
        {
            sourceModel->setResetMode();
        }

        sourceModel->pauseModelProcessing(true);

        if(clear)
        {
            sourceModel->clearTransfers(QModelIndexList());
        }
        if(cancel)
        {
            sourceModel->cancelTransfers(QModelIndexList(), this);
        }

        sourceModel->pauseModelProcessing(false);
    }
}

int MegaTransferView::getVerticalScrollBarWidth() const
{
    return verticalScrollBar()->width();
}

void MegaTransferView::onClearCompletedVisibleTransfers()
{
    auto proxy (qobject_cast<QSortFilterProxyModel*>(model()));
    auto sourceModel(qobject_cast<TransfersModel*>(proxy->sourceModel()));

    bool singleTransfer = isSingleTransfer(true, TransferData::FINISHED_STATES_MASK);

    QPointer<MegaTransferView> dialog = QPointer<MegaTransferView>(this);

    if (QMegaMessageBox::warning(this, QString::fromUtf8("MEGAsync"),
                                 tr("Are you sure you want to clear this(these) transfer(s)?", "", !singleTransfer),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            != QMessageBox::Yes
            || !dialog)
    {
        return;
    }

    auto indexes = getTransfers(true, TransferData::FINISHED_STATES_MASK);

    sourceModel->clearTransfers(indexes);
}

void MegaTransferView::onRetryVisibleTransfers()
{
    bool singleTransfer = isSingleTransfer(true, TransferData::TRANSFER_FAILED);

    QPointer<MegaTransferView> dialog = QPointer<MegaTransferView>(this);

    if (QMegaMessageBox::warning(this, QString::fromUtf8("MEGAsync"),
                                 tr("Are you sure you want to retry this(these) transfer(s)?", "", !singleTransfer),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            != QMessageBox::Yes
            || !dialog)
    {
        return;
    }

    QModelIndexList indexes = getTransfers(true, TransferData::TRANSFER_FAILED);

    auto proxy (qobject_cast<QSortFilterProxyModel*>(model()));
    auto sourceModel(qobject_cast<TransfersModel*>(proxy->sourceModel()));
    sourceModel->retryTransfers(indexes);
}

void MegaTransferView::onCancelClearSelection(bool isClear)
{
    QModelIndexList indexes = getSelectedTransfers();
    auto proxy (qobject_cast<QSortFilterProxyModel*>(model()));
    auto sourceModel(qobject_cast<TransfersModel*>(proxy->sourceModel()));
    isClear ? sourceModel->clearTransfers(indexes) : sourceModel->cancelTransfers(indexes, this);
}

void MegaTransferView::disableContextMenus(bool option)
{
    mDisableMenus = option;
}

void MegaTransferView::createContextMenu()
{
    if (!mContextMenu)
    {
        mContextMenu = new QMenu(this);
        mContextMenu->setWindowFlags(mContextMenu->windowFlags() | Qt::NoDropShadowWindowHint);
    }
    else
    {        
        for (auto action : mContextMenu->actions())
        {
            mContextMenu->removeAction(action);
        }
    }

    if (mPauseAction)
    {
        mPauseAction->deleteLater();
        mPauseAction = nullptr;
    }

    mPauseAction = new QAction(QIcon(QLatin1String(":/images/pause_ico.png")),
                                     tr("Pause Transfer(s)"), this);
    connect(mPauseAction, &QAction::triggered,
            this, &MegaTransferView::pauseSelectedClicked);

    if (mResumeAction)
    {
        mResumeAction->deleteLater();
        mResumeAction = nullptr;
    }

    mResumeAction = new QAction(QIcon(QLatin1String(":/images/resume_ico.png")),
                                      tr("Resume Transfer(s)"), this);
    connect(mResumeAction, &QAction::triggered,
            this, &MegaTransferView::resumeSelectedClicked);

    if (mMoveToTopAction)
    {
        mMoveToTopAction->deleteLater();
        mMoveToTopAction = nullptr;
    }

    mMoveToTopAction = new QAction(QIcon(QLatin1String(":/images/move_top_ico.png")),
                                         tr("Move to top"), this);
    connect(mMoveToTopAction, &QAction::triggered, this, &MegaTransferView::moveToTopClicked);

    if (mMoveUpAction)
    {
        mMoveUpAction->deleteLater();
        mMoveUpAction = nullptr;
    }

    mMoveUpAction = new QAction(QIcon(QLatin1String(":/images/move_up_ico.png")),
                                      tr("Move up"), this);
    connect(mMoveUpAction, &QAction::triggered, this, &MegaTransferView::moveUpClicked);

    if (mMoveDownAction)
    {
        mMoveDownAction->deleteLater();
        mMoveDownAction = nullptr;
    }

    mMoveDownAction = new QAction(QIcon(QLatin1String(":/images/move_down_ico.png")),
                                        tr("Move down"), this);
    connect(mMoveDownAction, &QAction::triggered, this, &MegaTransferView::moveDownClicked);

    if (mMoveToBottomAction)
    {
        mMoveToBottomAction->deleteLater();
        mMoveToBottomAction = nullptr;
    }

    mMoveToBottomAction = new QAction(QIcon(QLatin1String(":/images/move_bottom_ico.png")),
                                            tr("Move to bottom"), this);
    connect(mMoveToBottomAction, &QAction::triggered, this, &MegaTransferView::moveToBottomClicked);

    if (mCancelAction)
    {
        mCancelAction->deleteLater();
        mCancelAction = nullptr;
    }

    mCancelAction = new QAction(QIcon(QLatin1String(":/images/cancel_transfer_ico.png")),
                                      tr("Cancel Transfer(s)"), this);
    connect(mCancelAction, &QAction::triggered,
            this, &MegaTransferView::cancelSelectedClicked);

    if (mGetLinkAction)
    {
        mGetLinkAction->deleteLater();
        mGetLinkAction = nullptr;
    }

    mGetLinkAction = new QAction(QIcon(QLatin1String(":/images/get_link_ico.png")),
                                 tr("Get link"), this);
    connect(mGetLinkAction, &QAction::triggered, this, &MegaTransferView::getLinkClicked);

    if (mOpenInMEGAAction)
    {
        mOpenInMEGAAction->deleteLater();
        mOpenInMEGAAction = nullptr;
    }

    mOpenInMEGAAction = new QAction(QIcon(QLatin1String(":/images/ico_open_MEGA.png")),
                                 tr("Open in MEGA"), this);
    connect(mOpenInMEGAAction, &QAction::triggered, this, &MegaTransferView::openInMEGAClicked);

    if (mOpenItemAction)
    {
        mOpenItemAction->deleteLater();
        mOpenItemAction = nullptr;
    }

    mOpenItemAction = new QAction(QIcon(QLatin1String(":/images/open_file_ico.png")),
                                  tr("Open"), this);
    connect(mOpenItemAction, &QAction::triggered, this, &MegaTransferView::openItemClicked);

    if (mShowInFolderAction)
    {
        mShowInFolderAction->deleteLater();
        mShowInFolderAction = nullptr;
    }

    mShowInFolderAction = new QAction(QIcon(QLatin1String(":/images/show_in_folder_ico.png")),
                                      tr("Show in folder"), this);
    connect(mShowInFolderAction, &QAction::triggered, this, &MegaTransferView::showInFolderClicked);

    if (mClearAction)
    {
        mClearAction->deleteLater();
        mClearAction = nullptr;
    }

    mClearAction = new QAction(QIcon(QLatin1String(":/images/ico_clear.png")),
                               tr("Clear"), this);
    connect(mClearAction, &QAction::triggered,
            this, &MegaTransferView::clearSelectedClicked);

    mContextMenu->addAction(mPauseAction);
    mContextMenu->addAction(mResumeAction);

    mContextMenu->addAction(mOpenItemAction);
    mContextMenu->addAction(mShowInFolderAction);
    mContextMenu->addAction(mOpenInMEGAAction);

    mContextMenu->addSeparator();

    mContextMenu->addAction(mGetLinkAction);


    mContextMenu->addAction(mMoveToTopAction);
    mContextMenu->addAction(mMoveUpAction);
    mContextMenu->addAction(mMoveDownAction);
    mContextMenu->addAction(mMoveToBottomAction);

    mContextMenu->addSeparator();

    mContextMenu->addAction(mCancelAction);
    mContextMenu->addAction(mClearAction);

    // Set default action to have it painted red
    mContextMenu->setDefaultAction(mCancelAction);
}

void MegaTransferView::updateContextMenu(bool enablePause, bool enableResume, bool enableMove,
                                         bool enableClear, bool enableCancel, bool isTopIndex, bool isBottomIndex)
{
    mPauseAction->setVisible(enablePause);
    mResumeAction->setVisible(enableResume);
    mCancelAction->setVisible(enableCancel);
    mClearAction->setVisible(enableClear);

    auto indexes = selectedIndexes();

    bool onlyOneSelected ((indexes.size() == 1));
    bool onlyOneAndClear(enableClear && onlyOneSelected);

    bool showLink (false);
    bool showOpen (false);
    bool showShowInFolder (false);

    if (onlyOneAndClear)
    {
        auto d (qvariant_cast<TransferItem>(indexes.first().data()).getTransferData());

        auto state (d->mState);
        auto type ((d->mType & TransferData::TRANSFER_UPLOAD) ?
                       TransferData::TRANSFER_UPLOAD
                     : TransferData::TRANSFER_DOWNLOAD);

        if (state == TransferData::TRANSFER_COMPLETED)
        {
            showLink = true;
            showOpen = true;
            showShowInFolder = true;
        }
        else if (type == TransferData::TRANSFER_UPLOAD)
        {
            showOpen = true;
            showShowInFolder = true;
        }
        else if (type == TransferData::TRANSFER_DOWNLOAD)
        {
            showLink = true;
        }
    }

    mGetLinkAction->setVisible(showLink);
    mOpenInMEGAAction->setVisible(showLink);
    mOpenItemAction->setVisible(showOpen);
    mShowInFolderAction->setVisible(showShowInFolder);

    if (onlyOneSelected)
    {
        mPauseAction->setText(tr("Pause Transfer"));
        mResumeAction->setText(tr("Resume Transfer"));
        mCancelAction->setText(tr("Cancel Transfer"));
    }
    else
    {
        mPauseAction->setText(tr("Pause Transfers"));
        mResumeAction->setText(tr("Resume Transfers"));
        mCancelAction->setText(tr("Cancel Transfers"));
    }

    if(enableMove)
    {
        mMoveToTopAction->setVisible(!isTopIndex);
        mMoveUpAction->setVisible(!isTopIndex);
        mMoveToBottomAction->setVisible(!isBottomIndex);
        mMoveDownAction->setVisible(!isBottomIndex);
    }
    else
    {
        mMoveToTopAction->setVisible(false);
        mMoveUpAction->setVisible(false);
        mMoveToBottomAction->setVisible(false);
        mMoveDownAction->setVisible(false);
    }
}

void MegaTransferView::mouseReleaseEvent(QMouseEvent* event)
{
    QTreeView::mouseReleaseEvent(event);
}

void MegaTransferView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
    {
        return;
    }

    QTreeView::mouseMoveEvent(event);
}

void MegaTransferView::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        createContextMenu();
    }
    QTreeView::changeEvent(event);
}

void MegaTransferView::dropEvent(QDropEvent* event)
{
    QAbstractItemView::dropEvent(event);
    clearSelection();
}

void MegaTransferView::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Delete)
    {
        onCancelClearSelectedTransfers();
    }
    else if(event->key() == Qt::Key_Down || event->key() == Qt::Key_Up)
    {
        mKeyNavigation = true;
    }

    QTreeView::keyPressEvent(event);

    if(mKeyNavigation)
    {
        mKeyNavigation = false;
    }
}

void MegaTransferView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    if(mKeyNavigation)
    {
        auto selectedIndexes = selected.indexes();
        auto deselectedIndexes = deselected.indexes();

        if(!selectedIndexes.isEmpty())
        {
            scrollTo(selectedIndexes.last(), QAbstractItemView::PositionAtCenter);
        }
        else if(!deselectedIndexes.isEmpty())
        {
            scrollTo(deselectedIndexes.last(), QAbstractItemView::PositionAtCenter);
        }
    }

    QTreeView::selectionChanged(selected, deselected);
}

bool MegaTransferView::eventFilter(QObject *object, QEvent *event)
{
    if(object == verticalScrollBar())
    {
        if(event->type() == QEvent::Show)
        {
            emit verticalScrollBarVisibilityChanged(true);
        }
        else if(event->type() == QEvent::Hide)
        {
            emit verticalScrollBarVisibilityChanged(false);
        }
    }

    return QTreeView::eventFilter(object, event);
}

void MegaTransferView::focusOutEvent(QFocusEvent *event)
{
    clearSelection();
    QTreeView::focusOutEvent(event);
}

void MegaTransferView::onCustomContextMenu(const QPoint& point)
{
    bool enablePause = false;
    bool enableResume = false;
    bool enableCancel = false;
    bool enableClear = false;
    bool enableMove = false;
    bool isTopIndex(false);
    bool isBottomIndex(false);

    QModelIndexList indexes = selectedIndexes();

    for (auto index : qAsConst(indexes))
    {
        if(index.row() == 0)
        {
            isTopIndex = true;
        }
        else if(index.row() == (model()->rowCount() -1))
        {
            isBottomIndex = true;
        }

        auto d (qvariant_cast<TransferItem>(index.data()).getTransferData());
        switch (d->mState)
        {
            case TransferData::TRANSFER_ACTIVE:
            case TransferData::TRANSFER_QUEUED:
            case TransferData::TRANSFER_RETRYING:
            {
                enablePause = true;
                enableMove = true;
                enableCancel = !(d->mType & TransferData::TRANSFER_SYNC);
                break;
            }
            case TransferData::TRANSFER_PAUSED:
            {
                enableResume = true;
                enableMove = true;
                enableCancel = !(d->mType & TransferData::TRANSFER_SYNC);
                break;
            }
            case TransferData::TRANSFER_CANCELLED:
            case TransferData::TRANSFER_FAILED:
            case TransferData::TRANSFER_COMPLETED:
            {
                enableClear = true;
                break;
            }
            default:
                break;
        }
    }
    updateContextMenu(enablePause, enableResume, enableMove, enableClear, enableCancel, isTopIndex, isBottomIndex);

    //Check if any of the action is visible, if not, do not show the qmenu
    bool atLeastOneVisible(false);
    foreach(auto action, mContextMenu->actions())
    {
        if(!action->isSeparator() && action->isVisible())
        {
            atLeastOneVisible = true;
        }
    }

    if(atLeastOneVisible)
    {
        mContextMenu->exec(mapToGlobal(point));
    }
}

void MegaTransferView::moveToTopClicked()
{
    auto indexes = selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        // Sort to keep items in the same order
        std::sort(indexes.begin(), indexes.end(),[](QModelIndex check1, QModelIndex check2){
           return check1.row() < check2.row();
        });

        auto firstToMove = indexes.first();

        if(firstToMove.row() == 0)
        {
            return;
        }

        auto rowTarget = -1;

        for (int item = 0; item < indexes.size(); ++item)
        {
            auto index = indexes.at(item);

            if(item == 1)
            {
                rowTarget = indexes.at(item-1).row();
            }

            model()->moveRows(QModelIndex(), index.row(), 1, QModelIndex(), rowTarget);
        }
    }

    clearSelection();
}

void MegaTransferView::moveUpClicked()
{
    auto indexes = selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        // Sort to keep items in the same order
        std::sort(indexes.begin(), indexes.end(),[](QModelIndex check1, QModelIndex check2){
           return check1.row() < check2.row();
        });

        auto firstToMove = indexes.first();

        if(firstToMove.row() == 0)
        {
            return;
        }

        auto rowTarget = indexes.first().row() - 1;

        for (int item = 0; item < indexes.size(); ++item)
        {
            auto index = indexes.at(item);
            if(item != 0)
            {
                auto previousIndex = indexes.at(item-1);

                if(index.row() - previousIndex.row() != 1)
                {
                    rowTarget = index.row() - 1;
                }
            }

            model()->moveRows(QModelIndex(), index.row(), 1, QModelIndex(), rowTarget);
        }
    }
    clearSelection();
}

void MegaTransferView::moveDownClicked()
{
    auto indexes = selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        // Sort to keep items in the same order
        std::sort(indexes.begin(), indexes.end(),[](QModelIndex check1, QModelIndex check2){
           return check1.row() > check2.row();
        });

        auto firstToMove = indexes.first();

        if(firstToMove.row() == model()->rowCount())
        {
            return;
        }

        auto rowTarget = indexes.first().row() + 1;

        for (int item = 0; item < indexes.size(); ++item)
        {
            auto index = indexes.at(item);
            if(item != 0)
            {
                auto previousIndex = indexes.at(item-1);

                if(previousIndex.row() - index.row() != 1)
                {
                    rowTarget = index.row() + 1;
                }
            }

            model()->moveRows(QModelIndex(), rowTarget, 1, QModelIndex(), index.row());
        }
    }

    clearSelection();
}

void MegaTransferView::moveToBottomClicked()
{
    auto indexes = selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        // Sort to keep items in the same order
        std::sort(indexes.begin(), indexes.end(),[](QModelIndex check1, QModelIndex check2){
           return check1.row() < check2.row();
        });

        auto firstToMove = indexes.last();

        if(firstToMove.row() == model()->rowCount())
        {
            return;
        }

        auto rowTarget = model()->rowCount();

        for (int item = 0; item < indexes.size(); ++item)
        {
            auto index = indexes.at(item);

            if(item == 1)
            {
                rowTarget = indexes.at(item-1).row();
            }

            model()->moveRows(QModelIndex(), index.row(), 1, QModelIndex(), rowTarget);
        }
    }

    clearSelection();
}

void MegaTransferView::getLinkClicked()
{
    if (mDisableLink)
    {
        return;
    }

    QList<int> rows;
    auto proxy(qobject_cast<QSortFilterProxyModel*>(model()));

    const auto indexes (proxy ?
                            proxy->mapSelectionToSource(selectionModel()->selection()).indexes()
                          : selectionModel()->selection().indexes());
    for (auto index : indexes)
    {
        rows.push_back(index.row());
    }

    if (!rows.isEmpty())
    {
        mParentTransferWidget->getModel()->getLinks(rows);
    }

    clearSelection();
}

void MegaTransferView::openInMEGAClicked()
{
    if (mDisableLink)
    {
        return;
    }

    QList<int> rows;
    auto proxy(qobject_cast<QSortFilterProxyModel*>(model()));

    const auto indexes (proxy ?
                            proxy->mapSelectionToSource(selectionModel()->selection()).indexes()
                          : selectionModel()->selection().indexes());
    for (auto index : indexes)
    {
        rows.push_back(index.row());
    }

    if (!rows.isEmpty())
    {
        mParentTransferWidget->getModel()->openInMEGA(rows);
    }

    clearSelection();
}

void MegaTransferView::openItemClicked()
{
    const QModelIndexList selection (selectedIndexes());
    for (auto index : selection)
    {
        if (index.isValid())
        {
            const auto transferItem (
                        qvariant_cast<TransferItem>(index.data(Qt::DisplayRole)));
            auto d (transferItem.getTransferData());
            auto path = d->path();
            if (!path.isEmpty())
            {
                QtConcurrent::run(QDesktopServices::openUrl, QUrl::fromLocalFile(path));
            }
        }
    }
    clearSelection();
}

void MegaTransferView::showInFolderClicked()
{
    const QModelIndexList selection (selectedIndexes());
    for (auto index : selection)
    {
        if (index.isValid())
        {
            mParentTransferWidget->getModel()->openFolderByIndex(index);
        }
    }
    clearSelection();
}

void MegaTransferView::showInMegaClicked()
{
    const QModelIndexList selection (selectedIndexes());
    for (auto index : selection)
    {
        if (index.isValid())
        {
            const auto transferItem (
                        qvariant_cast<TransferItem>(index.data(Qt::DisplayRole)));
            auto d (transferItem.getTransferData());
            if (d->mParentHandle != mega::INVALID_HANDLE)
            {
                qobject_cast<MegaApplication*>(qApp)->shellViewOnMega(d->mParentHandle, false);
            }
        }
    }
    clearSelection();
}

void MegaTransferView::cancelSelectedClicked()
{
    onCancelClearSelection(false);
}

void MegaTransferView::clearSelectedClicked()
{
    onCancelClearSelection(true);
}

void MegaTransferView::pauseSelectedClicked()
{
    onPauseResumeSelection(true);
}

void MegaTransferView::resumeSelectedClicked()
{
    onPauseResumeSelection(false);
}