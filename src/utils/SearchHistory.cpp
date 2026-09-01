/* ============================================================
   SearchHistory.cpp —— 搜索历史持久化实现
   ============================================================ */

#include "SearchHistory.h"
#include <QSettings>

const QString SearchHistory::KEY = QStringLiteral("webcolor_search_history_v1");

SearchHistory &SearchHistory::instance()
{
    static SearchHistory inst;
    return inst;
}

QStringList SearchHistory::list() const
{
    QSettings settings;
    return settings.value(KEY).toStringList();
}

void SearchHistory::push(const QString &keyword)
{
    QString k = keyword.trimmed();
    if (k.isEmpty()) return;

    QStringList lst = list();
    lst.removeAll(k);          // 去重
    lst.prepend(k);            // 置顶
    if (lst.size() > MAX)
        lst = lst.mid(0, MAX);

    QSettings settings;
    settings.setValue(KEY, lst);
}

void SearchHistory::clear()
{
    QSettings settings;
    settings.setValue(KEY, QStringList());
}
