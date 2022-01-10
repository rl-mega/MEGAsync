#include "TransferItem2.h"
#include "megaapi.h"
#include "Utilities.h"

using namespace mega;

const TransferData::TransferStates TransferData::STATE_MASK = TransferData::TransferStates (
//        TransferData::TransferState::TRANSFER_NONE |
        TransferData::TransferState::TRANSFER_QUEUED |
        TransferData::TransferState::TRANSFER_ACTIVE |
        TransferData::TransferState::TRANSFER_PAUSED |
        TransferData::TransferState::TRANSFER_RETRYING |
        TransferData::TransferState::TRANSFER_COMPLETING |
        TransferData::TransferState::TRANSFER_COMPLETED |
        TransferData::TransferState::TRANSFER_CANCELLED |
        TransferData::TransferState::TRANSFER_FAILED);

const TransferData::TransferTypes TransferData::TYPE_MASK = TransferData::TransferTypes (
        TransferData::TransferType::TRANSFER_LTCPDOWNLOAD |
        TransferData::TransferType::TRANSFER_UPLOAD |
        TransferData::TransferType::TRANSFER_DOWNLOAD);

void TransferItem2::updateValuesTransferFinished(int64_t finishTime,
                                                 int errorCode, long long errorValue,
                                                 unsigned long long meanSpeed,
                                                 TransferData::TransferState state,
                                                 unsigned long long transferedBytes,
                                                 MegaHandle parentHandle,
                                                 MegaHandle nodeHandle,
                                                 MegaNode* publicNode)
{
    d->mFinishedTime = finishTime;
    d->mErrorCode = errorCode;
    d->mState = state;
    d->mErrorValue = errorValue;
    d->mRemainingTime = 0LL;
    d->mSpeed = 0;
    d->mMeanSpeed = meanSpeed;
    d->mTransferredBytes = transferedBytes;
    d->mParentHandle = parentHandle;
    d->mNodeHandle = nodeHandle;
    d->mPublicNode = publicNode;
}

void TransferItem2::updateValuesTransferUpdated(int64_t remainingTime,
                                                int errorCode, long long errorValue,
                                                unsigned long long meanSpeed,
                                                unsigned long long speed,
                                                unsigned long long priority,
                                                TransferData::TransferState state,
                                                unsigned long long transferedBytes,
                                                MegaNode* publicNode)
{
    d->mState = state;
    d->mRemainingTime = remainingTime;
    d->mErrorCode = errorCode;
    d->mErrorValue = errorValue;
    d->mSpeed = speed;
    d->mMeanSpeed = meanSpeed;
    d->mTransferredBytes = transferedBytes;
    d->mPriority = priority;
    d->mPublicNode = publicNode;
}