/* ============================================================
   SearchGather.cpp —— 搜索词拆分/合并实现
   ============================================================ */

#include "SearchGather.h"
#include <QRegularExpression>

static const QRegularExpression COMMA_RE(QStringLiteral("[,，]"));

QStringList SearchGather::splitSegments(const QString &q)
{
    QStringList parts;
    const auto segs = q.split(COMMA_RE);
    for (const auto &s : segs) {
        QString trimmed = s.trimmed();
        if (!trimmed.isEmpty())
            parts.append(trimmed);
    }
    return parts;
}

QString SearchGather::getSuggestToken(const QString &q)
{
    QStringList parts = splitSegments(q);
    return parts.isEmpty() ? QString() : parts.last();
}

QString SearchGather::mergeSuggestion(const QString &q, const QString &suggestion)
{
    QString text = suggestion.trimmed();
    if (text.isEmpty()) return q;

    if (!q.contains(COMMA_RE)) return text;

    QStringList parts = splitSegments(q);
    if (parts.isEmpty()) return text;
    parts[parts.size() - 1] = text;

    QString sep = q.contains(QChar(0xFF0C)) ? QStringLiteral("，") : QStringLiteral(", ");
    return parts.join(sep);
}
