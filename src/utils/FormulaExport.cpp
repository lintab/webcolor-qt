/* ============================================================
   FormulaExport.cpp —— 配方导出实现
   ============================================================ */

#include "FormulaExport.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>

namespace FormulaExport {

// ---------- 工具函数 ----------

static QString padRight(const QString &str, int width)
{
    QString s = str;
    if (s.length() >= width) return s.left(width);
    return s + QString(width - s.length(), QLatin1Char(' '));
}

static QString formatFixed(double n, int decimals = 5)
{
    return QString::number(n, 'f', decimals);
}

static QString formatTxtAmount(double n)
{
    QString s = QString::number(n, 'f', 4);
    // 去尾零
    while (s.endsWith(QLatin1Char('0'))) s.chop(1);
    if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    return s.isEmpty() ? QStringLiteral("0") : s;
}

static QString escapeXml(const QString &value)
{
    QString s = value;
    s.replace(QLatin1Char('&'),  QStringLiteral("&amp;"));
    s.replace(QLatin1Char('<'),  QStringLiteral("&lt;"));
    s.replace(QLatin1Char('>'),  QStringLiteral("&gt;"));
    s.replace(QLatin1Char('"'),  QStringLiteral("&quot;"));
    s.replace(QLatin1Char('\''), QStringLiteral("&apos;"));
    return s;
}

static QString sanitizeFilename(const QString &name)
{
    QString n = name.isEmpty() ? QStringLiteral("recipe") : name;
    n.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|]+)")), QStringLiteral("_"));
    return n;
}

static QString nowRezDate()
{
    QDateTime dt = QDateTime::currentDateTime();
    return dt.toString(QStringLiteral("dd-MM-yyyy\thh:mm:ss"));
}

static QString nowTxtDate()
{
    QDateTime dt = QDateTime::currentDateTime();
    return dt.toString(QStringLiteral("MM/dd/yyyy hh:mm:ss"));
}

// ---------- XML ----------

QString buildColorLinkXml(const ExportOptions &opts)
{
    QString version = opts.meta.value(QStringLiteral("version")).toString(QStringLiteral("2017-12-1"));
    QString amount  = opts.meta.contains(QStringLiteral("amount"))
                      ? QString::number(opts.meta.value(QStringLiteral("amount")).toDouble())
                      : QStringLiteral("1");
    QString brand   = opts.meta.value(QStringLiteral("brand")).toString();
    QString product = opts.meta.value(QStringLiteral("product")).toString();
    QString manufacturer = opts.meta.value(QStringLiteral("manufacturer")).toString();
    QString model   = opts.meta.value(QStringLiteral("model")).toString();
    QString desc    = opts.meta.value(QStringLiteral("description")).toString();
    QString innerCode = opts.meta.value(QStringLiteral("innerColorCode")).toString();
    QString colorGroup = opts.meta.value(QStringLiteral("colorGroup")).toString();
    QString stdColor = opts.meta.value(QStringLiteral("standardColor")).toString();
    QString varCode  = opts.meta.value(QStringLiteral("variantCode")).toString();
    QString barrels  = opts.meta.contains(QStringLiteral("barrels"))
                       ? QString::number(opts.meta.value(QStringLiteral("barrels")).toInt())
                       : QStringLiteral("1");
    QString isWgh    = opts.meta.contains(QStringLiteral("isWgh"))
                       ? QString::number(opts.meta.value(QStringLiteral("isWgh")).toInt())
                       : QStringLiteral("1");

    QStringList lines;
    lines << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
    lines << QStringLiteral("<COLORFORMULA>");
    lines << QStringLiteral("\t<FORMULA>");
    lines << QStringLiteral("\t\t<VERSION>%1</VERSION>").arg(escapeXml(version));
    lines << QStringLiteral("\t\t<AMOUNT>%1</AMOUNT>").arg(escapeXml(amount));
    lines << QStringLiteral("\t\t<LAYERNO>%1</LAYERNO>").arg(opts.layerNo);
    lines << QStringLiteral("\t\t<BRAND>%1</BRAND>").arg(escapeXml(brand));
    lines << QStringLiteral("\t\t<PRODUCT>%1</PRODUCT>").arg(escapeXml(product));
    lines << QStringLiteral("\t\t<MANUFACTURER>%1</MANUFACTURER>").arg(escapeXml(manufacturer));
    lines << QStringLiteral("\t\t<MODELS>%1</MODELS>").arg(escapeXml(model));
    lines << QStringLiteral("\t\t<DESCRIPTION>%1</DESCRIPTION>").arg(escapeXml(desc));
    lines << QStringLiteral("\t\t<INNERCOLORCODE>%1</INNERCOLORCODE>").arg(escapeXml(innerCode));
    lines << QStringLiteral("\t\t<COLORGROUP>%1</COLORGROUP>").arg(escapeXml(colorGroup));
    lines << QStringLiteral("\t\t<STANDARDCOLOR>%1</STANDARDCOLOR>").arg(escapeXml(stdColor));
    lines << QStringLiteral("\t\t<VARIANTCODE>%1</VARIANTCODE>").arg(escapeXml(varCode));
    lines << QStringLiteral("\t\t<BARRELS>%1</BARRELS>").arg(escapeXml(barrels));
    lines << QStringLiteral("\t\t<UNIT>%1</UNIT>").arg(escapeXml(opts.unit));
    lines << QStringLiteral("\t\t<ISWGH>%1</ISWGH>").arg(escapeXml(isWgh));
    lines << QStringLiteral("\t</FORMULA>");
    lines << QStringLiteral("\t<FORMULAITEMS>");

    for (const auto &it : opts.items) {
        if (it.colorant.isEmpty() || it.amount <= 0) continue;
        lines << QStringLiteral("\t\t<FORMULAITEM>");
        lines << QStringLiteral("\t\t\t<COLORANT>%1</COLORANT>").arg(escapeXml(it.colorant));
        lines << QStringLiteral("\t\t\t<AMOUNT>%1</AMOUNT>").arg(formatTxtAmount(it.amount));
        lines << QStringLiteral("\t\t</FORMULAITEM>");
    }

    lines << QStringLiteral("\t</FORMULAITEMS>");
    lines << QStringLiteral("</COLORFORMULA>");
    return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

// ---------- DAT ----------

QString buildRecipeDat(const ExportOptions &opts)
{
    QList<ExportItem> list;
    for (const auto &it : opts.items)
        if (!it.colorant.isEmpty() && it.amount > 0) list.append(it);

    double total = opts.quantity > 0 ? opts.quantity : 0;
    if (total <= 0) {
        for (const auto &it : list) total += it.amount;
    }

    QStringList lines;
    lines << QStringLiteral("1");
    lines << padRight(opts.assortmentId, 49);
    lines << QStringLiteral("-");
    lines << padRight(opts.sampleId, 49);
    lines << QStringLiteral("STAMM");
    lines << QStringLiteral("G");
    lines << QString::number(list.size());
    lines << formatFixed(total, 5);

    for (const auto &it : list) {
        lines << it.colorant.trimmed();
        lines << formatFixed(it.amount, 5);
        lines << QStringLiteral("--");
    }
    lines << QString();
    return lines.join(QStringLiteral("\r\n"));
}

// ---------- REZ ----------

QString buildRecipeRez(const ExportOptions &opts)
{
    QList<ExportItem> list;
    for (const auto &it : opts.items)
        if (!it.colorant.isEmpty() && it.amount > 0) list.append(it);

    double total = opts.quantity > 0 ? opts.quantity : 0;
    if (total <= 0) {
        for (const auto &it : list) total += it.amount;
        if (total <= 0) total = 1;
    }

    QStringList lines;
    lines << QStringLiteral("[Recipe]");
    lines << QStringLiteral("ProtocolVersion=\t1");
    lines << QStringLiteral("Type=\tBasic");
    lines << QStringLiteral("ID=\t%1").arg(opts.sampleId);
    lines << QStringLiteral("Info=\t");
    lines << QStringLiteral("Date=\t%1").arg(nowRezDate());
    lines << QStringLiteral("UseLeftovers=\tNO");
    lines << QStringLiteral("Quantity=\t%1").arg(formatFixed(total, 5));
    lines << QStringLiteral("AddInkQuantityFactor=\t");
    lines << QStringLiteral("IFT=\t1.00000");
    lines << QStringLiteral("WeightPerArea=\t1.00000");
    lines << QString();
    lines << QStringLiteral("[Assortment]");
    lines << QStringLiteral("ID=\t%1").arg(opts.assortmentId);
    lines << QStringLiteral("Info=\t");
    lines << QString();

    for (int i = 0; i < list.size(); ++i) {
        double part = list[i].amount / total;
        lines << QStringLiteral("[Component%1]").arg(i + 1);
        lines << QStringLiteral("ID=\t%1").arg(list[i].colorant.trimmed());
        lines << QStringLiteral("Info=\t");
        lines << QStringLiteral("Part=\t   %1").arg(formatFixed(part, 5));
        lines << QString();
    }
    return lines.join(QStringLiteral("\r\n"));
}

// ---------- TXT ----------

QString buildRemoteFormulaTxt(const ExportOptions &opts)
{
    QList<ExportItem> list;
    for (const auto &it : opts.items)
        if (!it.colorant.isEmpty() && it.amount > 0) list.append(it);

    QString id = opts.sampleId.trimmed().toUpper();
    QStringList lines;
    lines << id;
    lines << opts.sampleId.trimmed();
    lines << nowTxtDate();
    lines << QString();

    for (int i = 0; i < list.size(); ++i) {
        QString code = padRight(list[i].colorant.trimmed(), 15);
        QString amt  = QString("%1").arg(formatTxtAmount(list[i].amount)).rightJustified(7, QLatin1Char(' '));
        lines << QStringLiteral("%1 ,%2, %3, ").arg(i).arg(code, amt);
    }
    return lines.join(QStringLiteral("\r\n")) + QStringLiteral("\r\n");
}

// ---------- 统一入口 ----------

QString buildContent(Format fmt, const ExportOptions &opts)
{
    switch (fmt) {
    case Format::Xml: return buildColorLinkXml(opts);
    case Format::Dat: return buildRecipeDat(opts);
    case Format::Rez: return buildRecipeRez(opts);
    case Format::Txt: return buildRemoteFormulaTxt(opts);
    }
    return {};
}

QString fileExtension(Format fmt)
{
    switch (fmt) {
    case Format::Xml: return QStringLiteral("xml");
    case Format::Dat: return QStringLiteral("dat");
    case Format::Rez: return QStringLiteral("rez");
    case Format::Txt: return QStringLiteral("txt");
    }
    return QStringLiteral("txt");
}

QString fileFilter(Format fmt)
{
    switch (fmt) {
    case Format::Xml: return QStringLiteral("XML 文件 (*.xml)");
    case Format::Dat: return QStringLiteral("DAT 文件 (*.dat)");
    case Format::Rez: return QStringLiteral("REZ 文件 (*.rez)");
    case Format::Txt: return QStringLiteral("TXT 文件 (*.txt)");
    }
    return QStringLiteral("所有文件 (*)");
}

bool saveToFile(const QString &filePath, const QString &content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << content;
    file.close();
    return true;
}

} // namespace FormulaExport
