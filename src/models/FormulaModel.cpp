/* ============================================================
   FormulaModel.cpp —— 配方数据结构实现
   ============================================================ */

#include "FormulaModel.h"
#include "api/HttpClient.h"
#include <QJsonArray>

namespace FormulaModel {

QString rgbToHex(int r, int g, int b)
{
    auto clamp = [](int v) { return qBound(0, v, 255); };
    return QStringLiteral("#%1%2%3")
        .arg(clamp(r), 2, 16, QLatin1Char('0'))
        .arg(clamp(g), 2, 16, QLatin1Char('0'))
        .arg(clamp(b), 2, 16, QLatin1Char('0'));
}

QString getResourceUrl(const QString &url)
{
    return HttpClient::resourceUrl(url);
}

QString pickRowImage(const QJsonObject &row)
{
    if (row.contains(QStringLiteral("imageUrl"))) {
        auto v = row.value(QStringLiteral("imageUrl"));
        if (v.isString() && !v.toString().isEmpty()) return v.toString();
    }
    if (row.contains(QStringLiteral("cover"))) {
        auto v = row.value(QStringLiteral("cover"));
        if (v.isString() && !v.toString().isEmpty()) return v.toString();
    }
    if (row.contains(QStringLiteral("previewImages"))) {
        auto arr = row.value(QStringLiteral("previewImages")).toArray();
        if (!arr.isEmpty()) {
            auto first = arr.first();
            if (first.isString()) return first.toString();
            auto obj = first.toObject();
            for (const auto &key : {QStringLiteral("url"), QStringLiteral("imageUrl"),
                                    QStringLiteral("imgUrl"), QStringLiteral("thumb")}) {
                if (obj.contains(key) && obj.value(key).isString())
                    return obj.value(key).toString();
            }
        }
    }
    // 嵌套 formula 对象
    if (row.contains(QStringLiteral("formula"))) {
        auto inner = row.value(QStringLiteral("formula")).toObject();
        if (inner.contains(QStringLiteral("imageUrl")))
            return inner.value(QStringLiteral("imageUrl")).toString();
        if (inner.contains(QStringLiteral("cover")))
            return inner.value(QStringLiteral("cover")).toString();
    }
    return {};
}

FormulaRow normalizeFormulaRow(const QJsonObject &row)
{
    FormulaRow r;
    r.id         = row.value(QStringLiteral("id")).toInt(
                   row.value(QStringLiteral("formulaId")).toInt());
    r.formulaId  = row.value(QStringLiteral("formulaId")).toInt(
                   row.value(QStringLiteral("id")).toInt());
    r.sampleId   = row.value(QStringLiteral("sampleId")).toString();
    r.formulaName = row.value(QStringLiteral("formulaName")).toString();
    r.colorName  = row.value(QStringLiteral("colorName")).toString();
    r.colorNameEn = row.value(QStringLiteral("colorNameEn")).toString();
    r.colorCode  = row.value(QStringLiteral("colorCode")).toString();
    r.colorBias  = row.value(QStringLiteral("colorBias")).toString();
    r.brandName  = row.value(QStringLiteral("brandName")).toString();
    r.seriesName = row.value(QStringLiteral("seriesName")).toString();
    r.modelName  = row.value(QStringLiteral("modelName")).toString();
    r.year       = row.value(QStringLiteral("year")).toString();

    QString imgUrl = pickRowImage(row);
    r.cover     = getResourceUrl(imgUrl);
    r.imageUrl  = getResourceUrl(imgUrl);
    return r;
}

QJsonArray pickFormulaRows(const QJsonObject &res)
{
    if (res.contains(QStringLiteral("rows")))
        return res.value(QStringLiteral("rows")).toArray();
    if (res.contains(QStringLiteral("data"))) {
        auto data = res.value(QStringLiteral("data"));
        if (data.isArray()) return data.toArray();
        if (data.isObject()) {
            auto obj = data.toObject();
            if (obj.contains(QStringLiteral("rows")))
                return obj.value(QStringLiteral("rows")).toArray();
        }
    }
    return {};
}

FormulaDetail parseFormulaDetail(const QJsonObject &data)
{
    FormulaDetail detail;
    detail.formula = data.value(QStringLiteral("formula")).toObject();

    // 纹理
    auto tex = data.value(QStringLiteral("texture")).toObject();
    if (!tex.isEmpty()) {
        detail.texture.hasData = true;
        detail.texture.coarseness = tex.value(QStringLiteral("coarseness")).toDouble();
        detail.texture.sparkleR15As15 = tex.value(QStringLiteral("sparkleGradeR15As15")).toVariant().toString();
        detail.texture.sparkleR15As80 = tex.value(QStringLiteral("sparkleGradeR15As80")).toVariant().toString();
    }

    // 色母明细
    auto rows = data.value(QStringLiteral("detailRows")).toArray();
    for (const auto &v : rows) {
        auto obj = v.toObject();
        PigmentDetailRow row;
        row.layer = obj.value(QStringLiteral("layer")).toInt(1);
        row.pigmentCode = obj.value(QStringLiteral("pigmentCode")).toString();
        row.pigmentName = obj.value(QStringLiteral("pigmentName")).toString();
        row.weight = obj.value(QStringLiteral("weight")).toDouble();

        auto rgb = obj.value(QStringLiteral("rgb")).toObject();
        if (!rgb.isEmpty()) {
            row.rgb.r = rgb.value(QStringLiteral("r")).toInt();
            row.rgb.g = rgb.value(QStringLiteral("g")).toInt();
            row.rgb.b = rgb.value(QStringLiteral("b")).toInt();
            row.rgb.hex = rgb.value(QStringLiteral("hex")).toString();
            if (row.rgb.hex.isEmpty())
                row.rgb.hex = rgbToHex(row.rgb.r, row.rgb.g, row.rgb.b);
        }
        detail.detailRows.append(row);
    }

    detail.detailTotal = data.value(QStringLiteral("detailTotal")).toInt();
    return detail;
}

} // namespace FormulaModel
