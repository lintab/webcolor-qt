/* ============================================================
   SpectralCurveChart.cpp —— 光谱曲线图（QPainter 自绘）
   ============================================================ */

#include "SpectralCurveChart.h"

#include "utils/I18nManager.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

// 与前端 scChartGrid({left:52,right:16}) 及 scChartTheme 配色一致
constexpr int kMarginLeft = 52;
constexpr int kMarginRight = 16;
constexpr int kMarginTop = 36;
constexpr int kMarginBottom = 36;

const QColor kSplitLine(QStringLiteral("#e5e7eb"));
const QColor kAxisLine(QStringLiteral("#cbd5e1"));
const QColor kAxisLabel(QStringLiteral("#64748b"));
const QColor kPointerLine(QStringLiteral("#94a3b8"));
const QColor kTooltipBorder(QStringLiteral("#e2e8f0"));
const QColor kTooltipText(QStringLiteral("#334155"));
const QColor kEmptyText(QStringLiteral("#909399"));

// 图例：颜色已经表示角度，来源只能靠线型区分，所以样本用中性色
constexpr int kLegendSampleW = 22;
constexpr int kLegendLineH = 16;
constexpr int kLegendGap = 16;
constexpr int kLegendTop = 6;
const QColor kLegendColor(QStringLiteral("#475569"));

inline bool isFinite(double v) { return std::isfinite(v); }

/** 去掉角度码前缀 r（前端 normalizeScGeometry） */
QString normalizeGeometry(const QString &code)
{
    const QString s = code.trimmed();
    return s.startsWith(QLatin1Char('r')) ? s.mid(1) : s;
}

/** Heckbert nice number */
double niceNum(double range, bool roundUp)
{
    if (!(range > 0)) return 1.0;
    const double exp = std::floor(std::log10(range));
    const double mag = std::pow(10.0, exp);
    const double f = range / mag;
    double nf = 10.0;
    if (roundUp) {
        if (f < 1.5) nf = 1.0;
        else if (f < 3.0) nf = 2.0;
        else if (f < 7.0) nf = 5.0;
    } else {
        if (f <= 1.0) nf = 1.0;
        else if (f <= 2.0) nf = 2.0;
        else if (f <= 5.0) nf = 5.0;
    }
    return nf * mag;
}

int decimalsForStep(double step)
{
    if (!(step > 0)) return 0;
    return std::max(0, static_cast<int>(std::ceil(-std::log10(step))) + 1);
}

QString formatTickValue(double v, int decimals)
{
    QString s = QString::number(v, 'f', decimals);
    if (decimals > 0 && s.contains(QLatin1Char('.'))) {
        while (s.endsWith(QLatin1Char('0'))) s.chop(1);
        if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    }
    return s;
}

} // namespace

SpectralCurveChart::SpectralCurveChart(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumHeight(200);
    setMouseTracking(true);
    // m_emptyText 留空表示未外部覆盖：绘制时按当前语言取 sc_no_data
}

void SpectralCurveChart::setSeries(const QVector<Series> &series)
{
    m_series = series;
    m_hoverIndex = -1;
    rebuildCache();
    update();
}

void SpectralCurveChart::setMode(Mode mode)
{
    if (m_mode == mode) return;
    m_mode = mode;
    m_hoverIndex = -1;
    rebuildCache();
    update();
}

void SpectralCurveChart::setEmptyText(const QString &text)
{
    // 传空串表示恢复默认空数据提示（绘制时按当前语言解析）
    m_emptyText = text;
    update();
}

QVector<double> SpectralCurveChart::collectWavelengths() const
{
    QVector<double> out;
    for (const auto &s : m_series) {
        for (auto it = s.values.constBegin(); it != s.values.constEnd(); ++it) {
            const double wl = it.key();
            if (!isFinite(wl) || wl < 380.0 || wl > 780.0) continue;
            out.append(wl);
        }
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    if (out.isEmpty()) {
        for (int wl = 400; wl <= 700; wl += 10) out.append(wl);
        if (m_series.isEmpty()) out.clear();
    }
    return out;
}

void SpectralCurveChart::rebuildCache()
{
    m_wavelengths = collectWavelengths();

    QMap<QString, QMap<double, double>> targets;
    for (const auto &s : m_series) {
        if (s.sourceKey.compare(QLatin1String("target"), Qt::CaseInsensitive) == 0)
            targets.insert(normalizeGeometry(s.geometry), s.values);
    }

    m_drawables.clear();
    m_drawables.reserve(m_series.size());
    for (const auto &s : m_series) {
        DrawableSeries d;
        d.src = s;
        d.isTarget = s.sourceKey.compare(QLatin1String("target"), Qt::CaseInsensitive) == 0;
        const QMap<double, double> ref = targets.value(normalizeGeometry(s.geometry));
        d.values.reserve(m_wavelengths.size());
        for (double wl : m_wavelengths) {
            double v = std::numeric_limits<double>::quiet_NaN();
            auto it = s.values.constFind(wl);
            if (it != s.values.constEnd() && isFinite(it.value())) {
                v = it.value();
                if (m_mode == Mode::DeltaReflectance) {
                    auto rit = ref.constFind(wl);
                    v = (rit != ref.constEnd() && isFinite(rit.value()))
                            ? v - rit.value()
                            : std::numeric_limits<double>::quiet_NaN();
                }
            }
            d.values.append(v);
        }
        m_drawables.append(d);
    }

    // y 轴范围：R 模式下非负数据从 0 起（ECharts scale:false 语义）
    double dataMin = std::numeric_limits<double>::quiet_NaN();
    double dataMax = std::numeric_limits<double>::quiet_NaN();
    for (const auto &d : m_drawables) {
        if (m_mode == Mode::DeltaReflectance && d.isTarget) continue;
        for (double v : d.values) {
            if (!isFinite(v)) continue;
            dataMin = isFinite(dataMin) ? std::min(dataMin, v) : v;
            dataMax = isFinite(dataMax) ? std::max(dataMax, v) : v;
        }
    }

    if (!isFinite(dataMin) || !isFinite(dataMax)) {
        m_yMin = (m_mode == Mode::DeltaReflectance) ? -10.0 : 0.0;
        m_yMax = 100.0;
        m_yStep = 20.0;
        return;
    }

    double lo = dataMin;
    double hi = dataMax;
    if (m_mode == Mode::Reflectance && lo >= 0) lo = 0;
    if (hi == lo) hi = lo + 1.0;

    const double rawStep = (hi - lo) / 5.0;
    double step = niceNum(rawStep, true);
    double niceLo = std::floor(lo / step) * step;
    double niceHi = std::ceil(hi / step) * step;
    if (niceHi - niceLo > step * 20.0) step = niceNum((hi - lo) / 5.0, false);

    m_yMin = niceLo;
    m_yMax = niceHi;
    m_yStep = step;
}

QRectF SpectralCurveChart::plotRect() const
{
    const int top = kMarginTop + m_legendExtraTop;
    return QRectF(kMarginLeft, top,
                  std::max(1, width() - kMarginLeft - kMarginRight),
                  std::max(1, height() - top - kMarginBottom));
}

int SpectralCurveChart::categoryX(int index) const
{
    const QRectF r = plotRect();
    if (m_wavelengths.size() <= 1) return static_cast<int>(r.center().x());
    const double spacing = r.width() / (m_wavelengths.size() - 1);
    return static_cast<int>(std::lround(r.left() + index * spacing));
}

int SpectralCurveChart::hitIndex(const QPoint &pos) const
{
    const QRectF r = plotRect();
    if (m_wavelengths.isEmpty() || !r.contains(pos)) return -1;
    if (m_wavelengths.size() == 1) return 0;
    const double spacing = r.width() / (m_wavelengths.size() - 1);
    const int idx = static_cast<int>(std::lround((pos.x() - r.left()) / spacing));
    return std::clamp(idx, 0, static_cast<int>(m_wavelengths.size()) - 1);
}

QSize SpectralCurveChart::sizeHint() const
{
    return QSize(640, 280);
}

QSize SpectralCurveChart::minimumSizeHint() const
{
    return QSize(320, 200);
}

void SpectralCurveChart::mouseMoveEvent(QMouseEvent *event)
{
    const int idx = hitIndex(event->pos());
    if (idx != m_hoverIndex || event->pos() != m_hoverPos) {
        m_hoverIndex = idx;
        m_hoverPos = event->pos();
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void SpectralCurveChart::leaveEvent(QEvent *event)
{
    if (m_hoverIndex != -1) {
        m_hoverIndex = -1;
        update();
    }
    QWidget::leaveEvent(event);
}

void SpectralCurveChart::drawEmptyState(QPainter &p)
{
    p.setPen(kEmptyText);
    QFont f = p.font();
    f.setPixelSize(13);
    p.setFont(f);
    p.drawText(rect(), Qt::AlignCenter,
               m_emptyText.isEmpty() ? T("sc_no_data") : m_emptyText);
}

void SpectralCurveChart::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), Qt::white);

    bool hasData = false;
    for (const auto &d : m_drawables) {
        if (m_mode == Mode::DeltaReflectance && d.isTarget) continue;
        for (double v : d.values) {
            if (isFinite(v)) { hasData = true; break; }
        }
        if (hasData) break;
    }
    if (!hasData) {
        drawEmptyState(p);
        return;
    }

    const QVector<LegendEntry> legend = collectLegendEntries();

    QFont legendFont = p.font();
    legendFont.setPixelSize(12);
    const QFontMetrics lfm(legendFont);
    const double legendAvail = std::max(60.0,
                                        static_cast<double>(width() - kMarginLeft - kMarginRight));
    // 一行图例用现成的顶部留白，多出的行把绘图区往下推
    m_legendExtraTop = std::max(0, legendRowCount(legend, lfm, legendAvail) - 1) * kLegendLineH;

    const QRectF plot = plotRect();

    QFont axisFont = p.font();
    axisFont.setPixelSize(12);
    p.setFont(axisFont);
    const QFontMetrics fm(axisFont);

    // ── 横向分隔线 + y 轴标签（百分比）──
    const int yDecimals = decimalsForStep(m_yStep);
    p.setPen(QPen(kSplitLine, 1));
    for (double v = m_yMin; v <= m_yMax + m_yStep * 0.5; v += m_yStep) {
        const double ratio = (v - m_yMin) / (m_yMax - m_yMin);
        const int y = static_cast<int>(std::lround(plot.bottom() - ratio * plot.height()));
        p.setPen(QPen(kSplitLine, 1));
        p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        p.setPen(kAxisLabel);
        p.drawText(QRectF(0, y - 8, kMarginLeft - 8, 16),
                   Qt::AlignRight | Qt::AlignVCenter,
                   formatTickValue(v, yDecimals) + QStringLiteral("%"));
    }

    // ── x 轴：轴线、刻度、标签（按需抽稀）──
    p.setPen(QPen(kAxisLine, 1));
    p.drawLine(QPointF(plot.left(), plot.bottom()), QPointF(plot.right(), plot.bottom()));

    const int n = m_wavelengths.size();
    int stride = 1;
    if (n > 1) {
        const double spacing = plot.width() / (n - 1);
        const int labelW = fm.horizontalAdvance(QStringLiteral("000")) + 10;
        stride = std::max(1, static_cast<int>(std::ceil(labelW / std::max(1.0, spacing))));
    }
    for (int i = 0; i < n; ++i) {
        const int x = categoryX(i);
        p.setPen(QPen(kAxisLine, 1));
        p.drawLine(QPointF(x, plot.bottom()), QPointF(x, plot.bottom() + 4));
        if (i % stride == 0) {
            p.setPen(kAxisLabel);
            const QString text = QString::number(static_cast<int>(std::lround(m_wavelengths[i])));
            // 首末标签向内对齐，避免超出绘图区
            if (i == 0) {
                p.drawText(QRectF(x, plot.bottom() + 5, 48, 16),
                           Qt::AlignLeft | Qt::AlignTop, text);
            } else if (i == n - 1) {
                p.drawText(QRectF(x - 48, plot.bottom() + 5, 48, 16),
                           Qt::AlignRight | Qt::AlignTop, text);
            } else {
                p.drawText(QRectF(x - 24, plot.bottom() + 5, 48, 16),
                           Qt::AlignHCenter | Qt::AlignTop, text);
            }
        }
    }
    // 轴名「nm」（ECharts nameLocation: end，画在标签下一行）
    p.setPen(kAxisLabel);
    p.drawText(QRectF(plot.right() - 40, plot.bottom() + 21, 40, 14),
               Qt::AlignRight | Qt::AlignTop, T("chart_nm"));

    // ── 曲线 ──
    p.save();
    p.setClipRect(plot.adjusted(0, -2, 1, 2));
    for (const auto &d : m_drawables) {
        if (m_mode == Mode::DeltaReflectance && d.isTarget) continue;

        QPainterPath path;
        bool started = false;
        for (int i = 0; i < d.values.size() && i < m_wavelengths.size(); ++i) {
            const double v = d.values[i];
            if (!isFinite(v)) { started = false; continue; }
            const double x = categoryX(i);
            const double y = plot.bottom()
                             - (v - m_yMin) / (m_yMax - m_yMin) * plot.height();
            if (!started) { path.moveTo(x, y); started = true; }
            else path.lineTo(x, y);
        }
        QPen pen(d.src.color, 2);
        pen.setStyle(d.src.lineStyle);
        pen.setJoinStyle(Qt::RoundJoin);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        // 数据点（ECharts symbol: circle, size 6）
        p.setPen(Qt::NoPen);
        p.setBrush(d.src.color);
        for (int i = 0; i < d.values.size() && i < m_wavelengths.size(); ++i) {
            const double v = d.values[i];
            if (!isFinite(v)) continue;
            const double x = categoryX(i);
            const double y = plot.bottom()
                             - (v - m_yMin) / (m_yMax - m_yMin) * plot.height();
            p.drawEllipse(QPointF(x, y), 3.0, 3.0);
        }
    }
    p.restore();

    // ── 来源图例（初始 / AC / AC-Edited 等，线型对应曲线）──
    p.setFont(legendFont);
    drawLegend(p, legend, lfm, legendAvail);

    // ── 悬停轴指针 + 悬浮提示 ──
    if (m_hoverIndex >= 0 && m_hoverIndex < m_wavelengths.size()) {
        const int x = categoryX(m_hoverIndex);
        QPen pointerPen(kPointerLine, 1, Qt::DashLine);
        p.setPen(pointerPen);
        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        drawTooltip(p);
    }
}

void SpectralCurveChart::drawTooltip(QPainter &p)
{
    struct Row { QColor color; QString text; };
    QVector<Row> rows;
    for (const auto &d : m_drawables) {
        if (m_mode == Mode::DeltaReflectance && d.isTarget) continue;
        const double v = (m_hoverIndex < d.values.size())
                             ? d.values[m_hoverIndex]
                             : std::numeric_limits<double>::quiet_NaN();
        const QString val = isFinite(v) ? QString::number(v, 'f', 2) + QStringLiteral("%")
                                        : QStringLiteral("—");
        rows.append({d.src.color, d.src.name + QStringLiteral(": ") + val});
    }

    QFont f = p.font();
    f.setPixelSize(12);
    const QFontMetrics fm(f);
    QFont boldF = f;
    boldF.setBold(true);
    const QFontMetrics bfm(boldF);

    const QString header = T("wavelength") + QStringLiteral(": ")
                           + QString::number(static_cast<int>(std::lround(m_wavelengths[m_hoverIndex])));

    constexpr int padX = 12;
    constexpr int padY = 8;
    constexpr int rowH = 20;
    constexpr int colGap = 16;
    const int headerH = bfm.height() + 6;

    // 行数过多时改为两列排布，避免超出控件高度被裁剪
    int columns = 1;
    if (padY * 2 + headerH + static_cast<int>(rows.size()) * rowH > height() - 12
        && rows.size() > 6) {
        columns = 2;
    }
    const int perCol = (static_cast<int>(rows.size()) + columns - 1) / columns;

    QVector<int> colW(columns, 0);
    for (int i = 0; i < rows.size(); ++i) {
        const int c = i / std::max(1, perCol);
        if (c < columns)
            colW[c] = std::max(colW[c], 14 + fm.horizontalAdvance(rows[i].text));
    }
    int contentW = bfm.horizontalAdvance(header);
    int totalW = (columns - 1) * colGap;
    for (int c = 0; c < columns; ++c) totalW += colW[c];
    contentW = std::min(std::max(contentW, totalW), std::max(60, width() - 24));

    const int tipW = contentW + padX * 2;
    const int tipH = padY * 2 + headerH + perCol * rowH;

    int tipX = m_hoverPos.x() + 14;
    if (tipX + tipW > width() - 6) tipX = m_hoverPos.x() - tipW - 14;
    tipX = std::max(6, tipX);
    int tipY = m_hoverPos.y() + 12;
    tipY = std::clamp(tipY, 6, std::max(6, height() - tipH - 6));

    const QRectF tipRect(tipX, tipY, tipW, tipH);
    p.setPen(QPen(kTooltipBorder, 1));
    p.setBrush(QColor(255, 255, 255, 250));
    p.drawRoundedRect(tipRect, 4, 4);

    int y = tipY + padY;
    p.setFont(boldF);
    p.setPen(kTooltipText);
    p.drawText(QRectF(tipX + padX, y, contentW, headerH - 6),
               Qt::AlignLeft | Qt::AlignVCenter, header);
    y += headerH;

    p.setFont(f);
    QVector<int> colOffset(columns, 0);
    for (int c = 1; c < columns; ++c)
        colOffset[c] = colOffset[c - 1] + colW[c - 1] + colGap;
    for (int i = 0; i < rows.size(); ++i) {
        const int c = std::min(columns - 1, i / std::max(1, perCol));
        const int rowY = y + (i % std::max(1, perCol)) * rowH;
        const int rowX = tipX + padX + colOffset[c];
        p.setPen(Qt::NoPen);
        p.setBrush(rows[i].color);
        p.drawEllipse(QPointF(rowX + 4, rowY + rowH / 2.0), 4.0, 4.0);
        p.setPen(kTooltipText);
        p.drawText(QRectF(rowX + 14, rowY, colW[c] - 14, rowH),
                   Qt::AlignLeft | Qt::AlignVCenter, rows[i].text);
    }
}

QVector<SpectralCurveChart::LegendEntry> SpectralCurveChart::collectLegendEntries() const
{
    QVector<LegendEntry> entries;
    for (const auto &d : m_drawables) {
        if (m_mode == Mode::DeltaReflectance && d.isTarget) continue;
        const QString text = d.src.legendName.isEmpty() ? d.src.name : d.src.legendName;
        if (text.isEmpty()) continue;
        bool seen = false;
        for (const auto &e : entries) {
            if (e.text == text) { seen = true; break; }
        }
        if (!seen) entries.append({text, d.src.lineStyle});
    }
    return entries;
}

int SpectralCurveChart::legendRowCount(const QVector<LegendEntry> &entries,
                                      const QFontMetrics &fm, double availWidth) const
{
    if (entries.isEmpty()) return 0;
    int lines = 1;
    double x = 0.0;
    for (const auto &e : entries) {
        const double w = kLegendSampleW + 6 + fm.horizontalAdvance(e.text);
        if (x > 0 && x + kLegendGap + w > availWidth) {
            ++lines;
            x = 0.0;
        }
        x += (x > 0 ? kLegendGap : 0.0) + w;
    }
    return lines;
}

void SpectralCurveChart::drawLegend(QPainter &p, const QVector<LegendEntry> &entries,
                                    const QFontMetrics &fm, double availWidth)
{
    if (entries.isEmpty()) return;
    double x = kMarginLeft;
    double y = kLegendTop;
    for (const auto &e : entries) {
        const double textW = fm.horizontalAdvance(e.text);
        const double w = kLegendSampleW + 6 + textW;
        if (x > kMarginLeft && x + w > kMarginLeft + availWidth) {
            x = kMarginLeft;
            y += kLegendLineH;
        }
        p.setPen(QPen(kLegendColor, 2, e.style));
        p.drawLine(QPointF(x, y + kLegendLineH / 2.0),
                   QPointF(x + kLegendSampleW, y + kLegendLineH / 2.0));
        p.setPen(kLegendColor);
        p.drawText(QRectF(x + kLegendSampleW + 6, y, textW + 4, kLegendLineH),
                   Qt::AlignLeft | Qt::AlignVCenter, e.text);
        x += w + kLegendGap;
    }
}
