#include "TransfersManagerSortFilterProxyModel.h"
#include "TransfersModel.h"
#include "MegaApplication.h"
#include "TransferManagerDelegateWidget.h"
#include <megaapi.h>
#include <QElapsedTimer>


TransfersManagerSortFilterProxyModel::TransfersManagerSortFilterProxyModel(QObject* parent)
    : TransfersSortFilterProxyBaseModel(parent),
      mTransferStates (TransferData::STATE_MASK),
      mTransferTypes (TransferData::TYPE_MASK),
      mFileTypes (~Utilities::FileTypes()),
      mNextTransferStates (mTransferStates),
      mNextTransferTypes (mTransferTypes),
      mNextFileTypes (mFileTypes),
      mSortCriterion (SortCriterion::PRIORITY),
      mThreadPool (ThreadPoolSingleton::getInstance())
{
    qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>("QAbstractItemModel::LayoutChangeHint");

    connect(&mFilterWatcher, &QFutureWatcher<void>::finished,
            this, &TransfersManagerSortFilterProxyModel::onModelSortedFiltered);

    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

TransfersManagerSortFilterProxyModel::~TransfersManagerSortFilterProxyModel()
{
}

void TransfersManagerSortFilterProxyModel::sort(int sortCriterion, Qt::SortOrder order)
{
    if(!dynamicSortFilter())
    {
        setDynamicSortFilter(true);
    }

    auto sourceM = qobject_cast<TransfersModel*>(sourceModel());
    if(sourceM)
    {
        sourceM->pauseModelProcessing(false);
    }

    emit modelAboutToBeChanged();
    if (sortCriterion != static_cast<int>(mSortCriterion))
    {
        mSortCriterion = static_cast<SortCriterion>(sortCriterion);
    }

    QFuture<void> filtered = QtConcurrent::run([this, order, sortCriterion](){
        auto sourceM = qobject_cast<TransfersModel*>(sourceModel());
        sourceM->lockModelMutex(true);
        QSortFilterProxyModel::sort(sortCriterion, order);
        QSortFilterProxyModel::invalidate();
        sourceM->lockModelMutex(false);
    });
    mFilterWatcher.setFuture(filtered);
}

void TransfersManagerSortFilterProxyModel::invalidate()
{
    resetNumberOfItems();
    emit modelAboutToBeChanged();

    QSortFilterProxyModel::invalidate();

    emit modelChanged();
    emit searchNumbersChanged();
}

void TransfersManagerSortFilterProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &TransfersManagerSortFilterProxyModel::onRowsAboutToBeRemoved, Qt::DirectConnection);

    QSortFilterProxyModel::setSourceModel(sourceModel);
}

void TransfersManagerSortFilterProxyModel::setFilterFixedString(const QString& pattern)
{
    mFilterText = pattern;
    updateFilters();

    resetNumberOfItems();
    emit modelAboutToBeChanged();

    QFuture<void> filtered = QtConcurrent::run([this](){
        auto sourceM = qobject_cast<TransfersModel*>(sourceModel());
        sourceM->lockModelMutex(true);
        invalidateFilter();
        sourceM->lockModelMutex(false);
    });
    mFilterWatcher.setFuture(filtered);
}

void TransfersManagerSortFilterProxyModel::textSearchTypeChanged()
{
    updateFilters();

    emit modelAboutToBeChanged();

    QFuture<void> filtered = QtConcurrent::run([this](){
        auto sourceM = qobject_cast<TransfersModel*>(sourceModel());
        sourceM->lockModelMutex(true);
        invalidateFilter();
        sourceM->lockModelMutex(false);
    });
    mFilterWatcher.setFuture(filtered);
}

void TransfersManagerSortFilterProxyModel::onModelSortedFiltered()
{
    auto sourceM = qobject_cast<TransfersModel*>(sourceModel());
    if(sourceM)
    {
        sourceM->pauseModelProcessing(false);
    }

    emit modelChanged();
    emit searchNumbersChanged();
}

void TransfersManagerSortFilterProxyModel::setFilters(const TransferData::TransferTypes transferTypes,
                                               const TransferData::TransferStates transferStates,
                                               const Utilities::FileTypes fileTypes)
{
    mNextTransferTypes = transferTypes ? transferTypes : TransferData::TYPE_MASK;
    mNextTransferStates = transferStates ? transferStates : TransferData::STATE_MASK;
    if (fileTypes)
    {
        mNextFileTypes = fileTypes;
    }
    else
    {
        mNextFileTypes = ~Utilities::FileTypes({});
    }
}

void TransfersManagerSortFilterProxyModel::resetAllFilters()
{
    resetNumberOfItems();
    setFilters({}, {}, {});
}

int  TransfersManagerSortFilterProxyModel::getNumberOfItems(TransferData::TransferType transferType)
{
    int nb(0);

    if(transferType == TransferData::TransferType::TRANSFER_UPLOAD)
    {
        nb = mUlNumber.size();
    }
    else if(transferType == TransferData::TransferType::TRANSFER_DOWNLOAD)
    {
        nb = mDlNumber.size();
    }

    return nb;
}

void TransfersManagerSortFilterProxyModel::resetNumberOfItems()
{
    mDlNumber.clear();
    mUlNumber.clear();
}

TransferBaseDelegateWidget *TransfersManagerSortFilterProxyModel::createTransferManagerItem(QWidget* parent)
{
    auto item = new TransferManagerDelegateWidget(parent);

    //All are UniqueConnection to avoid reconnecting if thw item already exists in cache and it is not a new item
    connect(item, &TransferManagerDelegateWidget::cancelTransfer,
            this, &TransfersManagerSortFilterProxyModel::onCancelClearTransfer);
    connect(item, &TransferManagerDelegateWidget::pauseResumeTransfer,
            this, &TransfersManagerSortFilterProxyModel::onPauseResumeTransfer);
    connect(item, &TransferManagerDelegateWidget::retryTransfer,
             this, &TransfersManagerSortFilterProxyModel::onRetryTransfer);
    connect(item, &TransferManagerDelegateWidget::openTransfer,
             this, &TransfersManagerSortFilterProxyModel::onOpenTransfer);

    return item;
}

void TransfersManagerSortFilterProxyModel::updateFilters()
{
    auto sourceM = qobject_cast<TransfersModel*>(sourceModel());
    if(sourceM)
    {
        sourceM->pauseModelProcessing(false);
    }

    mTransferStates = mNextTransferStates;
    mTransferTypes = mNextTransferTypes;
    mFileTypes = mNextFileTypes;
}

bool TransfersManagerSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    const auto d (qvariant_cast<TransferItem>(index.data()).getTransferData());

    if(d->mTag >= 0)
    {
        auto accept = (d->mState & mTransferStates)
                 && (d->mType & mTransferTypes)
                 && (toInt(d->mFileType) & mFileTypes);

        if (accept)
        {
            if(!mFilterText.isEmpty())
            {
                accept = d->mFilename.contains(mFilterText,Qt::CaseInsensitive);

                if (accept)
                {
                    if (d->mType & TransferData::TRANSFER_UPLOAD && !mUlNumber.contains(d->mTag))
                    {
                        mUlNumber.insert(d->mTag);
                    }
                    else if (d->mType & TransferData::TRANSFER_DOWNLOAD && !mDlNumber.contains(d->mTag))
                    {
                        mDlNumber.insert(d->mTag);
                    }
                }
            }

        }

        return accept;
    }

    return false;
}

//It is called in the main thread, from a QtConcurrent thread
void TransfersManagerSortFilterProxyModel::onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
   if(mFilterText.isEmpty())
   {
       return;
   }

   bool RowsRemoved(false);

   for(int row = first; row <= last; ++row)
   {
       QModelIndex index = sourceModel()->index(row, 0, parent);

       const auto d (qvariant_cast<TransferItem>(index.data()).getTransferData());

       if(d->mTag >= 0)
       {
           if (d->mType & TransferData::TRANSFER_UPLOAD)
           {
               mUlNumber.remove(d->mTag);
           }
           else
           {
               mDlNumber.remove(d->mTag);
           }

           RowsRemoved = true;
       }
   }

   if(RowsRemoved)
   {
       searchNumbersChanged();
   }
}

bool TransfersManagerSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto leftItem (qvariant_cast<TransferItem>(left.data()).getTransferData());
    const auto rightItem (qvariant_cast<TransferItem>(right.data()).getTransferData());

    switch (mSortCriterion)
    {
        case SortCriterion::PRIORITY:
        {
            if(leftItem->mState != rightItem->mState)
            {
                return leftItem->mState > rightItem->mState;
            }
            else
            {

                return leftItem->mPriority > rightItem->mPriority;
            }
        }
        case SortCriterion::TOTAL_SIZE:
        {
            return leftItem->mTotalSize < rightItem->mTotalSize;
        }
        case SortCriterion::NAME:
        {
            return QString::compare(leftItem->mFilename, rightItem->mFilename, Qt::CaseInsensitive) < 0;
        }
    }

    return false;
}

int TransfersManagerSortFilterProxyModel::getPausedTransfers() const
{
    auto paused(0);

    for(int row = 0; row < rowCount(); ++row)
    {
        QModelIndex proxyIndex = index(row, 0);

        const auto d (qvariant_cast<TransferItem>(proxyIndex.data()).getTransferData());

        if(d->mState & TransferData::TransferState::TRANSFER_PAUSED)
        {
            paused++;
        }
    }

    return paused;
}

bool TransfersManagerSortFilterProxyModel::isAnyPaused() const
{
    auto paused(false);
    auto last = rowCount();

    for(int row = 0; row < last; ++row)
    {
        QModelIndex proxyIndex = index(row, 0);

        const auto d (qvariant_cast<TransferItem>(proxyIndex.data()).getTransferData());

        if(d->mState & TransferData::TransferState::TRANSFER_PAUSED)
        {
            paused = true;
            break;
        }
    }

    return paused;
}

bool TransfersManagerSortFilterProxyModel::moveRows(const QModelIndex &proxyParent, int proxyRow, int count,
              const QModelIndex &destinationParent, int destinationChild)
{
    bool moveOk(true);
    int row(proxyRow);

    for(int row = 0; row < rowCount(); ++row)
    {
        auto sourceIndex(mapToSource(index(row, 0)));
    }

    while (moveOk && row < (proxyRow+count))
    {
        auto sourceIndex(mapToSource(index(proxyRow, 0, proxyParent)));
        int destRow;
        if (destinationChild == rowCount())
        {
            destRow = sourceModel()->rowCount();
        }
        else
        {
            destRow = mapToSource(index(destinationChild, 0, destinationParent)).row();
        }
        moveOk = sourceModel()->moveRows(sourceIndex.parent(), sourceIndex.row(), 1,
                                         sourceIndex.parent(), destRow);
        row++;
    }
    return moveOk;
}

void TransfersManagerSortFilterProxyModel::onCancelClearTransfer()
{
    auto delegateWidget = dynamic_cast<TransferManagerDelegateWidget*>(sender());
    auto sourModel = dynamic_cast<TransfersModel*>(sourceModel());

    if(delegateWidget && sourModel)
    {
        QModelIndexList indexes;
        auto index = delegateWidget->getCurrentIndex();
        index = mapToSource(index);
        indexes.append(index);
        sourModel->cancelClearTransfers(indexes, false);
    }
}

void TransfersManagerSortFilterProxyModel::onPauseResumeTransfer()
{
    auto delegateWidget = dynamic_cast<TransferManagerDelegateWidget*>(sender());
    auto sourModel = dynamic_cast<TransfersModel*>(sourceModel());

    if(delegateWidget && sourModel)
    {
        auto tag = delegateWidget->getData()->mTag;
        auto pause = delegateWidget->getData()->mState != TransferData::TransferState::TRANSFER_PAUSED;
        sourModel->pauseResumeTransferByTag(tag,
                                            pause);
        emit transferPauseResume(pause);
    }
}

void TransfersManagerSortFilterProxyModel::onRetryTransfer()
{
    auto delegateWidget = dynamic_cast<TransferManagerDelegateWidget*>(sender());
    auto sourModel = dynamic_cast<TransfersModel*>(sourceModel());

    if(delegateWidget && sourModel)
    {
        auto data = delegateWidget->getData();

        if(data->mFailedTransfer)
        {
            QModelIndexList indexToRemove;
            auto index = delegateWidget->getCurrentIndex();
            index = mapToSource(index);
            indexToRemove.append(index);
            sourModel->cancelClearTransfers(indexToRemove,false);

            MegaSyncApp->getMegaApi()->retryTransfer(data->mFailedTransfer.get());
            data->removeFailedTransfer();
        }
    }
}

void TransfersManagerSortFilterProxyModel::onOpenTransfer()
{
    auto delegateWidget = dynamic_cast<TransferManagerDelegateWidget*>(sender());
    auto sourModel = dynamic_cast<TransfersModel*>(sourceModel());

    if(delegateWidget && sourModel)
    {
        auto data = delegateWidget->getData();
        if(data)
        {
            auto tag = data->mTag;
            //If the transfer is an upload (already on the local drive)
            //Or if it is an download but already finished
            if(data->mType & TransferData::TRANSFER_UPLOAD
                    || data->mState & TransferData::FINISHED_STATES_MASK)
            {
                sourModel->openFolderByTag(tag);
            }
        }
    }
}