#include "MessageDialogOpener.h"

#include "Utilities.h"

#include <QPointer>

void MessageDialogOpener::success(const MessageDialogInfo& info)
{
    return show(MessageDialogData::Type::SUCCESS, info);
}

void MessageDialogOpener::information(const MessageDialogInfo& info)
{
    return show(MessageDialogData::Type::INFORMATION, info);
}

void MessageDialogOpener::warning(const MessageDialogInfo& info)
{
    return show(MessageDialogData::Type::WARNING, info);
}

void MessageDialogOpener::question(const MessageDialogInfo& info)
{
    return show(MessageDialogData::Type::QUESTION, info);
}

void MessageDialogOpener::critical(const MessageDialogInfo& info)
{
    return show(MessageDialogData::Type::CRITICAL, info);
}

void MessageDialogOpener::show(MessageDialogData::Type type, const MessageDialogInfo& info)
{
    auto showDialog = [type, info]()
    {
        if (info.parentQml)
        {
            createQmlDialogWrapper<QmlDialog>(type, info, info.parentQml);
        }
        else
        {
            createQmlDialogWrapper<QWidget>(type, info, info.parent);
        }
    };

    if (MegaSyncApp->thread() != MegaSyncApp->thread()->currentThread())
    {
        Utilities::queueFunctionInAppThread(
            [showDialog]()
            {
                showDialog();
            });
    }
    else
    {
        showDialog();
    }
}
