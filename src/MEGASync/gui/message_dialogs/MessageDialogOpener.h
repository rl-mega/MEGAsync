#ifndef MESSAGE_DIALOG_OPENER_H
#define MESSAGE_DIALOG_OPENER_H

#include "DialogOpener.h"
#include "MessageDialogComponent.h"
#include "MessageDialogData.h"
#include "QmlDialogWrapper.h"

class MessageDialogOpener
{
public:
    explicit MessageDialogOpener() = delete;
    virtual ~MessageDialogOpener() = delete;

    static void success(const MessageDialogInfo& info);
    static void information(const MessageDialogInfo& info);
    static void warning(const MessageDialogInfo& info);
    static void question(const MessageDialogInfo& info);
    static void critical(const MessageDialogInfo& info);

private:
    static void show(MessageDialogData::Type type, const MessageDialogInfo& info);

    template<typename parentType>
    static void createQmlDialogWrapper(MessageDialogData::Type type,
                                       const MessageDialogInfo& info,
                                       parentType* parent)
    {
        QPointer<MessageDialogData> data = new MessageDialogData(type, info, parent);
        auto dialog = new QmlDialogWrapper<MessageDialogComponent>(parent, data);
        dialog->setShowWhenCreated();
        DialogOpener::showMessageDialog(dialog, data);
    }
};

#endif // MESSAGE_DIALOG_OPENER_H
