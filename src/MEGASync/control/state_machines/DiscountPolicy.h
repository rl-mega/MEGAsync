#pragma once

#include "megaapi.h"
#include "Preferences.h"
#include "UpsellController.h"
#include "UpsellPlans.h"

#include <QDateTime>
#include <QObject>
#include <QString>

#include <memory>

class DiscountPolicy: public QObject
{
    Q_OBJECT

public:
    explicit DiscountPolicy(QObject* parent = nullptr);

    void activateCampaign(std::shared_ptr<mega::MegaDiscountCodeInfo> discountInfo);
    void deactivateCampaign();
    bool isCampaignActive() const;
    void recordShown();
    void recordDismissed();
    void recordAccepted();
    QDateTime getDialogLastShownDateUtc() const;
    QDateTime getExpiryDateUtc() const;
    QString getCurrencySymbol() const;
    QString getCurrencyName() const;
    QString getPlanName(bool shortName = true) const;
    QString getStorage() const;
    QString getTransfer() const;
    QStringList getPlanFeatures() const;
    QString getPrice() const;
    QString getDiscountedPrice() const;
    bool hasTax() const;
    bool localCurrencyIsBillingCurrency() const;
    QString getCode() const;
    int getPercentage() const;
    int getMonths() const;

signals:
    void campaignActivated();
    void campaignDeactivated();

protected:
    bool load();
    void persist() const;
    void checkAndDeactivateExpiredCampaign();
    static bool isDiscountValid(const std::shared_ptr<mega::MegaDiscountCodeInfo>& discountInfo);
    static bool isCampaignExpiredUtc(const QDateTime& expiryDateUtc);
    std::shared_ptr<UpsellPlans::Data> findPlanByLevel(int level) const;

    // State
    bool mIsCampaignActive = false;
    bool mIsLoadingPersistedDataNeeded = true;
    bool mIsNewCampaign = false;
    bool mIsPlanPending = false;
    QDateTime mLastTimeShownUtc;
    QDateTime mCampaignExpiryDateUtc;
    QString mDiscountCode;

    std::shared_ptr<mega::MegaDiscountCodeInfo> mDiscountInfo;
    std::shared_ptr<Preferences> mPreferences;

    QPointer<UpsellController> mUpsellController;
    std::shared_ptr<UpsellPlans::Data> mDiscountedPlan = nullptr;

protected slots:
    void onPlansReady();
};
