/* ============================================================
   SearchGather.h —— 搜索词拆分/合并/联想处理
   ============================================================ */

#pragma once

#include <QString>
#include <QStringList>

class SearchGather
{
public:
    /** 按中英文逗号拆分为多个搜索词 */
    static QStringList splitSegments(const QString &q);

    /** 取最后一段作为联想 key */
    static QString getSuggestToken(const QString &q);

    /** 用联想词替换输入框最后一段 */
    static QString mergeSuggestion(const QString &q, const QString &suggestion);
};
