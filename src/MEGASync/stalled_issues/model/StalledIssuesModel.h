#ifndef STALLEDISSUESMODEL_H
#define STALLEDISSUESMODEL_H

#include "QTMegaRequestListener.h"
#include "QTMegaGlobalListener.h"
#include "StalledIssue.h"

#include <QObject>
#include <QMutex>
#include <QAbstractItemModel>
#include <QTimer>

class ThreadPool;

class StalledIssuesReceiver : public QObject, public mega::MegaRequestListener
{
    Q_OBJECT
public:
    struct StalledIssuesReceived
    {
        StalledIssuesVariantList mNewStalledIssues;
        StalledIssuesVariantList mUpdateStalledIssues;
        StalledIssuesVariantList mDeleteStalledIssues;

        bool isEmpty(){return mNewStalledIssues.isEmpty() && mUpdateStalledIssues.isEmpty() && mDeleteStalledIssues.isEmpty();}
        void clear()
        {
            mNewStalledIssues.clear();
            mUpdateStalledIssues.clear();
            mDeleteStalledIssues.clear();
        }
    };

    explicit StalledIssuesReceiver(QObject *parent = nullptr);
    ~StalledIssuesReceiver(){}

    bool setCurrentStalledIssuesToCompare(const StalledIssuesVariantList &currentIssues);

signals:
    void stalledIssuesReady(StalledIssuesReceiver::StalledIssuesReceived);

protected:
    void onRequestFinish(::mega::MegaApi*, ::mega::MegaRequest *request, ::mega::MegaError*);

private:
    QMutex mCacheMutex;
    StalledIssuesReceived mCacheStalledIssues;
    StalledIssuesVariantList mCurrentStalledIssues;

    void processStalledIssues();
};

Q_DECLARE_METATYPE(StalledIssuesReceiver::StalledIssuesReceived);

class StalledIssuesModel : public QAbstractItemModel, public mega::MegaGlobalListener
{
    Q_OBJECT

public:
    static const int ADAPTATIVE_HEIGHT_ROLE;

    explicit StalledIssuesModel(QObject* parent = 0);
    ~StalledIssuesModel();

    virtual Qt::DropActions supportedDropActions() const override;
    bool hasChildren(const QModelIndex& parent) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    int getCountByFilterCriterion(StalledIssueFilterCriterion criterion);

    void finishStalledIssues(const QModelIndexList& indexes);
    void updateStalledIssues();
    void updateStalledIssuesWhenReady();
    bool hasStalledIssues() const;

    void lockModelMutex(bool lock);

    void blockUi();
    void unBlockUi();

    //Methods to modify data
    bool solveLocalConflictedNameByRemove(const QString& name, int conflictIndex, const QModelIndex& index);
    bool solveLocalConflictedNameByRename(const QString& name, const QString& renameTo, int conflictIndex, const QModelIndex& index);

    bool solveCloudConflictedNameByRemove(const QString& name, int conflictIndex, const QModelIndex& index);
    bool solveCloudConflictedNameByRename(const QString& name, const QString &renameTo, int conflictIndex, const QModelIndex& index);

    void solveIssue(bool isCloud, const QModelIndex& index);

signals:
    void stalledIssuesReceived(bool state);
    void globalSyncStateChanged(bool state);
    void stalledIssuesCountChanged();

    void uiBlocked();
    void uiUnblocked();

protected slots:
    void onGlobalSyncStateChanged(mega::MegaApi *api) override;

private slots:
    void onProcessStalledIssues(StalledIssuesReceiver::StalledIssuesReceived issuesReceived);

private:
    void removeRows(QModelIndexList &indexesToRemove);
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    void updateStalledIssuedByOrder();
    void reset();
    QModelIndex getSolveIssueIndex(const QModelIndex& index);

    QThread* mStalledIssuesThread;
    StalledIssuesReceiver* mStalledIssuedReceiver;
    mega::QTMegaRequestListener* mRequestListener;
    mega::QTMegaGlobalListener* mGlobalListener;
    mega::MegaApi* mMegaApi;
    bool mHasStalledIssues;
    bool mUpdateWhenGlobalStateChanges;
    bool mIssuesRequested;

    mutable QMutex mModelMutex;

    mutable StalledIssuesVariantList mStalledIssues;
    mutable QHash<StalledIssueVariant*, int> mStalledIssuesByOrder;

    QHash<int, int> mCountByFilterCriterion;

    std::unique_ptr<ThreadPool> mThreadPool;
};

#endif // STALLEDISSUESMODEL_H