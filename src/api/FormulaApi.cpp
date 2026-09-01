/* ============================================================
   FormulaApi.cpp —— 配方查询接口实现
   ============================================================ */

#include "FormulaApi.h"
#include "HttpClient.h"

static const QString PAINT_SERIES_SCOPE = QStringLiteral("A");

FormulaApi::FormulaApi(QObject *parent) : QObject(parent) {}

FormulaApi &FormulaApi::instance()
{
    static FormulaApi inst;
    return inst;
}

void FormulaApi::searchFormula(const QString &keyword, int pageNum, int pageSize,
                               Callback onSuccess, ErrorCallback onError)
{
    QJsonObject params;
    params[QStringLiteral("keyword")]           = keyword;
    params[QStringLiteral("pageNum")]           = pageNum;
    params[QStringLiteral("pageSize")]          = pageSize;
    params[QStringLiteral("paintSeriesScope")]  = PAINT_SERIES_SCOPE;

    HttpClient::instance().get(QStringLiteral("/colorcr/formula/app/search"), params, onSuccess, onError);
}

void FormulaApi::suggest(const QString &q, int limit,
                         Callback onSuccess, ErrorCallback onError)
{
    QJsonObject params;
    params[QStringLiteral("q")]                 = q;
    params[QStringLiteral("limit")]             = limit;
    params[QStringLiteral("paintSeriesScope")]  = PAINT_SERIES_SCOPE;

    HttpClient::instance().get(QStringLiteral("/colorcr/formula/app/search/suggest"), params, onSuccess, onError);
}

void FormulaApi::getFormulaDetail(int id, const QString &pigmentCodeSystem,
                                  Callback onSuccess, ErrorCallback onError)
{
    QJsonObject params;
    if (!pigmentCodeSystem.isEmpty())
        params[QStringLiteral("pigmentCodeSystem")] = pigmentCodeSystem;

    QString path = QStringLiteral("/colorcr/formula/") + QString::number(id);
    HttpClient::instance().get(path, params, onSuccess, onError);
}
