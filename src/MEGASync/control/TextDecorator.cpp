#include "TextDecorator.h"

#include "TokenParserWidgetManager.h"

#include <QDebug>

namespace Text
{

Decorator::Decorator(QObject *parent) :
    QObject(parent)
{
}

void Decorator::process(QString &input) const
{
    if(parent())
    {
        if(Decorator *td = qobject_cast<Decorator*>(parent()))
        {
            td->process(input);
        }
    }
}

Link::Link(const QStringList& links, QObject *parent) :
    Decorator(parent),
    mLinkAddresses(links)
{
}

Link::Link(const QString& link, QObject* parent):
    Decorator(parent),
    mLinkAddresses(link)
{

}

//Use [A] for url replacement
static const QString headerTag = QLatin1String("[A]");
void Link::process(QString &input) const
{
    Decorator::process(input);

    // QLabel honors `<a style="color:...">`, but QTextBrowser (e.g. WordWrapLabel) renders
    // anchors with QPalette::Link and ignores inline style on the <a> itself. Wrapping the
    // link text in <font color=...> applies the token color in both cases.
    const QString linkColor =
        TokenParserWidgetManager::instance()->getColor(QLatin1String("link-primary")).name();

    auto headerLength(headerTag.length());
    auto currentIndex(0);
    foreach(auto link, mLinkAddresses)
    {
        currentIndex = input.indexOf(headerTag, currentIndex);
        const QString href = link.isEmpty() ? QLatin1String("empty") : link;
        const QString replacement =
            QString::fromUtf8("<a href=\"%1\" style=\"color:%2;\"><font color=\"%2\">")
                .arg(href, linkColor);
        input.replace(currentIndex, headerLength, replacement);
        currentIndex += replacement.length();
    }

    input.replace(QLatin1String("[/A]"), QLatin1String("</font></a>"));
}

ClearLink::ClearLink(QObject *parent) : Decorator(parent)
{

}

void ClearLink::process(QString &input) const
{
    Decorator::process(input);
    input.remove(QLatin1String("[A]"));
    input.remove(QLatin1String("[/A]"));
}

Bold::Bold(QObject *parent) : Decorator(parent)
{
}

//Use [B] for bold tags replacement
void Bold::process(QString &input) const
{
    Decorator::process(input);
    input.replace(QLatin1String("[B]"), QLatin1String("<b>"));
    input.replace(QLatin1String("[/B]"), QLatin1String("</b>"));
}

NewLine::NewLine(QObject *parent) : Decorator(parent)
{
}

//Use [BR] for new line tags replacement
void NewLine::process(QString &input) const
{
    Decorator::process(input);
    input.replace(QLatin1String("[BR]"), QLatin1String("<br>"));
    input.replace(QLatin1String("[/BR]"), QLatin1String(""));
}

void NewLine::process(QString &input, int count) const
{
    Decorator::process(input);

    QString brTag = QString::fromUtf8("<br>").repeated(count);
    input.replace(QLatin1String("[BR]"), brTag);
    input.replace(QLatin1String("[/BR]"), QLatin1String(""));
}

RichText::RichText(QObject* parent):
    Decorator(parent),
    mLinkAddresses(QStringList())
{}

RichText::RichText(const QStringList& links, QObject* parent):
    Decorator(parent),
    mLinkAddresses(links)
{}

RichText::RichText(const QString& link, QObject* parent):
    Decorator(parent),
    mLinkAddresses(link)
{}

void RichText::process(QString& input) const
{
    if (!mLinkAddresses.isEmpty())
    {
        Text::Link(mLinkAddresses).process(input);
    }
    Text::Bold().process(input);
    Text::NewLine().process(input);
}
}
