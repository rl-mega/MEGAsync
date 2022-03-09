#ifndef TRANSFERSSORTFILTERPROXYMODEL_H
#define TRANSFERSSORTFILTERPROXYMODEL_H

#include "TransferItem.h"
#include "TransfersSortFilterProxyBaseModel.h"

#include <QSortFilterProxyModel>
#include <QReadWriteLock>
#include <QFutureWatcher>

class TransferBaseDelegateWidget;
class TransfersModel;

class TransfersManagerSortFilterProxyModel : public TransfersSortFilterProxyBaseModel
{
        Q_OBJECT

public:
        TransfersManagerSortFilterProxyModel(QObject *parent = nullptr);
        ~TransfersManagerSortFilterProxyModel();

        bool moveRows(const QModelIndex& proxyParent, int proxyRow, int count,
                      const QModelIndex& destinationParent, int destinationChild) override;
        void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
        void invalidate();
        void setSourceModel(QAbstractItemModel *sourceModel) override;
        void setFilterFixedString(const QString &pattern);
        void textSearchTypeChanged();
        void setFilters(const TransferData::TransferTypes transferTypes,
                        const TransferData::TransferStates transferStates,
                        const Utilities::FileTypes fileTypes);
        void updateFilters();
        void resetAllFilters();
        int  getNumberOfItems(TransferData::TransferType transferType);
        void resetNumberOfItems();

        TransferBaseDelegateWidget* createTransferManagerItem(QWidget *parent) override;

        int getPausedTransfers() const;
        bool isAnyPaused() const;

signals:
        void modelAboutToBeChanged();
        void modelChanged();
        void searchNumbersChanged();
        void modelAboutToBeSorted();
        void modelSorted();
        void transferPauseResume(bool);

protected slots:
        void onCancelClearTransfer();
        void onPauseResumeTransfer();
        void onRetryTransfer();
        void onOpenTransfer();

protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

protected:
        TransferData::TransferStates mTransferStates;
        TransferData::TransferTypes mTransferTypes;
        Utilities::FileTypes mFileTypes;
        TransferData::TransferStates mNextTransferStates;
        TransferData::TransferTypes mNextTransferTypes;
        Utilities::FileTypes mNextFileTypes;
        SortCriterion mSortCriterion;
        mutable QSet<int> mDlNumber;
        mutable QSet<int> mUlNumber;

private slots:
        void onRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
        void onModelSortedFiltered();

private:
        ThreadPool* mThreadPool;
        QFutureWatcher<void> mFilterWatcher;
        QString mFilterText;
};

Q_DECLARE_METATYPE(QAbstractItemModel::LayoutChangeHint)

#endif // TRANSFERSSORTFILTERPROXYMODEL_H