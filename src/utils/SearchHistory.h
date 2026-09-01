/* ============================================================
   SearchHistory.h —— 搜索历史管理（QSettings 持久化）
   - 最多 20 条，去重置顶
   ============================================================ */

#pragma once

#include <QStringList>

class SearchHistory
{
public:
    static SearchHistory &instance();

    QStringList list() const;
    void push(const QString &keyword);
    void clear();

private:
    SearchHistory() = default;
    static constexpr int MAX = 20;
    static const QString KEY;
};
