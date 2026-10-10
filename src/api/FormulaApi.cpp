/* ============================================================
   FormulaApi.cpp —— 配方查询接口实现
   ============================================================ */

#include "FormulaApi.h"
#include "HttpClient.h"

#include <QJsonArray>

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

void FormulaApi::getPigmentVendorCodes(const QStringList &pigmentCodes,
                                        Callback onSuccess, ErrorCallback onError)
{
    QJsonArray codes;
    QStringList seen;
    for (const auto &raw : pigmentCodes) {
        const QString code = raw.trimmed();
        if (code.isEmpty() || seen.contains(code)) continue;
        seen.append(code);
        codes.append(code);
    }
    if (codes.isEmpty()) return;

    QJsonObject body;
    body[QStringLiteral("pigmentCodes")] = codes;
    HttpClient::instance().post(QStringLiteral("/colorcr/pigment/vendor-codes"), body,
                                onSuccess, onError);
}

void FormulaApi::getBrands(int pageNum, int pageSize,
                           Callback onSuccess, ErrorCallback onError)
{
    QJsonObject params;
    params[QStringLiteral("pageNum")] = pageNum;
    params[QStringLiteral("pageSize")] = pageSize;
    HttpClient::instance().get(QStringLiteral("/colorcr/brand/list"), params, onSuccess, onError);
}

void FormulaApi::getSeriesByBrand(const QString &brandId,
                                  Callback onSuccess, ErrorCallback onError)
{
    QString path = QStringLiteral("/colorcr/brand/") + brandId + QStringLiteral("/series/list");
    QJsonObject params;
    HttpClient::instance().get(path, params, onSuccess, onError);
}

void FormulaApi::getModelsBySeries(const QString &seriesId, int pageNum, int pageSize,
                                   Callback onSuccess, ErrorCallback onError)
{
    QJsonObject params;
    params[QStringLiteral("seriesId")] = seriesId;
    params[QStringLiteral("pageNum")] = pageNum;
    params[QStringLiteral("pageSize")] = pageSize;
    HttpClient::instance().get(QStringLiteral("/colorcr/model/list"), params, onSuccess, onError);
}

void FormulaApi::getFormulasByModel(const QString &modelId, int pageNum, int pageSize,
                                    Callback onSuccess, ErrorCallback onError)
{
    QJsonObject params;
    params[QStringLiteral("modelId")] = modelId;
    params[QStringLiteral("pageNum")] = pageNum;
    params[QStringLiteral("pageSize")] = pageSize;
    HttpClient::instance().get(QStringLiteral("/colorcr/formula/list"), params, onSuccess, onError);
}

void FormulaApi::uploadUserFormula(const QJsonObject &body,
                                   Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().post(QStringLiteral("/colorcr/user-formula/upload"),
                                body, onSuccess, onError);
}
