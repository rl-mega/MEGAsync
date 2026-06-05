#include "PermissionsDialog.h"

#include "ui_PermissionsDialog.h"

#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>

PermissionsDialog::PermissionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PermissionsDialog)
{
    ui->setupUi(this);

    ui->wFileOwner->configurePermissions(PermissionsWidget::ENABLED_EXECUTION);

    connect(ui->wFileOwner, SIGNAL(onPermissionChanged()), this, SLOT(permissionsChanged()));
    connect(ui->wFileGroup, SIGNAL(onPermissionChanged()), this, SLOT(permissionsChanged()));
    connect(ui->wFilePublic, SIGNAL(onPermissionChanged()), this, SLOT(permissionsChanged()));
    connect(ui->wFolderGroup, SIGNAL(onPermissionChanged()), this, SLOT(permissionsChanged()));
    connect(ui->wFolderPublic, SIGNAL(onPermissionChanged()), this, SLOT(permissionsChanged()));

    // Folder owner is fixed at 7; group and public are octal digits.
    auto* folderValidator =
        new QRegularExpressionValidator(QRegularExpression(QStringLiteral("7[0-7]{2}")), this);
    ui->lFolderPermissions->setValidator(folderValidator);

    // File owner has read+write locked on (only execute is user-toggleable, see
    // configurePermissions(ENABLED_EXECUTION) above), so the owner digit must be 6 or 7.
    auto* fileValidator =
        new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[67][0-7]{2}")), this);
    ui->lFilePermissions->setValidator(fileValidator);
}

PermissionsDialog::~PermissionsDialog()
{
    delete ui;
}

int PermissionsDialog::folderPermissions()
{
    return ui->lFolderPermissions->text().toInt(NULL, 8);
}

void PermissionsDialog::setFolderPermissions(int permissions)
{
    int group  = (permissions >> 3) & 0x07;
    int others = permissions & 0x07;

    ui->wFolderGroup->setDefaultPermissions(group);
    ui->wFolderPublic->setDefaultPermissions(others);
}

int PermissionsDialog::filePermissions()
{
    return ui->lFilePermissions->text().toInt(NULL, 8);
}

void PermissionsDialog::setFilePermissions(int permissions)
{
    int owner  = (permissions >> 6) & 0x07;
    int group  = (permissions >> 3) & 0x07;
    int others = permissions & 0x07;

    ui->wFileOwner->setDefaultPermissions(owner);
    ui->wFileGroup->setDefaultPermissions(group);
    ui->wFilePublic->setDefaultPermissions(others);
}

void PermissionsDialog::permissionsChanged()
{
    ui->lFolderPermissions->setText(QString::fromUtf8("7%1%2").arg(ui->wFolderGroup->getCurrentPermissions())
                                                              .arg(ui->wFolderPublic->getCurrentPermissions()));

    ui->lFilePermissions->setText(QString::fromUtf8("%1%2%3").arg(ui->wFileOwner->getCurrentPermissions())
                                                             .arg(ui->wFileGroup->getCurrentPermissions())
                                                             .arg(ui->wFilePublic->getCurrentPermissions()));
}

void PermissionsDialog::on_lFolderPermissions_textEdited(const QString& text)
{
    if (text.length() != 3)
    {
        return;
    }

    bool ok = false;
    const int value = text.toInt(&ok, 8);
    if (!ok)
    {
        return;
    }

    const QSignalBlocker blockGroup(ui->wFolderGroup);
    const QSignalBlocker blockPublic(ui->wFolderPublic);
    setFolderPermissions(value);
}

void PermissionsDialog::on_lFilePermissions_textEdited(const QString& text)
{
    if (text.length() != 3)
    {
        return;
    }

    bool ok = false;
    const int value = text.toInt(&ok, 8);
    if (!ok)
    {
        return;
    }

    const QSignalBlocker blockOwner(ui->wFileOwner);
    const QSignalBlocker blockGroup(ui->wFileGroup);
    const QSignalBlocker blockPublic(ui->wFilePublic);
    setFilePermissions(value);
}

void PermissionsDialog::on_bUpdate_clicked()
{
    accept();
}

void PermissionsDialog::on_bCancel_clicked()
{
    reject();
}
