#include "GuestContent.h"

#include "GuestQmlDialog.h"
#include "MegaApplication.h"
#include "Preferences.h"
#include "QmlDialogManager.h"

#include <QQmlEngine>

GuestContent::GuestContent(QObject* parent):
    QMLComponent(parent)
{
    qmlRegisterModule("GuestContent", 1, 0);
    qmlRegisterType<GuestQmlDialog>("GuestQmlDialog", 1, 0, "GuestQmlDialog");
}

void GuestContent::onSignUpClicked()
{
    openOnboardingDialog(LoginController::SIGN_UP);
}

void GuestContent::onLoginClicked()
{
    openOnboardingDialog(LoginController::LOGGED_OUT);
}

void GuestContent::onAboutMEGAClicked()
{
    MegaSyncApp->onAboutClicked();
}

void GuestContent::onPreferencesClicked()
{
    MegaSyncApp->openSettings();
}

void GuestContent::onExitClicked()
{
    MegaSyncApp->tryExitApplication();
}

void GuestContent::onVerifyEmailClicked()
{
    MegaSyncApp->getMegaApi()->resendVerificationEmail();
}

void GuestContent::onLogoutClicked()
{
    MegaSyncApp->unlink();
}

QUrl GuestContent::getQmlUrl()
{
    return QUrl(QString::fromUtf8("qrc:/guest/GuestDialog.qml"));
}

void GuestContent::openOnboardingDialog(LoginController::State state)
{
    const auto canOpenOnboardingDialog =
        !MegaSyncApp->finished() && Preferences::instance()->getSession().isEmpty();

    if (canOpenOnboardingDialog)
    {
        if (auto* loginController = MegaSyncApp->getLoginController())
        {
            loginController->setState(state);
        }

        QmlDialogManager::instance()->openOnboardingDialog();
    }
}
