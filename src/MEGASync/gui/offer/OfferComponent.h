#include "QmlDialogWrapper.h"
#include "state_machines/DiscountPolicy.h"

#include <QDateTime>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>

class OfferComponent: public QMLComponent
{
    Q_OBJECT
    Q_PROPERTY(QString currencySymbol READ getCurrencySymbol NOTIFY dataUpdated)
    Q_PROPERTY(QString currencyName READ getCurrencyName NOTIFY dataUpdated)
    Q_PROPERTY(QString planName READ getPlanName NOTIFY dataUpdated)
    Q_PROPERTY(QString storage READ getStorage NOTIFY dataUpdated)
    Q_PROPERTY(QString transfer READ getTransfer NOTIFY dataUpdated)
    Q_PROPERTY(QString totalPrice READ getPrice NOTIFY dataUpdated)
    Q_PROPERTY(QString discountedPrice READ getDiscountedPrice NOTIFY dataUpdated)
    Q_PROPERTY(int days READ getDays NOTIFY countdownChanged)
    Q_PROPERTY(int hours READ getHours NOTIFY countdownChanged)
    Q_PROPERTY(int minutes READ getMinutes NOTIFY countdownChanged)
    Q_PROPERTY(qint64 seconds READ getSeconds NOTIFY countdownChanged)
    Q_PROPERTY(int discountPercentage READ getPercentage NOTIFY dataUpdated)
    Q_PROPERTY(int discountMonths READ getMonths NOTIFY dataUpdated)
    Q_PROPERTY(bool hasTax READ hasTax NOTIFY dataUpdated)

public:
    explicit OfferComponent(QObject* parent = nullptr);
    virtual ~OfferComponent();
    QUrl getQmlUrl() override;
    static void registerQmlModules();
    QString getCurrencySymbol() const;
    QString getCurrencyName() const;
    QString getPlanName() const;
    QString getStorage() const;
    QString getTransfer() const;
    QString getPrice() const;
    QString getDiscountedPrice() const;
    int getDays() const;
    int getHours() const;
    int getMinutes() const;
    bool hasTax() const;
    qint64 getSeconds() const;
    void setOfferExpirationDateUtc(const QDateTime& date);
    Q_INVOKABLE QStringList getPlanFeatures() const;
    int getPercentage() const;
    int getMonths() const;
    void setDiscountPolicy(QPointer<DiscountPolicy> discountPolicy);
    Q_INVOKABLE void onGrabDeal();
    Q_INVOKABLE bool localCurrencyIsBillingCurrency() const;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    long long msToNextCountdownMinuteTick() const;

protected slots:
    void onTimerFired();

signals:
    void dataUpdated();
    void countdownChanged();

private:
    QPointer<DiscountPolicy> mDiscountPolicy = nullptr;
    QTimer mCountDownTimer;
    QDateTime mOfferEndTimeUtc;
    QString mDiscountCode;
};
