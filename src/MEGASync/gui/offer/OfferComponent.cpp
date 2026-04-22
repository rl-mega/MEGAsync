#include "OfferComponent.h"

#include "QmlManager.h"
#include "ServiceUrls.h"

namespace
{
static bool qmlRegistrationDone = false;
constexpr int MS_IN_ONE_MIN = 1000 * 60; // 1 minute
constexpr int COUNT_DOWN_UPDATE_INTERVAL_MS = MS_IN_ONE_MIN; // 1 minute
constexpr long long COUNT_DOWN_TOLERANCE_MS = 1500; // 1.5 seconds
}

OfferComponent::OfferComponent(QObject* parent):
    QMLComponent(parent)
{
    registerQmlModules();

    QmlManager::instance()->setRootContextProperty(QString::fromLatin1("offerComponentAccess"),
                                                   this);
    qApp->installEventFilter(this);

    mCountDownTimer.setInterval(COUNT_DOWN_UPDATE_INTERVAL_MS);
    connect(&mCountDownTimer, &QTimer::timeout, this, &OfferComponent::onTimerFired);
}

OfferComponent::~OfferComponent() {}

QUrl OfferComponent::getQmlUrl()
{
    return QUrl(QLatin1String("qrc:/offer/OfferDialog.qml"));
}

void OfferComponent::registerQmlModules()
{
    if (!qmlRegistrationDone)
    {
        qmlRegisterModule("Offer", 1, 0);
        qmlRegisterType<OfferComponent>("OfferComponent", 1, 0, "OfferComponent");
        qmlRegistrationDone = true;
    }
}

QString OfferComponent::getCurrencySymbol() const
{
    return mDiscountPolicy ? mDiscountPolicy->getCurrencySymbol() : QString();
}

QString OfferComponent::getCurrencyName() const
{
    return mDiscountPolicy ? mDiscountPolicy->getCurrencyName() : QString();
}

QString OfferComponent::getPlanName() const
{
    return mDiscountPolicy ? mDiscountPolicy->getPlanName() : QString();
}

QString OfferComponent::getStorage() const
{
    return mDiscountPolicy ? mDiscountPolicy->getStorage() : QString();
}

QString OfferComponent::getTransfer() const
{
    return mDiscountPolicy ? mDiscountPolicy->getTransfer() : QString();
}

QStringList OfferComponent::getPlanFeatures() const
{
    return mDiscountPolicy ? mDiscountPolicy->getPlanFeatures() : QStringList();
}

QString OfferComponent::getPrice() const
{
    return mDiscountPolicy ? mDiscountPolicy->getPrice() : QString();
}

QString OfferComponent::getDiscountedPrice() const
{
    return mDiscountPolicy ? mDiscountPolicy->getDiscountedPrice() : QString();
}

int OfferComponent::getDays() const
{
    qint64 secsRemaining = QDateTime::currentDateTime().secsTo(mOfferEndTimeUtc);
    if (secsRemaining <= 0)
    {
        return 0;
    }
    return static_cast<int>(secsRemaining / 86400); // 86400 secs in a day
}

int OfferComponent::getHours() const
{
    qint64 secsRemaining = QDateTime::currentDateTime().secsTo(mOfferEndTimeUtc);
    if (secsRemaining <= 0)
    {
        return 0;
    }
    return static_cast<int>((secsRemaining % 86400) / 3600);
}

int OfferComponent::getMinutes() const
{
    qint64 secsRemaining = QDateTime::currentDateTime().secsTo(mOfferEndTimeUtc);
    if (secsRemaining <= 0)
    {
        return 0;
    }
    return static_cast<int>((secsRemaining % 3600) / 60);
}

bool OfferComponent::hasTax() const
{
    return mDiscountPolicy ? mDiscountPolicy->hasTax() : true;
}

qint64 OfferComponent::getSeconds() const
{
    qint64 secsRemaining = QDateTime::currentDateTime().secsTo(mOfferEndTimeUtc);
    if (secsRemaining <= 0)
    {
        return 0;
    }
    return secsRemaining;
}

void OfferComponent::setOfferExpirationDateUtc(const QDateTime& date)
{
    mOfferEndTimeUtc = date;

    // Set single shot timer to next minute (relative to end date)
    QTimer::singleShot(msToNextCountdownMinuteTick(), this, &OfferComponent::onTimerFired);

    emit countdownChanged();
}

int OfferComponent::getPercentage() const
{
    return mDiscountPolicy ? mDiscountPolicy->getPercentage() : 0;
}

int OfferComponent::getMonths() const
{
    return mDiscountPolicy ? mDiscountPolicy->getMonths() : 0;
}

void OfferComponent::setDiscountPolicy(QPointer<DiscountPolicy> discountPolicy)
{
    if (discountPolicy)
    {
        mDiscountPolicy = discountPolicy;
        mDiscountCode = mDiscountPolicy->getCode();
        setOfferExpirationDateUtc(mDiscountPolicy->getExpiryDateUtc());
        emit dataUpdated();
    }
}

void OfferComponent::onGrabDeal()
{
    Utilities::openUrl(ServiceUrls::instance()->getDiscountUrl(mDiscountCode));
}

bool OfferComponent::localCurrencyIsBillingCurrency() const
{
    return mDiscountPolicy ? mDiscountPolicy->localCurrencyIsBillingCurrency() : true;
}

bool OfferComponent::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        emit dataUpdated();
    }
    return QMLComponent::eventFilter(obj, event);
}

void OfferComponent::onTimerFired()
{
    emit countdownChanged();

    // Realign if we drifted (sleep/wake or long stall) + check if we passed expiry
    const auto msToMinTick = msToNextCountdownMinuteTick();
    // We've reached the end
    if (msToMinTick <= 0)
    {
        mCountDownTimer.stop();
        return;
    }

    const bool aligned = (msToMinTick <= COUNT_DOWN_TOLERANCE_MS) ||
                         (MS_IN_ONE_MIN - msToMinTick <= COUNT_DOWN_TOLERANCE_MS);
    if (!aligned)
    {
        mCountDownTimer.stop();
        // Set single shot timer to next minute (relative to end date)
        QTimer::singleShot(msToMinTick, this, &OfferComponent::onTimerFired);
    }
    // Start timer if needed
    else if (!mCountDownTimer.isActive())
    {
        mCountDownTimer.start();
    }
}

long long OfferComponent::msToNextCountdownMinuteTick() const
{
    auto msLeft = QDateTime::currentDateTimeUtc().msecsTo(mOfferEndTimeUtc);

    // Return 0 if end has been reached
    if (msLeft < 0)
    {
        msLeft = 0;
    }
    // Return ms to next tick
    else if (msLeft > 0)
    {
        msLeft %= MS_IN_ONE_MIN;
        if (msLeft == 0)
        {
            msLeft = MS_IN_ONE_MIN;
        }
    }
    return msLeft;
}
