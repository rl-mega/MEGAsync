#include "DiscountPolicy.h"

#include "AppStatsEvents.h"
#include "StatsEventHandler.h"
#include "Utilities.h"

#include <cmath>

constexpr double DOUBLE_COMPARISON_EPSILON = 1e-5;

DiscountPolicy::DiscountPolicy(QObject* parent):
    QObject(parent),
    mPreferences(Preferences::instance())
{
    if (load())
    {
        checkAndDeactivateExpiredCampaign();
    }
}

void DiscountPolicy::activateCampaign(std::shared_ptr<mega::MegaDiscountCodeInfo> discountInfo)
{
    // We have several cases to take into account here:
    // - no stored code + no running campaign: start new campaign
    // - running campaign + same code: do nothing
    // - no running campaign + stored code + same new code: restore campaign
    // - stored code + expired stored campaign + new code: stop old campaign and start new
    // - running campaign + new code: stop old campaign and start new
    // - stored code + expired stored campaign + new code + expired new campaign: stop old campaign

    if (!discountInfo || !isDiscountValid(discountInfo))
    {
        return;
    }

    bool deactivateCampaignFirst = false;
    bool activateCampaign = false;

    // Check persisted campaign
    if ((mIsLoadingPersistedDataNeeded && load()) ||
        (!mIsLoadingPersistedDataNeeded && !mIsCampaignActive && !mDiscountCode.isEmpty()))
    {
        // We want to deactivate an expired persisted campaign
        deactivateCampaignFirst = isCampaignExpiredUtc(mCampaignExpiryDateUtc);
        // And activate it if it's not expired
        activateCampaign = !deactivateCampaignFirst;
    }

    const auto newCode = QString::fromUtf8(discountInfo->getCode());
    const auto isNewCampaign = newCode != mDiscountCode;

    // If it's a new campaign we want to activate it
    activateCampaign |= isNewCampaign;

    const auto newExpiryDateUtc = QDateTime::fromSecsSinceEpoch(discountInfo->getExpiry(), Qt::UTC);
    const auto isNewCampaignExpired =
        isCampaignExpiredUtc(newExpiryDateUtc); // Very low probability

    // We want to deactivate an active, but different campaign, or if the new campaign is already
    // expired when we process the info.
    deactivateCampaignFirst |= mIsCampaignActive && (isNewCampaign || isNewCampaignExpired);

    // Deactivate if needed
    if (deactivateCampaignFirst)
    {
        deactivateCampaign();
    }

    // Evaluate here because deactivating can change mIsCampaignActive value.
    // We want to activate only if the campaign is not already running, or if the new campaign is
    // already expired when we process the info.
    activateCampaign &= !(mIsCampaignActive || isNewCampaignExpired);

    // Activate if needed
    if (activateCampaign && !mIsPlanPending)
    {
        mCampaignExpiryDateUtc = newExpiryDateUtc;
        mDiscountInfo = discountInfo;
        mDiscountCode = newCode;
        mIsNewCampaign = isNewCampaign;

        // Get pricing
        if (!mUpsellController)
        {
            mUpsellController = new UpsellController(false, this);
            connect(mUpsellController,
                    &UpsellController::dataReady,
                    this,
                    &DiscountPolicy::onPlansReady);
        }
        mIsPlanPending = true;
        mUpsellController->requestPricingData();
    }
}

void DiscountPolicy::deactivateCampaign()
{
    // We want to send an event if the campaign was runing or we had a persisted campaign pending
    // re-activation
    auto wasCampaignActive = mIsCampaignActive || (!mIsLoadingPersistedDataNeeded &&
                                                   !mIsCampaignActive && !mDiscountCode.isEmpty());

    if (wasCampaignActive || mIsPlanPending)
    {
        mIsCampaignActive = false;
        mIsNewCampaign = false;
        mDiscountInfo.reset();

        mDiscountCode.clear();
        mCampaignExpiryDateUtc = QDateTime();
        mLastTimeShownUtc = QDateTime();
        persist();

        mIsPlanPending = false;
        if (mUpsellController)
        {
            mUpsellController->deleteLater();
            mUpsellController = nullptr;
        }
    }

    if (wasCampaignActive)
    {
        MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(
            AppStatsEvents::EventType::TARGETED_DISCOUNT_CAMPAIGN_STOPPED);
        emit campaignDeactivated();
    }
}

bool DiscountPolicy::isCampaignActive() const
{
    return mIsCampaignActive && mDiscountInfo;
}

void DiscountPolicy::recordShown()
{
    mLastTimeShownUtc = QDateTime::currentDateTimeUtc();
    persist();
    MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(
        AppStatsEvents::EventType::TARGETED_DISCOUNT_DIALOG_SHOWN);
}

void DiscountPolicy::recordDismissed()
{
    mLastTimeShownUtc = QDateTime::currentDateTimeUtc();
    persist();
    MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(
        AppStatsEvents::EventType::TARGETED_DISCOUNT_DIALOG_DISMISSED);
}

void DiscountPolicy::recordAccepted()
{
    mLastTimeShownUtc = QDateTime::currentDateTimeUtc();
    persist();
    MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(
        AppStatsEvents::EventType::TARGETED_DISCOUNT_DIALOG_DEAL_GRABBED);
}

QDateTime DiscountPolicy::getDialogLastShownDateUtc() const
{
    return mLastTimeShownUtc;
}

QDateTime DiscountPolicy::getExpiryDateUtc() const
{
    return mCampaignExpiryDateUtc;
}

QString DiscountPolicy::getCurrencySymbol() const
{
    auto plans = mUpsellController ? mUpsellController->getPlans() : nullptr;
    return plans ? plans->getCurrencySymbol() : QString();
}

QString DiscountPolicy::getCurrencyName() const
{
    const char* code = mDiscountInfo ? mDiscountInfo->getLocalCurrencyCode() : nullptr;
    return (code && *code) ? QString::fromUtf8(code) : QStringLiteral("EUR");
}

QString DiscountPolicy::getPlanName(bool shortName) const
{
    return mDiscountInfo ?
               Utilities::getReadablePlanFromId(mDiscountInfo->getAccountLevel(), shortName) :
               QString();
}

QString DiscountPolicy::getStorage() const
{
    if (mDiscountedPlan)
    {
        auto gbStorage = mDiscountedPlan->monthlyData().gBStorage();
        if (gbStorage < 0)
        {
            gbStorage = mDiscountedPlan->yearlyData().gBStorage();
        }
        return Utilities::getSizeString(gbStorage);
    }
    return {};
}

QString DiscountPolicy::getTransfer() const
{
    if (mDiscountedPlan)
    {
        auto gbTransfer = mDiscountedPlan->monthlyData().gBTransfer() * getMonths();
        if (gbTransfer < 0)
        {
            gbTransfer = mDiscountedPlan->yearlyData().gBTransfer();
        }
        return Utilities::getSizeString(gbTransfer);
    }
    return {};
}

QStringList DiscountPolicy::getPlanFeatures() const
{
    return QStringList() << QCoreApplication::translate("OfferStrings", "MEGA VPN")
                         << QCoreApplication::translate("OfferStrings", "MEGA Pass")
                         << QCoreApplication::translate("OfferStrings", "Object storage");
}

QString DiscountPolicy::getPrice() const
{
    if (mDiscountInfo)
    {
        const auto plans = mUpsellController ? mUpsellController->getPlans() : nullptr;
        QString currency;
        if (plans)
        {
            currency = plans->getCurrencySymbol();
        }
        const auto localByteSymbol = Utilities::decodeUnicodeEscapes(
            QString::fromUtf8(mDiscountInfo->getLocalCurrencySymbol()));

        if (localByteSymbol.isEmpty())
        {
            return Utilities::toPrice(mDiscountInfo->getEuroTotalPriceNet(), currency, true);
        }
        else
        {
            auto price = mDiscountInfo->getLocalTotalPriceNet();
            if (std::abs(price) < DOUBLE_COMPARISON_EPSILON)
            {
                price = mDiscountInfo->getLocalTotalPrice();
            }
            return Utilities::toPrice(price, localByteSymbol, true);
        }
    }
    return {};
}

QString DiscountPolicy::getDiscountedPrice() const
{
    if (mDiscountInfo)
    {
        const auto plans = mUpsellController ? mUpsellController->getPlans() : nullptr;
        QString currency;
        if (plans)
        {
            currency = plans->getCurrencySymbol();
        }

        const auto localByteSymbol = Utilities::decodeUnicodeEscapes(
            QString::fromUtf8(mDiscountInfo->getLocalCurrencySymbol()));

        if (localByteSymbol.isEmpty())
        {
            return Utilities::toPrice(mDiscountInfo->getEuroDiscountedTotalPriceNet(),
                                      currency,
                                      true);
        }
        else
        {
            auto price = mDiscountInfo->getLocalDiscountedTotalPriceNet();
            if (std::abs(price) < DOUBLE_COMPARISON_EPSILON)
            {
                price = mDiscountInfo->getLocalDiscountedTotalPrice();
            }
            return Utilities::toPrice(price, localByteSymbol, true);
        }
    }
    return {};
}

bool DiscountPolicy::hasTax() const
{
    return mDiscountInfo ? mDiscountInfo->getTaxRate() != -1 : true;
}

bool DiscountPolicy::localCurrencyIsBillingCurrency() const
{
    auto localCurrencyIsBillingCurrency = true; // Default to true
    if (mDiscountInfo)
    {
        // Local currency is billing currency if the API doesn't give us a local currency symbol.
        localCurrencyIsBillingCurrency =
            QString::fromUtf8(mDiscountInfo->getLocalCurrencySymbol()).isEmpty();
    }
    return localCurrencyIsBillingCurrency;
}

QString DiscountPolicy::getCode() const
{
    return mDiscountCode;
}

int DiscountPolicy::getPercentage() const
{
    return mDiscountInfo ? mDiscountInfo->getPercentageDiscount() : 0;
}

int DiscountPolicy::getMonths() const
{
    return mDiscountInfo ? mDiscountInfo->getMonths() : 0;
}

void DiscountPolicy::onPlansReady()
{
    const auto plans = mUpsellController ? mUpsellController->getPlans() : nullptr;

    if (mDiscountInfo && plans && plans->size() > 0)
    {
        mDiscountedPlan =
            findPlanByLevel(mDiscountInfo->getAccountLevel()); // For now, use the first plan

        if (mDiscountedPlan && mIsPlanPending)
        {
            if (mIsNewCampaign)
            {
                MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(
                    AppStatsEvents::EventType::TARGETED_DISCOUNT_CAMPAIGN_STARTED);
                mIsNewCampaign = false;
            }
            // Update in case it has changed (not very probable)
            persist();
            mIsCampaignActive = true;

            emit campaignActivated();
        }
    }

    if (!mDiscountedPlan)
    {
        mega::MegaApi::log(mega::MegaApi::LOG_LEVEL_INFO, "Targeted discount: No matching plan");
        deactivateCampaign();
    }

    mIsPlanPending = false;
}

bool DiscountPolicy::load()
{
    if (mPreferences->logged())
    {
        mLastTimeShownUtc = mPreferences->getOfferDialogLastExecution();
        mDiscountCode = mPreferences->getDiscountCode();
        mCampaignExpiryDateUtc = mPreferences->getOfferDialogCampaignExpiryDate();
        mIsLoadingPersistedDataNeeded = false;
    }
    return !mDiscountCode.isEmpty();
}

void DiscountPolicy::persist() const
{
    if (mPreferences->logged())
    {
        mPreferences->setOfferDialogLastExecution(mLastTimeShownUtc);
        mPreferences->setDiscountCode(mDiscountCode);
        mPreferences->setOfferDialogCampaignExpiryDate(mCampaignExpiryDateUtc);
    }
}

void DiscountPolicy::checkAndDeactivateExpiredCampaign()
{
    if (isCampaignExpiredUtc(mCampaignExpiryDateUtc))
    {
        deactivateCampaign();
    }
}

bool DiscountPolicy::isCampaignExpiredUtc(const QDateTime& expiryDateUtc)
{
    return expiryDateUtc <= QDateTime::currentDateTimeUtc();
}

bool DiscountPolicy::isDiscountValid(
    const std::shared_ptr<mega::MegaDiscountCodeInfo>& discountInfo)
{
    // Do not process any further if prices are 0
    return std::lround(discountInfo->getEuroTotalPrice()) != 0 ||
           std::lround(discountInfo->getEuroTotalPriceNet()) != 0 ||
           std::lround(discountInfo->getEuroDiscountedTotalPrice()) != 0 ||
           std::lround(discountInfo->getEuroDiscountAmount()) != 0;
}

std::shared_ptr<UpsellPlans::Data> DiscountPolicy::findPlanByLevel(int level) const
{
    if (mUpsellController)
    {
        if (auto plans = mUpsellController->getPlans())
        {
            for (int i = 0; i < plans->size(); ++i)
            {
                auto plan = plans->getPlan(i);
                if (plan->proLevel() == level)
                {
                    return plan;
                }
            }
        }
    }
    return {};
}