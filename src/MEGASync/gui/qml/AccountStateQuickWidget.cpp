#include "AccountStateQuickWidget.h"

#include "control/TextDecorator.h"
#include "megaapi.h"
#include "Preferences.h"
#include "Utilities.h"

#include <QQmlContext>

#include <algorithm>

namespace
{
QString decorateTooltipText(const QString& text)
{
    auto decoratedText = text;
    Text::NewLine().process(decoratedText);
    return decoratedText;
}

QVariantMap buildUsageSegment(AccountStateQuickWidget::UsageSegmentType type,
                              long long value,
                              const QVariantList& children = QVariantList())
{
    QVariantMap segment;
    segment.insert(QString::fromLatin1("type"), static_cast<int>(type));
    segment.insert(QString::fromLatin1("value"), QVariant::fromValue(value));
    segment.insert(QString::fromLatin1("sizeText"), Utilities::getSizeString(value));
    if (!children.isEmpty())
    {
        segment.insert(QString::fromLatin1("children"), children);
    }
    return segment;
}

void appendUsageSegment(QVariantList& segments,
                        AccountStateQuickWidget::UsageSegmentType type,
                        long long value)
{
    if (value <= 0)
    {
        return;
    }

    segments.push_back(buildUsageSegment(type, value));
}

QVariantList buildStorageSegments(Preferences* preferences)
{
    const auto cloudDriveStorage = preferences->cloudDriveStorage();
    const auto versionsStorage = preferences->versionsStorage();
    const auto availableStorage =
        std::max(0ll, preferences->totalStorage() - preferences->usedStorage());
    const auto isBusinessAccount = Utilities::isBusinessAccount();

    QVariantList segments;
    if (cloudDriveStorage > 0 || versionsStorage > 0)
    {
        QVariantList children;
        appendUsageSegment(children, AccountStateQuickWidget::Versions, versionsStorage);

        segments.push_back(
            buildUsageSegment(AccountStateQuickWidget::CloudDrive, cloudDriveStorage, children));
    }
    appendUsageSegment(segments, AccountStateQuickWidget::Backups, preferences->vaultStorage());
    appendUsageSegment(segments,
                       AccountStateQuickWidget::RubbishBin,
                       preferences->rubbishStorage());
    if (!isBusinessAccount)
    {
        appendUsageSegment(segments, AccountStateQuickWidget::Free, availableStorage);
    }

    return segments;
}

QVariantList buildTransferSegments(Preferences* preferences)
{
    QVariantList segments;
    appendUsageSegment(segments, AccountStateQuickWidget::Downloads, preferences->usedBandwidth());

    const auto totalBandwidth = preferences->totalBandwidth();
    if (totalBandwidth > 0)
    {
        const auto availableBandwidth =
            std::max(0ll, totalBandwidth - preferences->usedBandwidth());
        appendUsageSegment(segments, AccountStateQuickWidget::Free, availableBandwidth);
    }

    return segments;
}

AccountStateQuickWidget::ProgressState quotaStateToProgressState(QuotaState quotaState)
{
    switch (quotaState)
    {
        case QuotaState::WARNING:
            return AccountStateQuickWidget::WARNING;
        case QuotaState::OVERQUOTA:
        case QuotaState::FULL:
            return AccountStateQuickWidget::FULL;
        case QuotaState::OK:
        default:
            return AccountStateQuickWidget::OK;
    }
}

AccountStateQuickWidget::ProgressState storageStateToProgressState(int storageState)
{
    switch (storageState)
    {
        case mega::MegaApi::STORAGE_STATE_PAYWALL:
        case mega::MegaApi::STORAGE_STATE_RED:
            return AccountStateQuickWidget::FULL;
        case mega::MegaApi::STORAGE_STATE_ORANGE:
            return AccountStateQuickWidget::WARNING;
        case mega::MegaApi::STORAGE_STATE_UNKNOWN:
        case mega::MegaApi::STORAGE_STATE_GREEN:
        default:
            return AccountStateQuickWidget::OK;
    }
}
}

AccountStateQuickWidget::AccountStateQuickWidget(QWidget* parent):
    MegaQuickWidget(parent)
{
    rootContext()->setContextProperty(QString::fromLatin1("accountStateAccess"), this);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setSource(QString::fromUtf8("qrc:/settings/AccountStateSurface.qml"));
}

bool AccountStateQuickWidget::showStorageCard() const
{
    return mShowStorageCard;
}

bool AccountStateQuickWidget::showTransferCard() const
{
    return mShowTransferCard;
}

QString AccountStateQuickWidget::storageSummary() const
{
    return mStorageSummary;
}

QString AccountStateQuickWidget::storageFreeText() const
{
    return mStorageFreeText;
}

QString AccountStateQuickWidget::storageFreeTooltipText() const
{
    return mStorageFreeTooltipText;
}

int AccountStateQuickWidget::storagePercentage() const
{
    return mStoragePercentage;
}

int AccountStateQuickWidget::storageState() const
{
    return mStorageState;
}

QVariantList AccountStateQuickWidget::storageSegments() const
{
    return mStorageSegments;
}

bool AccountStateQuickWidget::storageUsageOnly() const
{
    return mStorageUsageOnly;
}

QString AccountStateQuickWidget::transferSummary() const
{
    return mTransferSummary;
}

QString AccountStateQuickWidget::transferFreeText() const
{
    return mTransferFreeText;
}

QString AccountStateQuickWidget::transferFreeTooltipText() const
{
    return mTransferFreeTooltipText;
}

int AccountStateQuickWidget::transferPercentage() const
{
    return mTransferPercentage;
}

int AccountStateQuickWidget::transferState() const
{
    return mTransferState;
}

QVariantList AccountStateQuickWidget::transferSegments() const
{
    return mTransferSegments;
}

bool AccountStateQuickWidget::transferValueOnly() const
{
    return mTransferValueOnly;
}

QString AccountStateQuickWidget::transferValueText() const
{
    return mTransferValueText;
}

void AccountStateQuickWidget::setShowStorageCard(bool showStorageCard)
{
    if (mShowStorageCard != showStorageCard)
    {
        mShowStorageCard = showStorageCard;
        emit showStorageCardChanged();
    }
}

void AccountStateQuickWidget::setShowTransferCard(bool showTransferCard)
{
    if (mShowTransferCard == showTransferCard)
    {
        return;
    }

    mShowTransferCard = showTransferCard;
    emit showTransferCardChanged();
}

void AccountStateQuickWidget::updateStorageData()
{
    auto preferences = Preferences::instance().get();
    if (!preferences)
    {
        return;
    }

    const auto totalStorage = preferences->totalStorage();
    const auto usedStorage = preferences->usedStorage();
    const auto availableStorage = std::max(0ll, totalStorage - usedStorage);
    const auto isBusinessAccount = Utilities::isBusinessAccount();

    QString storageSummary;
    QString availableText;
    int percentage = 0;
    auto state = AccountStateQuickWidget::OK;
    const auto segments = buildStorageSegments(preferences);

    if (totalStorage > 0 && isBusinessAccount)
    {
        storageSummary = Utilities::createSimpleUsedString(usedStorage);
        percentage = 100;
    }
    else if (totalStorage > 0)
    {
        percentage = Utilities::partPer(usedStorage, totalStorage);
        state = storageStateToProgressState(preferences->getStorageState());
        storageSummary = Utilities::createCompleteUsedString(usedStorage, totalStorage, percentage);
        availableText = Utilities::getSizeString(availableStorage);
    }

    setStorageData(storageSummary,
                   availableText,
                   percentage,
                   state,
                   segments,
                   isBusinessAccount && totalStorage > 0);
}

void AccountStateQuickWidget::updateTransferData(QuotaState quotaState)
{
    auto preferences = Preferences::instance().get();
    if (!preferences)
    {
        return;
    }

    const auto totalBandwidth = preferences->totalBandwidth();
    const auto usedBandwidth = preferences->usedBandwidth();
    const auto availableBandwidth = std::max(0ll, totalBandwidth - usedBandwidth);
    const auto isBusinessAccount = Utilities::isBusinessAccount();

    QString transferSummary;
    QString availableText;
    int percentage = 0;
    auto state = AccountStateQuickWidget::OK;
    const auto segments = buildTransferSegments(preferences);
    QString transferValueText;

    if (isBusinessAccount)
    {
        transferValueText = Utilities::getSizeString(usedBandwidth);
    }
    else if (totalBandwidth > 0)
    {
        percentage = Utilities::partPer(usedBandwidth, totalBandwidth);
        state = quotaStateToProgressState(quotaState);
        transferSummary = Utilities::createCompleteUsedString(usedBandwidth,
                                                              totalBandwidth,
                                                              std::min(percentage, 100));
        availableText = Utilities::getSizeString(availableBandwidth);
    }

    setTransferData(transferSummary,
                    availableText,
                    percentage,
                    state,
                    segments,
                    isBusinessAccount,
                    transferValueText);
}

void AccountStateQuickWidget::setStorageData(const QString& summary,
                                             const QString& freeText,
                                             int percentage,
                                             ProgressState state,
                                             const QVariantList& segments,
                                             bool usageOnly)
{
    if (mStorageSummary != summary)
    {
        mStorageSummary = summary;
        emit storageSummaryChanged();
    }

    const auto tooltipText =
        freeText.isEmpty() ? QString() : decorateTooltipText(tr("Available[BR]%1").arg(freeText));
    if (mStorageFreeText != freeText)
    {
        mStorageFreeText = freeText;
        emit storageFreeTextChanged();
    }
    if (mStorageFreeTooltipText != tooltipText)
    {
        mStorageFreeTooltipText = tooltipText;
        emit storageFreeTooltipTextChanged();
    }
    if (mStoragePercentage != percentage)
    {
        mStoragePercentage = percentage;
        emit storagePercentageChanged();
    }
    if (mStorageState != state)
    {
        mStorageState = state;
        emit storageStateChanged();
    }
    if (mStorageSegments != segments)
    {
        mStorageSegments = segments;
        emit storageSegmentsChanged();
    }
    if (mStorageUsageOnly != usageOnly)
    {
        mStorageUsageOnly = usageOnly;
        emit storageUsageOnlyChanged();
    }
}

void AccountStateQuickWidget::setTransferData(const QString& summary,
                                              const QString& freeText,
                                              int percentage,
                                              ProgressState state,
                                              const QVariantList& segments,
                                              bool valueOnly,
                                              const QString& valueText)
{
    if (mTransferSummary != summary)
    {
        mTransferSummary = summary;
        emit transferSummaryChanged();
    }

    const auto tooltipText =
        freeText.isEmpty() ? QString() : decorateTooltipText(tr("Available[BR]%1").arg(freeText));
    if (mTransferFreeText != freeText)
    {
        mTransferFreeText = freeText;
        emit transferFreeTextChanged();
    }
    if (mTransferFreeTooltipText != tooltipText)
    {
        mTransferFreeTooltipText = tooltipText;
        emit transferFreeTooltipTextChanged();
    }
    if (mTransferPercentage != percentage)
    {
        mTransferPercentage = percentage;
        emit transferPercentageChanged();
    }
    if (mTransferState != state)
    {
        mTransferState = state;
        emit transferStateChanged();
    }
    if (mTransferSegments != segments)
    {
        mTransferSegments = segments;
        emit transferSegmentsChanged();
    }
    if (mTransferValueOnly != valueOnly)
    {
        mTransferValueOnly = valueOnly;
        emit transferValueOnlyChanged();
    }
    if (mTransferValueText != valueText)
    {
        mTransferValueText = valueText;
        emit transferValueTextChanged();
    }
}
