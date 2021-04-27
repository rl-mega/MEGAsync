#include "TransfersSortFilterProxyModel.h"
#include "QTransfersModel2.h"
#include <megaapi.h>


TransfersSortFilterProxyModel::TransfersSortFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent),
      mTransferTypes (TransferData::TYPE_MASK),
      mTransferStates (TransferData::STATE_MASK),
      mFileTypes (~TransferData::FileTypes()),
      mSortCriterion (SORT_BY::PRIORITY)
{
}

void TransfersSortFilterProxyModel::setTransferTypes(const TransferData::TransferTypes transferTypes)
{
    mTransferTypes = transferTypes ? transferTypes : TransferData::TYPE_MASK;
}

void TransfersSortFilterProxyModel::addTransferTypes(const TransferData::TransferTypes transferTypes)
{
    mTransferTypes |= transferTypes;
}

void TransfersSortFilterProxyModel::setTransferStates(const TransferData::TransferStates transferStates)
{
    mTransferStates = transferStates ? transferStates : TransferData::STATE_MASK;
}

void TransfersSortFilterProxyModel::addTransferStates(const TransferData::TransferStates transferStates)
{
    mTransferStates |= transferStates;
}

void TransfersSortFilterProxyModel::setFileTypes(const TransferData::FileTypes fileTypes)
{
    if (fileTypes)
    {
        mFileTypes = fileTypes;
    }
    else
    {
        mFileTypes = ~TransferData::FileTypes({});
    }
}

void TransfersSortFilterProxyModel::addFileTypes(const TransferData::FileTypes fileTypes)
{
    mFileTypes |= fileTypes;
}

void TransfersSortFilterProxyModel::resetAllFilters()
{
    mTransferStates = TransferData::STATE_MASK;
    mTransferTypes = TransferData::TYPE_MASK;
    mFileTypes = ~TransferData::FileTypes();
}

void TransfersSortFilterProxyModel::setSortBy(SORT_BY sortCriterion)
{
    mSortCriterion = sortCriterion;
}

bool TransfersSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    const auto d (qvariant_cast<TransferItem2>(index.data()).getTransferData());

    return     (d->mState & mTransferStates)
            && (d->mType & mTransferTypes)
            && (d->mFileType & mFileTypes)
            && d->mFilename.contains(filterRegExp());
}

bool TransfersSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    bool lessThan (false);
    const auto leftItem (qvariant_cast<TransferItem2>(left.data()).getTransferData());
    const auto rightItem (qvariant_cast<TransferItem2>(right.data()).getTransferData());

    switch (mSortCriterion)
    {
        case SORT_BY::PRIORITY:
        {
            lessThan = leftItem->mPriority < rightItem->mPriority;
            break;
        }
        case SORT_BY::TOTAL_SIZE:
        {
            lessThan = leftItem->mTotalSize < rightItem->mTotalSize;
            break;
        }
        case SORT_BY::NAME:
        {
            lessThan = leftItem->mFilename < rightItem->mFilename;
            break;
        }
        default:
            break;
    }

    return lessThan;
}

bool TransfersSortFilterProxyModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
              const QModelIndex &destinationParent, int destinationChild)
{
    bool moveOk(true);
    int row(sourceRow);
    while (moveOk && row < (sourceRow+count))
    {
        auto sourceIndex(mapToSource(index(sourceRow, 0, sourceParent)));
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