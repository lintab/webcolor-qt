/* ============================================================
   FormulaApi.h —— 配方查询接口
   ============================================================ */

#pragma once

#include <QObject>
#include <QJsonObject>
#include <functional>

class FormulaApi : public QObject
{
    Q_OBJECT
public:
    using Callback = std::function<void(const QJsonObject &)>;
    using ErrorCallback = std::function<void(const QString &errMsg, int code)>;

    static FormulaApi &instance();

    /** 搜索公司配方（分页） */
    void searchFormula(const QString &keyword, int pageNum, int pageSize,
                       Callback onSuccess, ErrorCallback onError);

    /** 搜索联想 */
    void suggest(const QString &q, int limit,
                 Callback onSuccess, ErrorCallback onError);

    /** 配方详情 */
    void getFormulaDetail(int id, const QString &pigmentCodeSystem,
                          Callback onSuccess, ErrorCallback onError);

private:
    explicit FormulaApi(QObject *parent = nullptr);
};
