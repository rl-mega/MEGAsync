#ifndef QML_DIALOG_WRAPPER_UTILITIES_H
#define QML_DIALOG_WRAPPER_UTILITIES_H

#include "QmlDialog.h"

#include <QObject>
#include <QRect>
#include <QVariant>

namespace QmlDialogWrapperUtilities
{

constexpr const char* IS_QML_PROPERTY = "IsQML";
constexpr const char* PARENT_GEOMETRY_PROPERTY = "ParentGeometry";
constexpr const char* SHOW_WHEN_CREATED_PROPERTY = "ShowWhenCreated";

inline void setIsQML(QObject* dialog)
{
    if (dialog)
    {
        dialog->setProperty(IS_QML_PROPERTY, true);
    }
}

inline bool isQML(const QObject* dialog)
{
    if (!dialog)
    {
        return false;
    }

    auto value(dialog->property(IS_QML_PROPERTY));
    return value.isValid() && value.toBool();
}

inline QRect getParentGeometry(const QObject* dialog)
{
    if (!dialog)
    {
        return QRect();
    }

    auto value(dialog->property(PARENT_GEOMETRY_PROPERTY));
    return value.isValid() ? value.toRect() : QRect();
}

inline void setParentGeometry(QObject* dialog, const QRect& parentGeometry)
{
    if (dialog)
    {
        dialog->setProperty(PARENT_GEOMETRY_PROPERTY, parentGeometry);
    }
}

inline void setShowWhenCreated(QObject* dialog, bool showWhenCreated)
{
    if (dialog)
    {
        dialog->setProperty(SHOW_WHEN_CREATED_PROPERTY, showWhenCreated);
    }
}

inline bool isShowWhenCreated(const QObject* dialog)
{
    if (!dialog)
    {
        return false;
    }

    auto value(dialog->property(SHOW_WHEN_CREATED_PROPERTY));
    return value.isValid() && value.toBool();
}
}

#endif // QML_DIALOG_WRAPPER_UTILITIES_H
