/* ============================================================
   FormulaExport.h —— 配方导出（4 种格式）
   - XML (ColorLink) / DAT (recipe.dat) / REZ (recipe.rez) / TXT (RemoteFormula)
   ============================================================ */

#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>

struct ExportItem {
    QString colorant;
    double amount = 0.0;
};

struct ExportOptions {
    int layerNo = 1;
    QJsonObject meta;          // 配方头信息
    QList<ExportItem> items;   // 色母明细
    QString unit = QStringLiteral("g");
    double quantity = 0;
    QString assortmentId;
    QString sampleId;
    QString filename;
};

namespace FormulaExport {

/** 支持的格式 */
enum class Format { Xml, Dat, Rez, Txt };

/** 生成 ColorLink XML */
QString buildColorLinkXml(const ExportOptions &opts);

/** 生成 recipe.dat */
QString buildRecipeDat(const ExportOptions &opts);

/** 生成 recipe.rez */
QString buildRecipeRez(const ExportOptions &opts);

/** 生成 RemoteFormulaOutput.txt */
QString buildRemoteFormulaTxt(const ExportOptions &opts);

/** 按格式生成内容 */
QString buildContent(Format fmt, const ExportOptions &opts);

/** 获取格式对应的文件扩展名 */
QString fileExtension(Format fmt);

/** 获取格式对应的文件过滤器（用于 QFileDialog） */
QString fileFilter(Format fmt);

/** 保存文件到指定路径 */
bool saveToFile(const QString &filePath, const QString &content);

} // namespace FormulaExport
