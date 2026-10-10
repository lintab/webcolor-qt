/* ============================================================
   SpectralCurveChart.h —— 光谱曲线图（QPainter 自绘）
   对标前端 ScSpectralCurvePanel.vue：
   - R / ΔR 两种模式（ΔR 隐藏标样并逐角度减去标样值）
   - 按角度配色（scViewTypes.SC_GEOMETRY_COLORS）
   - 线型按来源区分（target/trial 实线、initial 虚线、recipe/ac 点线、acEdited 点划线）
   - 绘图区上方画来源图例（同来源的多角度曲线合并为一条），颜色只表示角度，
     因此图例样本用中性色 + 该来源线型
   - 悬停轴指针 + 悬浮提示（波长 (nm): xxx + 各系列值）
   - 无数据时显示「暂无可展示的数据」
   ============================================================ */

#pragma once

#include <QColor>
#include <QFontMetrics>
#include <QMap>
#include <QPoint>
#include <QString>
#include <QVector>
#include <QWidget>

class SpectralCurveChart : public QWidget
{
    Q_OBJECT
public:
    enum class Mode { Reflectance, DeltaReflectance };

    struct Series {
        QString sourceKey;              // target / initial / ac / acEdited / trial / recipe
        QString geometry;               // 角度（45as15 等）
        QString name;                   // 显示名，含角度，如「初始 45as45」
        QString legendName;             // 来源名（不含角度），图例按它合并同来源曲线
        QColor color;                   // 按角度配色
        Qt::PenStyle lineStyle = Qt::SolidLine;
        QMap<double, double> values;    // 波长(nm) -> 反射率(%)
    };

    explicit SpectralCurveChart(QWidget *parent = nullptr);

    void setSeries(const QVector<Series> &series);
    void setMode(Mode mode);
    Mode mode() const { return m_mode; }
    void setEmptyText(const QString &text);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct DrawableSeries {
        Series src;
        bool isTarget = false;
        QVector<double> values;         // 与 m_wavelengths 对齐，NaN 表示缺失
    };

    void rebuildCache();
    QVector<double> collectWavelengths() const;
    QRectF plotRect() const;
    int categoryX(int index) const;
    int hitIndex(const QPoint &pos) const;
    void drawEmptyState(QPainter &p);
    void drawTooltip(QPainter &p);

    struct LegendEntry {
        QString text;
        Qt::PenStyle style = Qt::SolidLine;
    };
    QVector<LegendEntry> collectLegendEntries() const;
    int legendRowCount(const QVector<LegendEntry> &entries, const QFontMetrics &fm,
                       double availWidth) const;
    void drawLegend(QPainter &p, const QVector<LegendEntry> &entries, const QFontMetrics &fm,
                    double availWidth);

    QVector<Series> m_series;
    Mode m_mode = Mode::Reflectance;
    QString m_emptyText;

    QVector<double> m_wavelengths;
    QVector<DrawableSeries> m_drawables;
    double m_yMin = 0.0;
    double m_yMax = 100.0;
    double m_yStep = 20.0;
    int m_legendExtraTop = 0;           // 图例换行时额外占用的顶部留白

    int m_hoverIndex = -1;
    QPoint m_hoverPos;
};
