#include "FileFolderAttributes.h"

#include <MegaApplication.h>
#include <UserAttributesRequests/FullName.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#endif


#ifdef WIN32
#define stat _stat
#endif

FileFolderAttributes::FileFolderAttributes(QObject* parent)
    : mCancelled(false),
      mSize(-1),
      QObject(parent)
{
}

FileFolderAttributes::~FileFolderAttributes()
{
}

void FileFolderAttributes::requestSize(QObject* caller,std::function<void (qint64)> func)
{
    if(auto context = requestReady(AttributeTypes::Size, caller))
    {
        connect(this, &FileFolderAttributes::sizeReady, context, [this, func](qint64 size){
            if(func)
            {
                func(size);
                if(size >= 0)
                {
                    requestFinish(AttributeTypes::Size);
                }
            }
        });
    }
}

void FileFolderAttributes::requestModifiedTime(QObject* caller, std::function<void (const QDateTime &)> func)
{
    if(auto context = requestReady(AttributeTypes::ModifiedTime, caller))
    {
        connect(this, &FileFolderAttributes::modifiedTimeReady, context, [this, func](const QDateTime& time){
            if(func)
            {
                func(time);
                if(time.isValid())
                {
                    requestFinish(AttributeTypes::ModifiedTime);
                }
            }
        });
    }
}

void FileFolderAttributes::requestCreatedTime(QObject* caller, std::function<void (const QDateTime &)> func)
{
    if(auto context = requestReady(AttributeTypes::CreatedTime, caller))
    {
        connect(this, &FileFolderAttributes::createdTimeReady, context, [this, func](const QDateTime& time){
            if(func)
            {
                func(time);
                if(time.isValid())
                {
                    requestFinish(AttributeTypes::CreatedTime);
                }
            }
        });
    }
}

void FileFolderAttributes::cancel()
{
    mCancelled = true;
}

bool FileFolderAttributes::attributeNeedsUpdate(int type)
{
    auto currentTime = QDateTime::currentMSecsSinceEpoch();
    if(!mRequestTimestamps.contains(type)
            || ((currentTime - mRequestTimestamps.value(type)) > 2000))
    {
        mRequestTimestamps.insert(type, currentTime);
        return true;
    }

    return false;
}

void FileFolderAttributes::requestFinish(int type)
{
    auto contextObject = mRequests.take(type);
    if(contextObject)
    {
        contextObject->deleteLater();
    }
}

QObject *FileFolderAttributes::requestReady(int type, QObject *caller)
{
    if(!mRequests.contains(type))
    {
        QObject* contextObject = new QObject(caller);
        mRequests.insert(type, contextObject);

        return contextObject;
    }

    return nullptr;
}


//LOCAL
LocalFileFolderAttributes::LocalFileFolderAttributes(const QString &path, QObject *parent)
    : mPath(path),
      FileFolderAttributes(parent)
{
    connect(&mModifiedTimeWatcher, &QFutureWatcher<QDateTime>::finished,
            this, &LocalFileFolderAttributes::onModifiedTimeCalculated);

    connect(&mFolderSizeFuture, &QFutureWatcher<qint64>::finished,
            this, &LocalFileFolderAttributes::onSizeCalculated);

    QDirIterator filesIt(mPath, QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDirIterator::Subdirectories);
    mIsEmpty = !filesIt.hasNext();
}

void LocalFileFolderAttributes::requestSize(QObject* caller,std::function<void(qint64)> func)
{
    FileFolderAttributes::requestSize(caller,func);

    if(attributeNeedsUpdate(AttributeTypes::Size))
    {
        QFileInfo fileInfo(mPath);

        if(fileInfo.isFile())
        {
            mSize = fileInfo.size();
        }
        else
        {
            if(mSize < 0)
            {
                auto future = QtConcurrent::run([this]() -> qint64{
                    return calculateSize();
                });
                mFolderSizeFuture.setFuture(future);
            }

        }
    }

    //We always send the size, even if the request is async...just to show on GUI a "loading size..." or the most recent size while the new is received
    emit sizeReady(mSize);
}

void LocalFileFolderAttributes::requestModifiedTime(QObject* caller,std::function<void(const QDateTime&)> func)
{
    FileFolderAttributes::requestModifiedTime(caller,func);

    if(attributeNeedsUpdate(AttributeTypes::ModifiedTime))
    {
        QFileInfo fileInfo(mPath);

        if(fileInfo.exists())
        {
            if(fileInfo.isFile())
            {
                mModifiedTime = fileInfo.lastModified();
            }
            //Is local folder
            else
            {
                if(mIsEmpty)
                {
                    requestCreatedTime(caller,func);
                }
                else
                {
                    auto future = QtConcurrent::run([this]() -> QDateTime{
                        return calculateModifiedTime();
                    });
                    mModifiedTimeWatcher.setFuture(future);
                }
            }
        }
    }

     //We always send the time, even if the request is async...just to show on GUI a "loading time..." or the most recent time while the new is received
    emit modifiedTimeReady(mModifiedTime);
}

void LocalFileFolderAttributes::onModifiedTimeCalculated()
{
    mModifiedTime = mModifiedTimeWatcher.result();
    if(mModifiedTime.isValid())
    {
        emit modifiedTimeReady(mModifiedTime);
    }
}

void LocalFileFolderAttributes::onSizeCalculated()
{
    mSize = mFolderSizeFuture.result();
    emit sizeReady(mSize);
}

void LocalFileFolderAttributes::requestCreatedTime(QObject* caller,std::function<void(const QDateTime&)> func)
{
    FileFolderAttributes::requestCreatedTime(caller,func);

    if(attributeNeedsUpdate(AttributeTypes::CreatedTime))
    {
        struct stat result;

#ifdef Q_OS_WINDOWS
        const QString sourcePath = mPath;
        QVarLengthArray<wchar_t, MAX_PATH + 1> file(sourcePath.length() + 2);
        sourcePath.toWCharArray(file.data());
        file[sourcePath.length()] = wchar_t{};
        file[sourcePath.length() + 1] = wchar_t{};
        if(_wstat(file.constData(), &result)==0)
#else
        const char* rawFile = mPath.toUtf8().constData();
        if(stat(rawFile, &result)==0)
#endif
        {
            auto mod_time = result.st_ctime;
            mCreatedTime = QDateTime::fromSecsSinceEpoch(mod_time);

            QFileInfo fileInfo(mPath);
            if(!fileInfo.isFile())
            {
                if(mIsEmpty)
                {
                    mModifiedTime = mCreatedTime;
                    emit modifiedTimeReady(mModifiedTime);
                }
            }
        }
    }

    emit createdTimeReady(mCreatedTime);
}

QDateTime LocalFileFolderAttributes::calculateModifiedTime()
{
    QDateTime newDate;
    QDirIterator filesIt(mPath, QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDirIterator::Subdirectories);

    while (filesIt.hasNext())
    {
        if(mCancelled)
        {
            break;
        }

        filesIt.next();
        if(filesIt.fileInfo().lastModified() > newDate)
        {
            newDate = filesIt.fileInfo().lastModified();
        }
    }

    return newDate;
}

qint64 LocalFileFolderAttributes::calculateSize()
{
    qint64 newSize(0);

    QDirIterator filesIt(mPath, QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDirIterator::Subdirectories);

    while (filesIt.hasNext())
    {
        filesIt.next();
        newSize += filesIt.fileInfo().size();
    }

    return newSize;
}

//REMOTE
RemoteFileFolderAttributes::RemoteFileFolderAttributes(const QString &filePath, QObject *parent)
    : FileFolderAttributes(parent),
      mFilePath(filePath),
      mHandle(mega::INVALID_HANDLE),
      mVersionCount(0)
{
    mListener = new mega::QTMegaRequestListener(MegaSyncApp->getMegaApi(), this);
}

RemoteFileFolderAttributes::RemoteFileFolderAttributes(mega::MegaHandle handle, QObject* parent)
    : FileFolderAttributes(parent),
      mHandle(handle),
      mVersionCount(0)
{
    mListener = new mega::QTMegaRequestListener(MegaSyncApp->getMegaApi(), this);
}

RemoteFileFolderAttributes::~RemoteFileFolderAttributes()
{
    delete mListener;
}

void RemoteFileFolderAttributes::requestSize(QObject* caller,std::function<void(qint64)> func)
{
    FileFolderAttributes::requestSize(caller,func);

    if(attributeNeedsUpdate(AttributeTypes::Size))
    {
        std::unique_ptr<mega::MegaNode> node = getNode();
        if(node)
        {
            if(node->isFile())
            {
                mSize = std::max(static_cast<long long>(node->getSize()), static_cast<long long>(0));
            }
            else
            {
                MegaSyncApp->getMegaApi()->getFolderInfo(node.get(),mListener);
            }
        }
    }

    //We always send the size, even if the request is async...just to show on GUI a "loading size..." or the most recent size while the new is received
    emit sizeReady(mSize);
}

void RemoteFileFolderAttributes::requestModifiedTime(QObject* caller,std::function<void(const QDateTime&)> func)
{
    FileFolderAttributes::requestModifiedTime(caller,func);

    if(attributeNeedsUpdate(AttributeTypes::ModifiedTime))
    {
        std::unique_ptr<mega::MegaNode> node = getNode();
        if(node)
        {
            int64_t newTime = node->isFile() ? node->getModificationTime()
                                     : node->getCreationTime();

            mModifiedTime = QDateTime::fromSecsSinceEpoch(newTime);
        }
    }

    emit modifiedTimeReady(mModifiedTime);
}

void RemoteFileFolderAttributes::requestCreatedTime(QObject* caller,std::function<void(const QDateTime&)> func)
{
    FileFolderAttributes::requestCreatedTime(caller, func);

    if(!mCreatedTime.isValid() && attributeNeedsUpdate(AttributeTypes::CreatedTime))
    {
        std::unique_ptr<mega::MegaNode> node = getNode(Version::First);
        if(node)
        {
            mCreatedTime = QDateTime::fromSecsSinceEpoch(node->getCreationTime());
        }
    }

    emit createdTimeReady(mCreatedTime);
}

void RemoteFileFolderAttributes::requestUser(QObject *caller, std::function<void (QString, bool)> func)
{
    if(attributeNeedsUpdate(RemoteAttributeTypes::User))
    {
        std::unique_ptr<mega::MegaNode> node = getNode();
        if(node)
        {
            auto user = node->getOwner();

            if(user != mega::INVALID_HANDLE && user != mOwner)
            {
                if(auto context = requestReady(RemoteAttributeTypes::User, caller))
                {
                    mOwner = user;
                    MegaSyncApp->getMegaApi()->getUserEmail(user,new MegaListenerFuncExecuter(true, [this ,func, context](mega::MegaApi*,  mega::MegaRequest* request, mega::MegaError* e) {
                        if (e->getErrorCode() == mega::MegaError::API_OK)
                        {
                            auto emailFromRequest = request->getEmail();
                            if (emailFromRequest)
                            {
                                mUserEmail = QString::fromUtf8(emailFromRequest);
                                mUserFullName = UserAttributes::FullName::requestFullName(emailFromRequest);

                                connect(mUserFullName.get(), &UserAttributes::FullName::fullNameReady, context, [this, func]{
                                    func(mUserFullName->getFullName(), true);
                                    requestFinish(RemoteAttributeTypes::User);
                                });

                                if(mUserFullName->isAttributeReady())
                                {
                                    requestFinish(RemoteAttributeTypes::User);
                                }
                            }
                        }
                    }));
                }
            }
        }
    }

    if(mUserFullName)
    {
        //We always send the name, even if the request is async...just to show on GUI a "loading user..." or the most recent user while the new is received
        func(mUserFullName->getFullName(), true);
    }
    else
    {
        func(QString(), true);
    }
}

void RemoteFileFolderAttributes::requestUser(QObject *caller, mega::MegaHandle currentUser, std::function<void (QString, bool)> func)
{
    std::unique_ptr<mega::MegaNode> node = getNode();
    if(node)
    {
        auto user = node->getOwner();

        if(user != mega::INVALID_HANDLE && user != currentUser)
        {
            requestUser(caller, func);
        }
        else
        {
            mOwner = mega::INVALID_HANDLE;
            mUserEmail.clear();
            mUserFullName = nullptr;
            func(QString(), false);
        }
    }
}

void RemoteFileFolderAttributes::onRequestFinish(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e)
{
    mega::MegaRequestListener::onRequestFinish(api, request, e);

    if (request->getType() == mega::MegaRequest::TYPE_FOLDER_INFO
            && e->getErrorCode() == mega::MegaError::API_OK)
    {
        auto folderInfo = request->getMegaFolderInfo();
        mSize = std::max(folderInfo->getCurrentSize(), 0LL);
        emit sizeReady(mSize);
    }
}

void RemoteFileFolderAttributes::requestVersions(QObject*, std::function<void (int)> func)
{
    if(attributeNeedsUpdate(RemoteFileFolderAttributes::Versions))
    {
        std::unique_ptr<mega::MegaNode> node = getNode();
        if(node)
        {
            mVersionCount = MegaSyncApp->getMegaApi()->getVersions(node.get())->size();
        }
    }

    func(mVersionCount);
}

std::unique_ptr<mega::MegaNode> RemoteFileFolderAttributes::getNode(Version type) const
{
    std::unique_ptr<mega::MegaNode> node;
    std::unique_ptr<mega::MegaNode> lastVersionNode;

    if(mFilePath.isEmpty())
    {
        node.reset(MegaSyncApp->getMegaApi()->getNodeByHandle(mHandle));
    }
    else
    {
        node.reset(MegaSyncApp->getMegaApi()->getNodeByPath(mFilePath.toUtf8().constData()));
    }

    auto nodeVersions = MegaSyncApp->getMegaApi()->getVersions(node.get());
    if(nodeVersions->size() > 1)
    {
        lastVersionNode.reset(nodeVersions->get(type == Version::Last ? 0 : (nodeVersions->size() - 1)));
    }
    else
    {
        lastVersionNode = std::move(node);
    }

    return lastVersionNode;
}