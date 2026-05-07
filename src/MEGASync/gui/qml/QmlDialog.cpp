#include "QmlDialog.h"

#include "DialogOpener.h"
#include "QmlDialogWrapperUtilities.h"

#include <QEvent>
#include <QResizeEvent>
#include <QScreen>

namespace
{
const QLatin1String DEFAULT_RES_MEGA_ICON(":/images/app_ico.ico");
const QLatin1String DEFAULT_TITLE("MEGA");
const qreal HIDDEN_OPACITY(0.0);
const qreal DEFAULT_VISIBLE_OPACITY(1.0);
const int SHOW_WHEN_CREATED_FALLBACK_DELAY_MS(60);
const int RESTORE_OPACITY_DELAY_MS(60);
}

QmlDialog::QmlDialog(QWindow* parent):
    QQuickWindow(parent),
    mIconSrc(DEFAULT_RES_MEGA_ICON),
    mInstancesManager(new QmlInstancesManager())
{
    setFlags(flags() | Qt::Dialog);
    setIcon(QIcon(mIconSrc));
    setTitle(DEFAULT_TITLE);
    setVisible(false);

    connect(this,
            &QmlDialog::requestPageFocus,
            this,
            &QmlDialog::onRequestPageFocus,
            Qt::QueuedConnection);

    connect(mInstancesManager,
            &QmlInstancesManager::instancesChanged,
            this,
            &QmlDialog::instancesManagerChanged);

    mShowWhenCreatedFallbackTimer.setInterval(SHOW_WHEN_CREATED_FALLBACK_DELAY_MS);
    mShowWhenCreatedFallbackTimer.setSingleShot(true);
    connect(&mShowWhenCreatedFallbackTimer,
            &QTimer::timeout,
            this,
            [this]()
            {
                if (mCenterAndRaiseAfterFirstHeightChangeEvent)
                {
                    placeAndRaise();
                }
            });

    // We restore the opacity with a timer to ensure the dialog is render
    mRestoreOpacityTimer.setInterval(RESTORE_OPACITY_DELAY_MS);
    mRestoreOpacityTimer.setSingleShot(true);
    connect(&mRestoreOpacityTimer,
            &QTimer::timeout,
            this,
            [this]()
            {
                setOpacity(mPreviousOpacity > HIDDEN_OPACITY ? mPreviousOpacity :
                                                               DEFAULT_VISIBLE_OPACITY);

                // The following two lines are required by Windows (activate) and macOS (raise)
                requestActivate();
                raise();
            });
}

void QmlDialog::setIconSrc(const QString& iconSrc)
{
    QString source = iconSrc;
    if (iconSrc.startsWith(QString::fromUtf8("qrc:")))
    {
        source = source.mid(3);
    }

    if (source != mIconSrc)
    {
        mIconSrc = source;
        setIcon(QIcon(mIconSrc));
    }
}

QmlInstancesManager* QmlDialog::getInstancesManager()
{
    return mInstancesManager;
}

void QmlDialog::readyToBeShow()
{
    mCenterAndRaiseAfterFirstHeightChangeEvent = true;
    mTrackedSize = geometry().size();

    hide();
    // Set the opacity to 0.0 to hide the window even if it is shown
    // The opacity will be set again to the real opacity
    mPreviousOpacity = opacity() > HIDDEN_OPACITY ? opacity() : DEFAULT_VISIBLE_OPACITY;
    setOpacity(HIDDEN_OPACITY);
    show();
    mShowWhenCreatedFallbackTimer.start();
}

bool QmlDialog::getCloseOnEscapePressed() const
{
    return mCloseOnEscapePressed;
}

void QmlDialog::setCloseOnEscapePressed(bool active)
{
    if (active != mCloseOnEscapePressed)
    {
        mCloseOnEscapePressed = active;

        emit closeOnEscapePressedChanged();
    }
}

bool QmlDialog::event(QEvent* event)
{
    if (event->type() == QEvent::Close)
    {
        emit finished();
    }
    else if (event->type() == QEvent::Resize)
    {
        auto* resizeEvent = static_cast<QResizeEvent*>(event);
        if (mCenterAndRaiseAfterFirstHeightChangeEvent)
        {
            mShowWhenCreatedFallbackTimer.start();
        }

#ifdef Q_OS_LINUX
        // Linux qml dialogs starts with QSize(1,1), so the first resize is invalid
        if (mTrackedSize.height() <= 1)
        {
            mTrackedSize = resizeEvent->size();
            return QQuickWindow::event(event);
        }
#endif

        if (resizeEvent && mCenterAndRaiseAfterFirstHeightChangeEvent &&
            mTrackedSize != resizeEvent->size())
        {
            placeAndRaise();
        }
    }
    else if (event->type() == QEvent::KeyPress)
    {
        auto* keyPressEvent = static_cast<QKeyEvent*>(event);
        if (mCloseOnEscapePressed && keyPressEvent != nullptr &&
            keyPressEvent->key() == Qt::Key_Escape)
        {
            close();
        }
    }

    return QQuickWindow::event(event);
}

void QmlDialog::onRequestPageFocus()
{
    emit initializePageFocus();
}

void QmlDialog::placeAndRaise()
{
    mShowWhenCreatedFallbackTimer.stop();

    mCenterAndRaiseAfterFirstHeightChangeEvent = false;

    QSize dialogSize = geometry().size();
    auto parentGeometry = QmlDialogWrapperUtilities::getParentGeometry(this);
    QmlDialog::setFramePosition(DialogOpener::initialDialogPosition(dialogSize, parentGeometry));

    mRestoreOpacityTimer.start();
}
