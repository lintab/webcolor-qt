/* ============================================================
   FormulaModel.h —— 配方数据结构与规范化
   - 对应 JS 版 normalizeFormulaRow / pickFormulaRows
   ============================================================ */

#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include "utils/I18nManager.h"

/** 配方列表行（规范化后的统一结构） */
struct FormulaRow {
    int id = 0;
    int formulaId = 0;
    QString sampleId;
    QString formulaName;
    QString colorName;
    QString colorNameEn;
    QString colorCode;
    QString colorBias;
    QString brandName;
    QString brandNameEn;
    QString seriesName;
    QString seriesNameEn;
    QString modelName;
    QString modelNameEn;
    QString year;
    QString cover;        // 图片 URL（已拼接完整地址）
    QString imageUrl;

    /** 颜色名称：按界面语言取中/英文 */
    QString displayName() const {
        return Tn(colorName, colorNameEn);
    }

    /** 车辆信息拼接 */
    QString vehicleInfo() const {
        QStringList parts;
        const QString brand = Tn(brandName, brandNameEn);
        const QString series = Tn(seriesName, seriesNameEn);
        const QString model = Tn(modelName, modelNameEn);
        if (!brand.isEmpty())   parts << brand;
        if (!series.isEmpty())  parts << series;
        if (!model.isEmpty())   parts << model;
        return parts.join(QStringLiteral(" · "));
    }
};

/** 色母明细行 */
struct PigmentDetailRow {
    int layer = 1;
    QString pigmentCode;
    QString pigmentName;
    QString pigmentNameEn;
    double weight = 0.0;
    struct { int r = 0; int g = 0; int b = 0; QString hex; } rgb;
};

/** 纹理参数 */
struct TextureData {
    double coarseness = 0;
    QString sparkleR15As15;
    QString sparkleR15As80;
    bool hasData = false;
};

/** 配方详情数据 */
struct FormulaDetail {
    QJsonObject formula;       // 原始 formula JSON（供导出用）
    TextureData texture;
    QList<PigmentDetailRow> detailRows;
    int detailTotal = 0;
};

namespace FormulaModel {

/** 从行数据中提取配方展示图 URL */
QString pickRowImage(const QJsonObject &row);

/** 规范化配方列表行 */
FormulaRow normalizeFormulaRow(const QJsonObject &row);

/** 从分页响应中提取 rows 数组 */
QJsonArray pickFormulaRows(const QJsonObject &res);

/** 解析配方详情 */
FormulaDetail parseFormulaDetail(const QJsonObject &data);

/** RGB 转 hex */
QString rgbToHex(int r, int g, int b);

/** 资源 URL 补全 */
QString getResourceUrl(const QString &url);

} // namespace FormulaModel
