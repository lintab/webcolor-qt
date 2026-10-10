/* ============================================================
   AutoScPage.cpp —— 自动搜色（S&C）页面实现
   ============================================================ */

#include "AutoScPage.h"
#include "api/ScApi.h"
#include "api/FormulaApi.h"
#include "api/HttpClient.h"
#include "ui/SpectralCurveChart.h"
#include "utils/I18nManager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QToolButton>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QNetworkAccessManager>
#include <QDateTime>
#include <QUuid>
#include <QDate>
#include <QRandomGenerator>
#include <QFormLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QSplitter>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QSet>
#include <QSignalBlocker>
#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QFrame>
#include <QPalette>
#include <QStyleFactory>
#include <QLinearGradient>
#include <QSharedPointer>
#include <QDoubleValidator>
#include <QColor>
#include <QTimer>
#include <QBrush>
#include <QFont>
#include <QStyledItemDelegate>
#include <QJsonParseError>
#include <QStyle>
#include <QFontMetrics>
#include <QMenu>
#include <QSettings>
#include <QSpinBox>
#include <QFileDialog>
#include "utils/FormulaExport.h"
#include <functional>
#include <memory>
#include <algorithm>
#include <cmath>
#include <vector>

// ─── 本地工具 ────────────────────────────────────────────────
namespace {

/**
 * 与其它模块一致的滑动条样式（MainWindow / FormulaDetailWidget 同款配色）。
 * Windows 原生横向滑动条槽是白的、滑块很浅，表格底部那条几乎看不出能拖，
 * 且原生条会随系统主题悬浮盖住内容，所以带表格/滚动区的地方统一拼上这段。
 */
const char *const kScrollBarQss =
    "QScrollBar:vertical { background: #f0f2f5; width: 10px; margin: 0; border-radius: 5px; }"
    "QScrollBar::handle:vertical { background: #606266; min-height: 30px; border-radius: 5px; }"
    "QScrollBar::handle:vertical:hover { background: #4a4c50; }"
    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
    "QScrollBar:horizontal { background: #f0f2f5; height: 10px; margin: 0; border-radius: 5px; }"
    "QScrollBar::handle:horizontal { background: #606266; min-width: 30px; border-radius: 5px; }"
    "QScrollBar::handle:horizontal:hover { background: #4a4c50; }"
    "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
    "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }";

/** 分数色调 → 颜色，与前端 .sc-score--green/yellow/red 一致 */
QColor scoreToneColor(const QString &tone)
{
    if (tone == QLatin1String("green"))  return QColor(QStringLiteral("#059669"));
    if (tone == QLatin1String("yellow")) return QColor(QStringLiteral("#ca8a04"));
    if (tone == QLatin1String("red"))    return QColor(QStringLiteral("#dc2626"));
    return QColor(QStringLiteral("#303133"));
}

/** 与前端 resolveScSettingsPayload 一致：剥壳 code/data，并合并嵌套 settings/display */
QJsonObject resolveScSettingsPayload(const QJsonObject &payload)
{
    if (payload.isEmpty()) return payload;
    if (payload.contains(QStringLiteral("code")) && !payload[QStringLiteral("data")].isUndefined()) {
        const auto data = payload[QStringLiteral("data")];
        if (data.isObject()) return resolveScSettingsPayload(data.toObject());
    }
    QJsonObject source = payload;
    const auto nested = {QStringLiteral("settings"), QStringLiteral("display"),
                         QStringLiteral("displaySettings")};
    for (const auto &key : nested) {
        const auto v = source[key].toObject();
        for (auto it = v.begin(); it != v.end(); ++it) source[it.key()] = it.value();
    }
    return source;
}

/** S&C 前端显示相关配置字段（与后端 ScSettings 一致） */
const QStringList kScDisplayKeys = {
    QStringLiteral("scoreDecimalPlaces"),
    QStringLiteral("solidInitialGreen"),  QStringLiteral("solidInitialYellow"),
    QStringLiteral("solidInitialRed"),
    QStringLiteral("solidCorrectedGreen"), QStringLiteral("solidCorrectedYellow"),
    QStringLiteral("solidCorrectedRed"),
    QStringLiteral("effectInitialGreen"),  QStringLiteral("effectInitialYellow"),
    QStringLiteral("effectInitialRed"),
    QStringLiteral("effectCorrectedGreen"), QStringLiteral("effectCorrectedYellow"),
    QStringLiteral("effectCorrectedRed")
};

/** 与前端 pickScDisplaySettings 一致：仅保留存在的数值字段 */
void mergeScDisplayKeys(QJsonObject &dst, const QJsonObject &raw)
{
    const auto src = resolveScSettingsPayload(raw);
    for (const auto &key : kScDisplayKeys) {
        const auto v = src[key];
        if (v.isDouble()) { dst[key] = v; continue; }
        if (v.isString()) {
            const QString s = v.toString();
            if (s.isEmpty()) continue;
            bool ok = false;
            double n = s.toDouble(&ok);
            if (ok) dst[key] = n;
        }
    }
}

/** 用户配置是否启用（启用时覆盖全局配置） */
bool isScMineSettingsEnabled(const QJsonObject &raw)
{
    const auto src = resolveScSettingsPayload(raw);
    const auto v = src[QStringLiteral("enabled")];
    if (v.isBool()) return v.toBool();
    if (v.isDouble()) return v.toInt() == 1;
    const QString s = v.toString();
    return s == QLatin1String("1") || s.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
}

/** 与前端 rgbToHex 一致：hex 优先，否则由 r/g/b 拼接 */
QString hexFromRgb(const QJsonObject &rgb)
{
    if (rgb.isEmpty()) return QString();
    QString hex = rgb[QStringLiteral("hex")].toString().trimmed();
    if (!hex.isEmpty()) {
        const QString candidate = hex.startsWith(QStringLiteral("#"))
            ? hex : QStringLiteral("#") + hex;
        if (QColor(candidate).isValid()) return candidate;
    }
    if (!rgb[QStringLiteral("r")].isDouble() || !rgb[QStringLiteral("g")].isDouble()
        || !rgb[QStringLiteral("b")].isDouble())
        return QString();
    const int r = rgb[QStringLiteral("r")].toInt();
    const int g = rgb[QStringLiteral("g")].toInt();
    const int b = rgb[QStringLiteral("b")].toInt();
    return QStringLiteral("#%1%2%3")
        .arg(r, 2, 16, QLatin1Char('0')).arg(g, 2, 16, QLatin1Char('0')).arg(b, 2, 16, QLatin1Char('0'))
        .toUpper();
}

/** 可视化的色彩数组字段名（标样 m_vizData 与配方 m_formulaVizData 共用） */
QJsonArray extractColorimetry(const QJsonObject &vizData)
{
    auto arr = vizData[QStringLiteral("colorimetry")].toArray();
    if (arr.isEmpty()) arr = vizData[QStringLiteral("colorData")].toArray();
    if (arr.isEmpty()) arr = vizData[QStringLiteral("measurements")].toArray();
    return arr;
}

/**
 * 可视化色彩数组 -> 来源 -> 角度 -> hex。
 * 后端每个角度行都带 geometryCode + rgb.hex，所以只选了测量作业（还没搜色、没点配方行）
 * 时也能拿到标样五角度颜色，色块卡片因此不必等配方行。
 */
QMap<QString, QMap<QString, QString>> colorimetryHexBySource(const QJsonArray &colorimetry)
{
    static const QStringList kAngles = {
        QStringLiteral("45as15"), QStringLiteral("45as25"), QStringLiteral("45as45"),
        QStringLiteral("45as75"), QStringLiteral("45as110")
    };
    QMap<QString, QMap<QString, QString>> out;
    for (const auto &v : colorimetry) {
        const QJsonObject item = v.toObject();
        if (item.isEmpty()) continue;
        QString angle = item[QStringLiteral("geometryCode")].toString();
        if (angle.isEmpty()) angle = item[QStringLiteral("geometry")].toString();
        if (!kAngles.contains(angle)) continue;
        const QString hex = hexFromRgb(item[QStringLiteral("rgb")].toObject());
        if (hex.isEmpty()) continue;
        QString src = item[QStringLiteral("sourceKey")].toString().trimmed().toLower();
        if (src.isEmpty()) src = QStringLiteral("target");
        if (!out[src].contains(angle)) out[src].insert(angle, hex);
    }
    return out;
}

/** 用 src 补齐 dst 里缺失的角度（rgbByGeometry 优先，可视化数据兜底） */
void mergeMissingAngles(QMap<QString, QString> &dst, const QMap<QString, QString> &src)
{
    for (auto it = src.constBegin(); it != src.constEnd(); ++it) {
        if (!dst.contains(it.key())) dst.insert(it.key(), it.value());
    }
}

/** 与前端 hashColor 一致：无颜色信息时按编号哈希出一个稳定色 */
QString hashColor(const QString &code)
{
    int hash = 0;
    for (const QChar &c : code) hash = c.unicode() + ((hash << 5) - hash);
    return QStringLiteral("#%1").arg(quint32(hash & 0xffffff), 6, 16, QLatin1Char('0'));
}

/** 校验并规范化 hex 字符串 */
QString normalizeHex(const QString &raw)
{
    QString hex = raw.trimmed();
    if (hex.isEmpty()) return QString();
    if (!hex.startsWith(QStringLiteral("#"))) hex.prepend(QStringLiteral("#"));
    return QColor(hex).isValid() ? hex : QString();
}

/** 配方百分比列宽 / 数值格式化（前端 toFixed(2)） */
inline QString fmtFixed(double v, int dp)
{
    return QString::number(v, 'f', dp);
}

/** 涂层类型规范化（后端约定值，用于逻辑判断） */
QString coatTypeCanonical(const QString &stored)
{
    static const QStringList canonical = {
        QStringLiteral("素色"), QStringLiteral("银粉"), QStringLiteral("珍珠"),
        QStringLiteral("混合色"), QStringLiteral("三工序白珍珠"), QStringLiteral("糖果色")
    };
    static const QMap<QString, QString> aliases = {
        {QStringLiteral("Solid"), QStringLiteral("素色")},
        {QStringLiteral("S"), QStringLiteral("素色")},
        {QStringLiteral("Metallic / silver"), QStringLiteral("银粉")},
        {QStringLiteral("M"), QStringLiteral("银粉")},
        {QStringLiteral("Metallic"), QStringLiteral("银粉")},
        {QStringLiteral("METALLIC"), QStringLiteral("银粉")},
        {QStringLiteral("SOLID"), QStringLiteral("素色")},
        {QStringLiteral("Pearl"), QStringLiteral("珍珠")},
        {QStringLiteral("P"), QStringLiteral("珍珠")},
        {QStringLiteral("Mixed"), QStringLiteral("混合色")},
        {QStringLiteral("MP"), QStringLiteral("混合色")},
        {QStringLiteral("3-stage pearl"), QStringLiteral("三工序白珍珠")},
        {QStringLiteral("3C"), QStringLiteral("三工序白珍珠")},
        {QStringLiteral("三工序珍珠"), QStringLiteral("三工序白珍珠")},
        {QStringLiteral("Candy"), QStringLiteral("糖果色")},
        {QStringLiteral("3CC"), QStringLiteral("糖果色")}
    };
    const QString v = stored.trimmed();
    if (v.isEmpty()) return QString();
    if (canonical.contains(v)) return v;
    return aliases.value(v, v);
}

/** 涂层类型显示文案（跟随界面语言，未知值原样显示） */
QString coatTypeDisplay(const QString &stored)
{
    const QString c = coatTypeCanonical(stored);
    if (c.isEmpty()) return QStringLiteral("—");
    if (c == QStringLiteral("素色"))         return T("autosc_coat_solid");
    if (c == QStringLiteral("银粉"))         return T("autosc_coat_silver");
    if (c == QStringLiteral("珍珠"))         return T("autosc_coat_pearl");
    if (c == QStringLiteral("混合色"))       return T("autosc_coat_mixed");
    if (c == QStringLiteral("三工序白珍珠")) return T("autosc_coat_three_pearl");
    if (c == QStringLiteral("糖果色"))       return T("autosc_coat_candy");
    return c;
}

/** 颜色偏向显示文案：后端 colorBias 存固定中文值（偏红/偏浅等），界面按语言显示 */
QString colorBiasDisplay(const QString &stored)
{
    const QString key = stored.trimmed();
    if (key.isEmpty()) return QStringLiteral("—");
    static const QMap<QString, const char *> labels = {
        {QStringLiteral("标准"),   "bias_standard"},
        {QStringLiteral("差异色"), "bias_variant"},
        {QStringLiteral("偏浅"),   "bias_lighter"},
        {QStringLiteral("偏深"),   "bias_darker"},
        {QStringLiteral("偏红"),   "autosc_bias_red"},
        {QStringLiteral("偏黄"),   "autosc_bias_yellow"},
        {QStringLiteral("偏蓝"),   "autosc_bias_blue"},
        {QStringLiteral("偏绿"),   "autosc_bias_green"}
    };
    return labels.contains(key) ? T(labels.value(key)) : key;
}

/** 与前端 parseWeight 一致：兼容千分位字符串 */
double parseWeight(const QJsonValue &val)
{
    if (val.isDouble()) return val.toDouble();
    QString s = val.toString();
    s.remove(QLatin1Char(','));
    bool ok = false;
    double n = s.trimmed().toDouble(&ok);
    return ok ? n : 0.0;
}

/** 与前端 formatWeightValue/trimNumber 一致：>=100 保留 1 位，否则 3 位，去尾零 */
QString formatWeightValue(double grams)
{
    if (!std::isfinite(grams)) return QStringLiteral("—");
    QString s = QString::number(grams, 'f', grams >= 100.0 ? 1 : 3);
    if (s.contains(QLatin1Char('.'))) {
        while (s.endsWith(QLatin1Char('0'))) s.chop(1);
        if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    }
    return s.isEmpty() ? QStringLiteral("0") : s;
}

/** 底部拖拽条：拖拽时临时安装 qApp 事件过滤器，释放时移除 */
class BottomDragBar : public QWidget
{
    Q_OBJECT
public:
    explicit BottomDragBar(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_dragging(false)
        , m_lastGlobalY(0)
    {
        setFixedHeight(10);
        setCursor(Qt::SizeVerCursor);
        setNormalStyle();
    }
    ~BottomDragBar() override {
        if (m_dragging && qApp) qApp->removeEventFilter(this);
    }
signals:
    void dragDelta(int dy);
protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton) {
            m_dragging = true;
            m_lastGlobalY = e->globalPosition().toPoint().y();
            qApp->installEventFilter(this); // 安装全局过滤器，保证鼠标移出时仍能收到事件
            setStyleSheet(QStringLiteral(
                "BottomDragBar { background: #909399; border-radius: 0px 0px 5px 5px; }"));
            e->accept();
        }
    }
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (!m_dragging) return QWidget::eventFilter(obj, event);
        if (event->type() == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent *>(event);
            int currentY = me->globalPosition().toPoint().y();
            int delta = currentY - m_lastGlobalY;
            if (delta != 0) {
                emit dragDelta(delta);
                m_lastGlobalY = currentY;
            }
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton || me->buttons() == Qt::NoButton) {
                m_dragging = false;
                qApp->removeEventFilter(this);
                setNormalStyle();
                return false;
            }
        }
        return QWidget::eventFilter(obj, event);
    }
private:
    bool m_dragging;
    int m_lastGlobalY;
    void setNormalStyle() {
        setStyleSheet(QStringLiteral(
            "BottomDragBar { background: #e4e7ed; border-radius: 0px 0px 5px 5px; }"
            "BottomDragBar:hover { background: #c0c4cc; }"));
    }
};

/** 可拖拽底部调整高度的卡片模块（带可见边框，支持可选折叠） */
class ResizablePanel : public QWidget
{
    Q_OBJECT
public:
    explicit ResizablePanel(const QString &title, bool collapsible = false, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_collapsible(collapsible)
        , m_collapsed(false)
        , m_expandedHeight(0)
    {
        // 启用样式表背景色（普通QWidget默认不会渲染背景）
        setAttribute(Qt::WA_StyledBackground, true);
        // 关键：告诉外部布局本控件竖向尺寸可被强制，不受子控件min限制
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
        // 外层背景 = 边框颜色
        setStyleSheet(QStringLiteral(
            "ResizablePanel { background-color: #d0d4dc; border-radius: 6px; }"));

        auto *outerLayout = new QVBoxLayout(this);
        outerLayout->setContentsMargins(2, 2, 2, 2); // 2px 让边框颜色可见
        outerLayout->setSpacing(0);

        // 白色内容区（尺寸可伸缩，内容超出时裁剪）
        m_contentWidget = new QWidget;
        m_contentWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
        m_contentWidget->setStyleSheet(QStringLiteral(
            "background-color: #ffffff; border-radius: 5px 5px 5px 5px;"));
        auto *contentLayout = new QVBoxLayout(m_contentWidget);
        contentLayout->setContentsMargins(10, 10, 10, 10);
        contentLayout->setSpacing(0);

        // 头部行：标题 + 弹性 + 可选折叠按钮
        if (!title.isEmpty()) {
            auto *headerRow = new QHBoxLayout;
            headerRow->setContentsMargins(0, 0, 0, 0);
            headerRow->setSpacing(8);
            auto *header = new QLabel(title);
            header->setStyleSheet(QStringLiteral(
                "font-weight: bold; font-size: 13px; color: #303133; "
                "padding: 0; border: none; background: transparent;"));
            headerRow->addWidget(header);
            if (m_collapsible) {
                m_collapseBtn = new QToolButton;
                m_collapseBtn->setText(QStringLiteral("▼")); // 展开时向下三角
                m_collapseBtn->setCursor(Qt::PointingHandCursor);
                m_collapseBtn->setFixedSize(24, 24);
                m_collapseBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
                m_collapseBtn->setStyleSheet(QStringLiteral(
                    "QToolButton { border: 1px solid #dcdfe6; background: #f5f7fa; color: #606266; "
                    "font-size: 10px; font-weight: bold; border-radius: 4px; }"
                    "QToolButton:hover { background: #e6e8eb; color: #303133; border-color: #c0c4cc; }"
                    "QToolButton:pressed { background: #dcdfe6; }"));
                connect(m_collapseBtn, &QToolButton::clicked, this, &ResizablePanel::toggleCollapse);
                headerRow->addWidget(m_collapseBtn);
            }
            headerRow->addStretch(); // 弹性放在按钮之后，让按钮紧贴标题
            contentLayout->addLayout(headerRow);
        }

        // 可隐藏的主体区域（标题以下的所有内容）
        m_bodyArea = new QWidget(m_contentWidget);
        m_bodyArea->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
        auto *bodyLayout = new QVBoxLayout(m_bodyArea);
        bodyLayout->setContentsMargins(0, 0, 0, 0);
        bodyLayout->setSpacing(0);
        bodyLayout->addSpacing(8);

        m_contentLayout = new QVBoxLayout;
        m_contentLayout->setContentsMargins(0, 0, 0, 0);
        m_contentLayout->setSpacing(0);
        bodyLayout->addLayout(m_contentLayout);

        contentLayout->addWidget(m_bodyArea);
        outerLayout->addWidget(m_contentWidget, 1);

        // 底部拖拽条
        m_dragBar = new BottomDragBar;
        outerLayout->addWidget(m_dragBar);

        connect(m_dragBar, &BottomDragBar::dragDelta, this, &ResizablePanel::heightChanged);
    }
    QVBoxLayout *contentLayout() { return m_contentLayout; }
    bool isCollapsed() const { return m_collapsed; }
    int expandedHeight() const { return m_expandedHeight; }
    void setExpandedHeight(int h) { m_expandedHeight = h; }

public slots:
    void toggleCollapse(bool forceExpand = false)
    {
        if (!m_collapsible) return;
        if (m_collapsed) {
            // 展开
            m_collapsed = false;
            m_contentWidget->show();
            m_bodyArea->show();
            m_dragBar->show();
            const int target = m_expandedHeight > 0 ? m_expandedHeight : 380;
            setFixedHeight(target);
            if (m_collapseBtn) m_collapseBtn->setText(QStringLiteral("▼"));
            emit collapsedToggled();
        } else if (!forceExpand) {
            // 折叠（仅在非强制展开时）
            m_collapsed = true;
            m_expandedHeight = height(); // 记住当前高度供下次展开恢复
            m_bodyArea->hide();
            m_dragBar->hide();
            // 仅保留标题栏高度：上外边2 + 上内边10 + header行~22 + 下内边10 + 下外边2
            const int collapsedH = 46;
            setFixedHeight(collapsedH);
            if (m_collapseBtn) m_collapseBtn->setText(QStringLiteral("▶"));
            emit collapsedToggled();
        }
        // 如果 forceExpand=true 且已展开，什么都不做
    }

signals:
    void heightChanged(int delta);
    void collapsedToggled();

private:
    bool m_collapsible;
    bool m_collapsed;
    int m_expandedHeight;
    QWidget *m_contentWidget = nullptr;
    QWidget *m_bodyArea = nullptr;
    QToolButton *m_collapseBtn = nullptr;
    BottomDragBar *m_dragBar = nullptr;
    QVBoxLayout *m_contentLayout = nullptr;
};

/** macOS / iOS 风格的加载转圈指示器（UIActivityIndicator 样式的放射状渐隐条）
    作为父控件（如表格 viewport）的覆盖层，居中绘制并旋转。 */
class SpinnerOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit SpinnerOverlay(QWidget *parent)
        : QWidget(parent), m_angle(0), m_timer(nullptr)
    {
        hide();
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
        m_timer = new QTimer(this);
        m_timer->setInterval(90);
        connect(m_timer, &QTimer::timeout, this, [this]() {
            m_angle = (m_angle + 30) % 360;
            update();
        });
        if (parent) parent->installEventFilter(this);
    }

    void start()
    {
        if (auto *vp = parentWidget()) setGeometry(vp->rect());
        m_timer->start();
        show();
        raise();
    }

    void stop()
    {
        m_timer->stop();
        hide();
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (obj == parentWidget() && event->type() == QEvent::Resize) {
            setGeometry(parentWidget()->rect());
        }
        return QWidget::eventFilter(obj, event);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.translate(rect().center());
        p.rotate(m_angle);

        const int spokes = 12;
        const double outer = 20.0;   // 外半径
        const double inner = 9.0;    // 内半径
        for (int i = 0; i < spokes; ++i) {
            // 渐隐：越靠"尾部"越淡，形成旋转的视觉残留
            const double t = static_cast<double>(i) / spokes;
            const int alpha = static_cast<int>(255.0 * (0.18 + 0.82 * t));
            QColor color(96, 98, 102, alpha);
            QPen pen(color);
            pen.setWidthF(3.2);
            pen.setCapStyle(Qt::RoundCap);
            p.setPen(pen);
            p.save();
            p.rotate(360.0 * i / spokes);
            p.drawLine(QPointF(0, -inner), QPointF(0, -outer));
            p.restore();
        }
    }

private:
    int m_angle;
    QTimer *m_timer;
};

// ─── “作业 | 配方”对比列：数据解析（对齐前端 ScJobFormulaCompare）────────

/** 预览图条目（角度码 + URL），对齐前端 parsePreviewImageItems */
struct PreviewItem { QString url; QString geometryCode; int sortOrder = 0; };

/** MA-5C 五角度顺序（前端 MEASUREMENT_GEOMETRY_ORDER） */
static const QStringList kGeometryOrder = {
    QStringLiteral("45as15"), QStringLiteral("45as25"), QStringLiteral("45as45"),
    QStringLiteral("45as75"), QStringLiteral("45as110")
};

static QString normalizeGeoCode(const QString &code)
{
    QString s = code.trimmed();
    if (s.startsWith(QLatin1Char('r'))) s = s.mid(1);
    return s;
}

static QString pickUrlField(const QJsonObject &o)
{
    static const char *keys[] = {"imageUrl", "url", "src"};
    for (const char *k : keys) {
        QString v = o[QString::fromUtf8(k)].toString();
        if (!v.isEmpty()) return v;
    }
    return QString();
}

/** 解析 previewImages（字符串/对象数组，兼容 JSON 字符串），按角度排序 */
static QList<PreviewItem> parsePreviewItems(const QJsonValue &val)
{
    QJsonArray arr;
    if (val.isArray()) {
        arr = val.toArray();
    } else if (val.isString()) {
        QJsonParseError err;
        QJsonDocument d = QJsonDocument::fromJson(val.toString().toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && d.isArray()) arr = d.array();
        else if (!val.toString().trimmed().isEmpty()) arr.append(val);
    }
    QList<PreviewItem> items;
    for (int i = 0; i < arr.size(); ++i) {
        const QJsonValue &v = arr[i];
        PreviewItem it;
        it.sortOrder = i;
        if (v.isString()) {
            it.url = v.toString();
        } else if (v.isObject()) {
            QJsonObject o = v.toObject();
            it.url = pickUrlField(o);
            it.geometryCode = o[QStringLiteral("geometryCode")].toString();
            if (o[QStringLiteral("sortOrder")].isDouble()) it.sortOrder = o[QStringLiteral("sortOrder")].toInt();
        }
        if (it.url.isEmpty()) continue;
        if (it.geometryCode.isEmpty())
            it.geometryCode = (i < kGeometryOrder.size()) ? kGeometryOrder[i] : QString();
        items.append(it);
    }
    std::stable_sort(items.begin(), items.end(), [](const PreviewItem &a, const PreviewItem &b) {
        // 对齐前端 comparePreviewGeometry：先按 MA-5C 角度序，否则按角度码字符串比较
        // （非 MA-5C 设备如 15as-15/15as15/... 时，必须按码字符串排序才能让作业侧与配方侧排到同一序列，
        //  进而 buildAngleMap 的下标配对才正确；若退回按数组下标排序会造成错位）
        const QString ga = normalizeGeoCode(a.geometryCode);
        const QString gb = normalizeGeoCode(b.geometryCode);
        const int ia = kGeometryOrder.indexOf(ga);
        const int ib = kGeometryOrder.indexOf(gb);
        int byGeo;
        if (ia >= 0 && ib >= 0) byGeo = ia - ib;
        else if (ia >= 0) byGeo = -1;
        else if (ib >= 0) byGeo = 1;
        else byGeo = ga.compare(gb);   // 码字符串比较（'-' 45 < '1' 49，与前端 localeCompare 一致）
        if (byGeo != 0) return byGeo < 0;
        return a.sortOrder < b.sortOrder;
    });
    return items;
}

static QMap<QString, QString> buildAngleMap(const QList<PreviewItem> &items)
{
    QMap<QString, QString> map;
    for (int index = 0; index < items.size(); ++index) {
        QString raw = normalizeGeoCode(items[index].geometryCode);
        QString angle = kGeometryOrder.contains(raw) ? raw
                        : (index < kGeometryOrder.size() ? kGeometryOrder[index] : raw);
        if (!items[index].url.isEmpty() && !angle.isEmpty()) map.insert(angle, items[index].url);
    }
    return map;
}

/** 计算对齐的作业/配方 URL 带列表（前端 anglePairs） */
static void buildComparePairs(const QList<PreviewItem> &jobItems,
                              const QList<PreviewItem> &formulaItems,
                              QStringList &outJob, QStringList &outFormula)
{
    outJob.clear(); outFormula.clear();
    const auto jobMap = buildAngleMap(jobItems);
    const auto formulaMap = buildAngleMap(formulaItems);
    QStringList jobUrls, formulaUrls;
    for (const auto &it : jobItems) jobUrls << it.url;
    for (const auto &it : formulaItems) formulaUrls << it.url;

    QStringList present;
    for (const auto &angle : kGeometryOrder)
        if (jobMap.contains(angle) || formulaMap.contains(angle)) present << angle;
    if (present.isEmpty()) {
        const int count = qMax(jobUrls.size(), formulaUrls.size());
        for (int i = 0; i < count && i < kGeometryOrder.size(); ++i) present << kGeometryOrder[i];
    }
    for (int i = 0; i < present.size(); ++i) {
        const QString &angle = present[i];
        QString j = jobMap.value(angle); if (j.isEmpty() && i < jobUrls.size()) j = jobUrls[i];
        QString f = formulaMap.value(angle); if (f.isEmpty() && i < formulaUrls.size()) f = formulaUrls[i];
        outJob << j; outFormula << f;
    }
}

// ─── 对比列单元格数据角色 ────────────────────────────────────
enum {
    RoleJobUrls = Qt::UserRole + 5,
    RoleFormulaUrls = Qt::UserRole + 6,
    RoleFormulaRawUrls = Qt::UserRole + 7,   // 本行配方图原始 url（未按角度配对，供作业图晚到时重算）
    RoleFormulaRawGeo = Qt::UserRole + 8     // 与原始 url 对齐的角度码
};

/** 对齐前端 ScJobFormulaCompare 的单元格渲染器 */
class JobFormulaCompareDelegate : public QStyledItemDelegate
{
public:
    std::function<QPixmap(const QString &)> resolver;
    QPixmap logo;

    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return QSize(108, 120);
    }

    void paint(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const QRect cell = option.rect;
        p->fillRect(cell, (option.state & QStyle::State_Selected)
                              ? QColor(QStringLiteral("#e8f0fe")) : QColor(QStringLiteral("#ffffff")));

        const QStringList jobUrls = index.data(RoleJobUrls).toStringList();
        const QStringList formulaUrls = index.data(RoleFormulaUrls).toStringList();
        const int bands = qMax(jobUrls.size(), formulaUrls.size());

        const int margin = 3;
        const QRect area = cell.adjusted(margin, margin, -margin, -margin);

        if (bands <= 0) {
            drawLogoEmpty(p, area);
            p->restore();
            return;
        }

        const int headerH = 15;
        drawHeader(p, QRect(area.left(), area.top(), area.width(), headerH));
        const QRect card(area.left(), area.top() + headerH + 1,
                         area.width(), area.height() - headerH - 1);
        drawCard(p, card, jobUrls, formulaUrls, bands);
        p->restore();
    }

private:
    void drawHeader(QPainter *p, const QRect &r) const
    {
        QFont f = p->font(); f.setPixelSize(11); f.setBold(true); p->setFont(f);
        const QString left = T("autosc_job");
        const QString sep = QStringLiteral("|");
        const QString right = T("autosc_recipe");
        QFontMetrics fm(f);
        const int wL = fm.horizontalAdvance(left);
        const int wS = fm.horizontalAdvance(sep);
        const int wR = fm.horizontalAdvance(right);
        const int gap = 4;
        const int total = wL + gap + wS + gap + wR;
        int x = r.center().x() - total / 2;
        p->setPen(QColor(QStringLiteral("#475569")));
        p->drawText(x, r.top(), wL, r.height(), Qt::AlignVCenter | Qt::AlignLeft, left); x += wL + gap;
        p->setPen(QColor(QStringLiteral("#94a3b8")));
        p->drawText(x, r.top(), wS, r.height(), Qt::AlignVCenter | Qt::AlignLeft, sep); x += wS + gap;
        p->setPen(QColor(QStringLiteral("#475569")));
        p->drawText(x, r.top(), wR, r.height(), Qt::AlignVCenter | Qt::AlignLeft, right);
    }

    void drawLogoEmpty(QPainter *p, const QRect &r) const
    {
        QRect box(0, 0, qMin(120, r.width()), qMin(110, r.height()));
        box.moveCenter(r.center());
        p->setPen(QPen(QColor(QStringLiteral("#ebeef5")), 1));
        p->setBrush(QColor(QStringLiteral("#f5f7fa")));
        p->drawRoundedRect(box, 6, 6);
        drawImageContain(p, box.adjusted(30, 26, -30, -26), logo);
    }

    void drawCard(QPainter *p, const QRect &card, const QStringList &jobUrls,
                  const QStringList &formulaUrls, int bands) const
    {
        p->setPen(QPen(QColor(QStringLiteral("#ebeef5")), 1));
        p->setBrush(QColor(QStringLiteral("#f1f5f9")));
        p->drawRoundedRect(card, 6, 6);

        p->save();
        p->setClipRect(card);
        const int bandH = card.height() / bands;
        for (int i = 0; i < bands; ++i) {
            const QRect band(card.left(), card.top() + i * bandH, card.width(),
                             (i == bands - 1) ? card.height() - i * bandH : bandH);
            const int midX = band.center().x();
            const QRect left(band.left(), band.top(), midX - band.left(), band.height());
            const QRect right(midX, band.top(), band.right() - midX + 1, band.height());
            drawHalf(p, left, i < jobUrls.size() ? jobUrls[i] : QString());
            drawHalf(p, right, i < formulaUrls.size() ? formulaUrls[i] : QString());
            p->setPen(QPen(QColor(255, 255, 255, 166), 1));
            p->drawLine(midX, band.top(), midX, band.bottom());
            if (i > 0) {
                p->setPen(QPen(QColor(255, 255, 255, 128), 1));
                p->drawLine(band.left(), band.top(), band.right(), band.top());
            }
        }
        p->restore();

        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(QColor(QStringLiteral("#ebeef5")), 1));
        p->drawRoundedRect(card, 6, 6);
    }

    void drawHalf(QPainter *p, const QRect &r, const QString &url) const
    {
        p->fillRect(r, QColor(QStringLiteral("#e2e8f0")));
        QPixmap pm = url.isEmpty() ? QPixmap() : (resolver ? resolver(url) : QPixmap());
        if (!pm.isNull()) drawImageCover(p, r, pm);
        else drawImageContain(p, r.adjusted(4, 4, -4, -4), logo);
    }

    void drawImageCover(QPainter *p, const QRect &r, const QPixmap &pm) const
    {
        if (pm.isNull() || r.isEmpty()) return;
        QSize s = pm.size();
        s.scale(r.size(), Qt::KeepAspectRatioByExpanding);
        QRect src(QPoint((pm.width() - s.width()) / 2, (pm.height() - s.height()) / 2), s);
        p->save();
        p->setClipRect(r);
        p->drawPixmap(r, pm, src);
        p->restore();
    }

    void drawImageContain(QPainter *p, const QRect &r, const QPixmap &pm) const
    {
        if (pm.isNull() || r.isEmpty()) return;
        QSize s = pm.size();
        s.scale(r.size(), Qt::KeepAspectRatio);
        QRect target(QPoint(r.center().x() - s.width() / 2, r.center().y() - s.height() / 2), s);
        p->drawPixmap(target, pm);
    }
};

} // namespace

// ─── 构造 ────────────────────────────────────────────────────
AutoScPage::AutoScPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    // ── SDX4 设备信号连接 ──
    auto &dev = XriteDevice::instance();
    connect(&dev, &XriteDevice::connected,
            this, &AutoScPage::onSdx4Connected);
    connect(&dev, &XriteDevice::disconnected,
            this, &AutoScPage::onSdx4Disconnected);
    connect(&dev, &XriteDevice::measurementReady,
            this, &AutoScPage::onSdx4MeasurementReady);
    connect(&dev, &XriteDevice::measurementProgress,
            this, &AutoScPage::onSdx4MeasurementProgress);
    connect(&dev, &XriteDevice::errorOccurred,
            this, &AutoScPage::onSdx4Error);

    // 初始化状态标签
    updateSdx4StatusLabel();
}

void AutoScPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_loaded) {
        m_loaded = true;
        loadInitialData();
    }
    // 首次显示时视口宽度才确定，按它校正一次左栏高度
    QTimer::singleShot(0, this, [this]() { syncSidebarContentHeight(); });
}

// ─── 布局 ────────────────────────────────────────────────────
void AutoScPage::setupUi()
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto *splitter = new QSplitter(Qt::Horizontal);

    // 左栏
    auto *sidebar = new QWidget;
    sidebar->setMinimumWidth(280);
    sidebar->setMaximumWidth(360);
    setupSidebar(sidebar);

    // 右栏
    auto *mainWidget = new QWidget;
    setupMainArea(mainWidget);

    splitter->addWidget(sidebar);
    splitter->addWidget(mainWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
}

// ─── 左侧栏 ──────────────────────────────────────────────────
void AutoScPage::setupSidebar(QWidget *sidebar)
{
    auto *scroll = new QScrollArea;
    m_sidebarScroll = scroll;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scroll->setFrameShape(QFrame::NoFrame);
    // macOS 默认滑动条为悬浮式会遮挡内容，设置样式表可强制非悬浮、占据独立空间
    scroll->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }")
        + QLatin1String(kScrollBarQss));

    auto *container = new QWidget;
    m_sidebarContainer = container;
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(12, 12, 10, 12);
    layout->setSpacing(10);

    // ── 作业定义 ──
    auto *jobGroup = new QGroupBox(T("sc_job_definition"));
    auto *jobLayout = new QFormLayout(jobGroup);
    jobLayout->setContentsMargins(8, 12, 8, 8);
    jobLayout->setSpacing(8);
    jobLayout->setLabelAlignment(Qt::AlignRight);
    jobLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    // 与品牌车型页面完全一致的样式
    auto applyComboStyle = [](QComboBox *combo) {
        combo->setStyleSheet(QStringLiteral(
            "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px 8px; "
            "font-size: 12px; background: #fff; min-height: 20px; color: #333; }"
            "QComboBox:hover { border-color: #c0c4cc; }"
            "QComboBox QAbstractItemView { border: 1px solid #dcdfe6; background: #fff; "
            "color: #333; selection-background-color: #e8f0fe; selection-color: #0d4f8b; }"));
        combo->setMinimumWidth(220);
        // QComboBox 只在 StrongFocus 时才响应滚轮改选项；滚过下拉框就换值会在翻页时误改筛选，
        // 改成点击才聚焦后滚轮事件会交给外层滚动区
        combo->setFocusPolicy(Qt::ClickFocus);
    };
    auto labelStyle = QStringLiteral("color: #606266; font-size: 12px;");

    auto *calsetLabel = new QLabel(T("autosc_gradient_data"));
    calsetLabel->setStyleSheet(labelStyle);
    m_calsetCombo = new QComboBox;
    applyComboStyle(m_calsetCombo);
    jobLayout->addRow(calsetLabel, m_calsetCombo);

    auto *deviceLabel = new QLabel(T("autosc_spectro"));
    deviceLabel->setStyleSheet(labelStyle);
    m_deviceCombo = new QComboBox;
    applyComboStyle(m_deviceCombo);
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AutoScPage::onDeviceChanged);
    jobLayout->addRow(deviceLabel, m_deviceCombo);

    auto *measLabel = new QLabel(T("sc_measurement_job"));
    measLabel->setStyleSheet(labelStyle);
    auto *measRow = new QHBoxLayout;
    measRow->setContentsMargins(0, 0, 0, 0);
    measRow->setSpacing(4);
    m_measurementCombo = new QComboBox;
    m_measurementCombo->setEnabled(false);
    applyComboStyle(m_measurementCombo);
    connect(m_measurementCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AutoScPage::onMeasurementChanged);
    measRow->addWidget(m_measurementCombo, 1);

    m_refreshJobsBtn = new QPushButton(QStringLiteral("↻"));
    m_refreshJobsBtn->setFixedSize(28, 28);
    m_refreshJobsBtn->setEnabled(false);
    m_refreshJobsBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border: 1px solid #dcdfe6; border-radius: 14px; background: #fff; "
        "font-size: 14px; color: #606266; }"
        "QPushButton:hover { border-color: #409eff; color: #409eff; }"));
    connect(m_refreshJobsBtn, &QPushButton::clicked, this, [this]() {
        if (m_deviceCombo->currentIndex() > 0) {
            loadMeasurementJobs(m_devices[m_deviceCombo->currentIndex() - 1].deviceId);
        }
    });
    measRow->addWidget(m_refreshJobsBtn);
    jobLayout->addRow(measLabel, measRow);

    // SDX4 设备状态标签
    m_deviceStatusLabel = new QLabel;
    m_deviceStatusLabel->setStyleSheet(QStringLiteral(
        "font-size: 11px; padding: 4px 8px; border-radius: 4px; background: #f5f7fa;"));
    m_deviceStatusLabel->setWordWrap(true);
    jobLayout->addRow(m_deviceStatusLabel);

    // 测量数据卡片
    m_jobCard = new QGroupBox;
    m_jobCard->setStyleSheet(QStringLiteral(
        "QGroupBox { border: 1px solid #e4e7ed; border-radius: 6px; background: #fafafa; "
        "margin-top: 4px; padding: 8px; }"));
    auto *cardLayout = new QVBoxLayout(m_jobCard);
    cardLayout->setContentsMargins(8, 8, 8, 8);
    cardLayout->setSpacing(4);

    // 预览图（多张角度图合成到一张，无间隙）
    m_jobPreviewLabel = new QLabel;
    m_jobPreviewLabel->setFixedHeight(120);
    m_jobPreviewLabel->setAlignment(Qt::AlignCenter);
    m_jobPreviewLabel->setStyleSheet(QStringLiteral(
        "background: #f8fafc; border: 1px solid #dcdfe6; border-radius: 6px;"));
    m_jobPreviewLabel->setText(T("autosc_no_preview"));
    cardLayout->addWidget(m_jobPreviewLabel);

    m_jobGranularityLabel = new QLabel(T("autosc_granularity_none"));
    m_jobSparkle15Label = new QLabel(T("autosc_front_sparkle_none"));
    m_jobSparkle80Label = new QLabel(T("autosc_side_sparkle_none"));
    for (auto *lbl : {m_jobGranularityLabel, m_jobSparkle15Label, m_jobSparkle80Label}) {
        lbl->setStyleSheet(QStringLiteral("color: #606266; font-size: 11px;"));
        cardLayout->addWidget(lbl);
    }

    m_jobCard->setVisible(false);
    jobLayout->addWidget(m_jobCard);

    layout->addWidget(jobGroup);

    // ── 分隔线 ──
    auto *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("color: #e4e7ed;"));
    layout->addWidget(line);

    // ── 搜色类型 ──
    auto *typeGroup = new QGroupBox(T("autosc_search_type"));
    auto *typeLayout = new QVBoxLayout(typeGroup);

    auto *radioRow = new QHBoxLayout;
    m_solidRadio = new QRadioButton(T("solid"));
    m_metalRadio = new QRadioButton(T("metallic"));
    m_solidRadio->setChecked(true);
    radioRow->addWidget(m_solidRadio);
    radioRow->addWidget(m_metalRadio);

    m_threeStepCheck = new QCheckBox(T("autosc_three_step_only"));
    m_threeStepCheck->setVisible(false);
    radioRow->addWidget(m_threeStepCheck);
    radioRow->addStretch();
    typeLayout->addLayout(radioRow);

    connect(m_metalRadio, &QRadioButton::toggled, this, [this](bool checked) {
        m_threeStepCheck->setVisible(checked);
    });

    layout->addWidget(typeGroup);

    // ── 筛选 ──
    auto *filterGroup = new QGroupBox(T("autosc_filter"));
    auto *filterLayout = new QFormLayout(filterGroup);
    filterLayout->setContentsMargins(8, 12, 8, 8);
    filterLayout->setSpacing(6);
    filterLayout->setLabelAlignment(Qt::AlignRight);
    filterLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    auto makeCombo = [applyComboStyle]() {
        auto *c = new QComboBox;
        applyComboStyle(c);
        return c;
    };
    auto makeEdit = []() {
        auto *e = new QLineEdit;
        e->setStyleSheet(QStringLiteral(
            "QLineEdit { border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px 8px; "
            "font-size: 12px; background: #fff; min-height: 20px; color: #333; }"));
        e->setMinimumWidth(280);
        return e;
    };

    m_brandCombo = makeCombo();
    connect(m_brandCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AutoScPage::onBrandChanged);
    filterLayout->addRow(T("autosc_brand"), m_brandCombo);

    m_seriesCombo = makeCombo();
    m_seriesCombo->setEnabled(false);
    connect(m_seriesCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AutoScPage::onSeriesChanged);
    filterLayout->addRow(T("autosc_series"), m_seriesCombo);

    m_modelCombo = makeCombo();
    m_modelCombo->setEnabled(false);
    filterLayout->addRow(T("autosc_vehicle_model"), m_modelCombo);

    m_yearEdit = makeEdit();
    filterLayout->addRow(T("autosc_year"), m_yearEdit);

    m_colorNameEdit = makeEdit();
    filterLayout->addRow(T("autosc_color_name_brief"), m_colorNameEdit);

    m_colorCodeEdit = makeEdit();
    filterLayout->addRow(T("autosc_color_code"), m_colorCodeEdit);

    layout->addWidget(filterGroup);

    // ── 按钮 ──
    auto *btnRow = new QHBoxLayout;
    m_resetBtn = new QPushButton(T("autosc_reset"));
    m_resetBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border: 1px solid #dcdfe6; border-radius: 4px; padding: 8px 16px; "
        "background: #fff; font-size: 12px; color: #606266; }"
        "QPushButton:hover { border-color: #409eff; color: #409eff; }"));
    connect(m_resetBtn, &QPushButton::clicked, this, &AutoScPage::resetFilters);

    m_searchBtn = new QPushButton(T("sc_search_btn"));
    m_searchBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border: none; border-radius: 4px; padding: 8px 20px; "
        "background: #409eff; font-size: 12px; color: #fff; font-weight: bold; }"
        "QPushButton:hover { background: #66b1ff; }"));
    connect(m_searchBtn, &QPushButton::clicked, this, &AutoScPage::doSearch);

    btnRow->addWidget(m_resetBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_searchBtn);
    btnRow->addSpacing(30);  // 搜索按钮向左偏移
    layout->addLayout(btnRow);

    layout->addStretch();

    // 内容变化（换行标签变高、测量数据卡片显隐）后按当前宽度补最小高度，
    // 否则滚动区范围偏小，划不到底部的搜色按钮
    container->installEventFilter(this);
    scroll->setWidget(container);

    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->addWidget(scroll);
}

// ─── 右侧主区域 ──────────────────────────────────────────────
void AutoScPage::setupMainArea(QWidget *mainWidget)
{
    // 外层布局：直接放 QScrollArea
    auto *outerLayout = new QVBoxLayout(mainWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    m_mainScroll = new QScrollArea;
    m_mainScroll->setWidgetResizable(true);   // 宽度自适应视口，高度受minimumHeight控制
    m_mainScroll->setFrameShape(QFrame::NoFrame);
    m_mainScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_mainScroll->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }")
        + QLatin1String(kScrollBarQss));

    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(0);

    // ── 搜索结果模块 ──
    auto *resultPanel = new ResizablePanel(T("autosc_result_list"), true /*collapsible*/);
    auto *resultLayout = resultPanel->contentLayout();

    m_resultCountLabel = new QLabel;
    m_resultCountLabel->setStyleSheet(QStringLiteral("color: #909399; font-size: 12px;"));
    resultLayout->addWidget(m_resultCountLabel);

    m_resultTable = new QTableWidget;
    // 列标题与数值列宽度对标前端 visibleResultColumnOrder；
    // 「颜色属性」按使用方要求排在三个测量纹理列（粗糙度/正侧面闪烁度）之后，
    // 「颜色偏向」排在车型之后
    m_resultTable->setColumnCount(14);
    m_resultTable->setHorizontalHeaderLabels({
        T("autosc_job_formula_col"),
        T("autosc_sample_no"),
        T("sc_name"),
        T("autosc_score_o"),
        T("autosc_score_ac"),
        T("autosc_roughness"),
        T("autosc_front_sparkle"),
        T("autosc_side_sparkle"),
        T("autosc_color_attr"),
        T("autosc_car_brand"),
        T("sc_color_name"),
        T("autosc_color_code"),
        T("autosc_vehicle_model"),
        T("autosc_color_bias")
    });
    // 数值列右对齐（与前端 align="right" 一致），文本列左对齐
    auto *hdr = m_resultTable->horizontalHeader();
    const QList<QPair<int, int>> colWidths = {
        {0, 90}, {1, 100}, {2, 110}, {3, 70}, {4, 70}, {5, 60},
        {6, 80}, {7, 80}, {8, 110}, {9, 70}, {10, 80}, {11, 60}, {12, 80}, {13, 80}
    };
    for (const auto &cw : colWidths) m_resultTable->setColumnWidth(cw.first, cw.second);
    hdr->setStretchLastSection(true);
    hdr->setSectionResizeMode(QHeaderView::Interactive);
    hdr->setHighlightSections(false);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->setMinimumHeight(80);
    m_resultTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_resultTable->verticalHeader()->setDefaultSectionSize(120);   // 行高适配对比卡片
    m_resultTable->setStyleSheet(QStringLiteral(
        "QTableWidget { border: 1px solid #e4e7ed; border-radius: 4px; background: #fff; "
        "gridline-color: #ebeef5; font-size: 12px; color: #333; }"
        "QTableWidget::item:selected { background: #e8f0fe; color: #0d4f8b; }"
        "QHeaderView::section { background: #f5f7fa; border: none; border-bottom: 1px solid #e4e7ed; "
        "padding: 6px 8px; font-size: 12px; color: #606266; }")
        + QLatin1String(kScrollBarQss));

    // “作业 | 配方”对比列渲染器（对齐前端 ScJobFormulaCompare）
    m_logoPixmap = QPixmap(QStringLiteral(":/logo.png"));
    auto *compareDelegate = new JobFormulaCompareDelegate(m_resultTable);
    compareDelegate->logo = m_logoPixmap;
    compareDelegate->resolver = [this](const QString &url) -> QPixmap {
        requestPreviewImage(url);              // 幂等：未缓存则触发异步下载
        return m_previewImgCache.value(url);
    };
    m_resultTable->setItemDelegateForColumn(0, compareDelegate);

    connect(m_resultTable, &QTableWidget::currentCellChanged,
            this, [this](int row, int /*col*/, int /*prevRow*/, int /*prevCol*/) {
        onResultRowSelected(row);
    });
    resultLayout->addWidget(m_resultTable);

    // 搜索结果加载转圈：覆盖在表格 viewport 中央（macOS 风格）
    m_resultSpinner = new SpinnerOverlay(m_resultTable->viewport());

    // ── 配方模块 ──
    auto *recipePanel = new ResizablePanel(T("autosc_recipe"), true /*collapsible*/);
    m_recipePanel = recipePanel; // 保存为成员变量，供搜索结果选中时展开
    auto *recipeLayout = recipePanel->contentLayout();
    recipeLayout->setSpacing(8);

    // 元信息栏：配方号 / 涂层类型 / 分数(O) / 分数(AC) …… 右侧罐容量
    const QString kLabStyle = QStringLiteral("color: #909399; font-size: 12px;");
    const QString kValStyle = QStringLiteral("color: #303133; font-size: 13px;");
    auto *metaRow = new QHBoxLayout;
    metaRow->setSpacing(6);

    auto addMetaPair = [&metaRow, &kLabStyle, &kValStyle](const QString &lab, QLabel *&valWidget) {
        auto *l = new QLabel(lab);
        l->setStyleSheet(kLabStyle);
        valWidget = new QLabel(QStringLiteral("—"));
        valWidget->setStyleSheet(kValStyle);
        metaRow->addWidget(l);
        metaRow->addWidget(valWidget);
        metaRow->addSpacing(14);
    };

    addMetaPair(T("autosc_meta_recipe"), m_recipeMetaLabel);
    addMetaPair(T("autosc_meta_coat_type"), m_recipeCoatTypeLabel);

    // 分数(O)/(AC)：加粗 + 阈值着色（.score-o / .score-ac）
    auto *scoreOLab = new QLabel(T("autosc_meta_score_o"));
    scoreOLab->setStyleSheet(kLabStyle);
    m_recipeScoreOLabel = new QLabel(QStringLiteral("—"));
    m_recipeScoreOLabel->setStyleSheet(QStringLiteral("color: #303133; font-size: 13px; font-weight: 700;"));
    auto *scoreAcLab = new QLabel(T("autosc_meta_score_ac"));
    scoreAcLab->setStyleSheet(kLabStyle);
    m_recipeScoreAcLabel = new QLabel(QStringLiteral("—"));
    m_recipeScoreAcLabel->setStyleSheet(QStringLiteral("color: #303133; font-size: 13px; font-weight: 700;"));
    metaRow->addWidget(scoreOLab);
    metaRow->addWidget(m_recipeScoreOLabel);
    metaRow->addSpacing(14);
    metaRow->addWidget(scoreAcLab);
    metaRow->addWidget(m_recipeScoreAcLabel);

    // 分数(AC-Edited)：仅在当前行有编辑预测结果时整组显示
    {
        auto *editedGroup = new QWidget;
        auto *editedLayout = new QHBoxLayout(editedGroup);
        editedLayout->setContentsMargins(0, 0, 0, 0);
        editedLayout->setSpacing(6);
        auto *editedLab = new QLabel(T("autosc_meta_score_ac_edited"));
        editedLab->setStyleSheet(kLabStyle);
        m_recipeScoreAcEditedLabel = new QLabel(QStringLiteral("—"));
        m_recipeScoreAcEditedLabel->setStyleSheet(
            QStringLiteral("color: #303133; font-size: 13px; font-weight: 700;"));
        editedLayout->addWidget(editedLab);
        editedLayout->addWidget(m_recipeScoreAcEditedLabel);
        editedGroup->setVisible(false);
        metaRow->addWidget(editedGroup);
        metaRow->addSpacing(14);
        m_recipeScoreAcEditedGroup = editedGroup;
    }

    // 品牌（色母编号体系）下拉
    metaRow->addSpacing(14);
    auto *pigBrandLab = new QLabel(T("autosc_meta_brand"));
    pigBrandLab->setStyleSheet(kLabStyle);
    m_pigmentSystemCombo = new QComboBox;
    m_pigmentSystemCombo->setStyleSheet(QStringLiteral(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px 8px; "
        "font-size: 12px; background: #fff; min-height: 20px; color: #333; }"
        "QComboBox:hover { border-color: #c0c4cc; }"
        "QComboBox QAbstractItemView { border: 1px solid #dcdfe6; background: #fff; "
        "color: #333; selection-background-color: #e8f0fe; selection-color: #0d4f8b; }"));
    m_pigmentSystemCombo->setMinimumWidth(110);
    // 选项与前端 PIGMENT_CODE_SYSTEM_OPTIONS 一致：值=编号体系，内部为空串
    m_pigmentSystemCombo->addItem(T("autosc_pigment_internal"), QStringLiteral(""));
    m_pigmentSystemCombo->addItem(T("autosc_pigment_mengying"), QStringLiteral("mengying"));
    m_pigmentSystemCombo->addItem(T("autosc_pigment_xiongtu"), QStringLiteral("xiongtu"));
    m_pigmentSystemCombo->addItem(T("autosc_pigment_kaiqi"), QStringLiteral("kaiqi"));
    m_pigmentSystemCombo->addItem(T("autosc_pigment_shikasi"), QStringLiteral("shikasi"));
    {
        const int di = m_pigmentSystemCombo->findData(m_pigmentCodeSystem);
        m_pigmentSystemCombo->setCurrentIndex(di >= 0 ? di : 1);   // 默认猛鹰
    }
    connect(m_pigmentSystemCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) { onPigmentSystemChanged(index); });
    metaRow->addWidget(pigBrandLab);
    metaRow->addWidget(m_pigmentSystemCombo);

    metaRow->addStretch();

    // 下载配方按钮（三工序：底层+面层；非三工序：单个）
    auto styleDownloadBtn = [](QPushButton *btn) {
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { border: 1px solid #409eff; border-radius: 4px; padding: 5px 15px; "
            "background: #fff; font-size: 12px; color: #409eff; }"
            "QPushButton:hover { background: #ecf5ff; }"
            "QPushButton:disabled { background: #f5f7fa; border-color: #dcdfe6; color: #c0c4cc; }"));
    };
    auto createDownloadMenu = [this](const QString &layerKey) {
        auto *menu = new QMenu(this);
        menu->addAction(QStringLiteral(".xml"), this, [this, layerKey]() { onDownloadFormula(layerKey); });
        menu->addAction(QStringLiteral(".dat"), this, [this, layerKey]() { onDownloadFormula(layerKey); });
        menu->addAction(QStringLiteral(".rez"), this, [this, layerKey]() { onDownloadFormula(layerKey); });
        menu->addAction(QStringLiteral(".txt"), this, [this, layerKey]() { onDownloadFormula(layerKey); });
        return menu;
    };
    m_downloadBaseBtn = new QPushButton(T("autosc_download_base"));
    styleDownloadBtn(m_downloadBaseBtn);
    m_downloadBaseBtn->setMenu(createDownloadMenu(QStringLiteral("base")));
    m_downloadBaseBtn->setVisible(false);

    m_downloadSurfaceBtn = new QPushButton(T("autosc_download_surface"));
    styleDownloadBtn(m_downloadSurfaceBtn);
    m_downloadSurfaceBtn->setMenu(createDownloadMenu(QStringLiteral("surface")));
    m_downloadSurfaceBtn->setVisible(false);

    m_downloadSingleBtn = new QPushButton(T("autosc_download_formula"));
    styleDownloadBtn(m_downloadSingleBtn);
    m_downloadSingleBtn->setMenu(createDownloadMenu(QStringLiteral("single")));
    m_downloadSingleBtn->setVisible(false);

    metaRow->addWidget(m_downloadBaseBtn);
    metaRow->addWidget(m_downloadSurfaceBtn);
    metaRow->addWidget(m_downloadSingleBtn);
    recipeLayout->addLayout(metaRow);

    // 配方信息
    m_recipeInfoLabel = new QLabel(T("autosc_select_result_hint"));
    m_recipeInfoLabel->setStyleSheet(QStringLiteral("color: #909399; font-size: 12px; padding: 2px;"));
    recipeLayout->addWidget(m_recipeInfoLabel);

    // 色母明细表容器（动态创建，根据三工序显示不同布局）
    m_tonerContainer = new QWidget;
    m_tonerContainer->setLayout(new QVBoxLayout(m_tonerContainer));
    m_tonerContainer->layout()->setContentsMargins(0, 0, 0, 0);
    m_tonerContainer->layout()->setSpacing(4);
    recipeLayout->addWidget(m_tonerContainer);

    // 操作按钮：修色 / 智能修色 / 预测
    auto *actionRow = new QHBoxLayout;
    actionRow->setSpacing(8);
    auto styleBtn = [](QPushButton *btn, const QString &color, const QString &hover) {
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { border: 1px solid %1; border-radius: 4px; padding: 5px 15px; "
            "background: %1; font-size: 12px; color: #fff; }"
            "QPushButton:hover { background: %2; }"
            "QPushButton:disabled { background: #c8cdd4; border-color: #c8cdd4; color: #fff; }")
            .arg(color, hover));
    };
    m_correctBtn = new QPushButton(T("autosc_correct"));
    styleBtn(m_correctBtn, QStringLiteral("#e6a23c"), QStringLiteral("#ebb563"));
    connect(m_correctBtn, &QPushButton::clicked, this, &AutoScPage::onCorrect);

    m_smartCorrectBtn = new QPushButton(T("autosc_smart_correct"));
    styleBtn(m_smartCorrectBtn, QStringLiteral("#409eff"), QStringLiteral("#66b1ff"));
    connect(m_smartCorrectBtn, &QPushButton::clicked, this, &AutoScPage::onSmartCorrect);

    m_editBtn = new QPushButton(T("autosc_edit"));
    styleBtn(m_editBtn, QStringLiteral("#909399"), QStringLiteral("#a6a9ad"));
    m_editBtn->setEnabled(false);   // 选中配方行后才可用
    connect(m_editBtn, &QPushButton::clicked, this, &AutoScPage::onEditFormula);

    // 编辑态的两个按钮：取消丢弃草稿，确认按草稿发起预测
    m_cancelEditBtn = new QPushButton(T("autosc_cancel_edit"));
    styleBtn(m_cancelEditBtn, QStringLiteral("#909399"), QStringLiteral("#a6a9ad"));
    m_cancelEditBtn->setVisible(false);
    connect(m_cancelEditBtn, &QPushButton::clicked, this, &AutoScPage::cancelEditFormula);

    m_confirmEditBtn = new QPushButton(T("autosc_confirm_edit"));
    styleBtn(m_confirmEditBtn, QStringLiteral("#409eff"), QStringLiteral("#66b1ff"));
    m_confirmEditBtn->setVisible(false);
    connect(m_confirmEditBtn, &QPushButton::clicked, this, &AutoScPage::confirmEditFormula);

    m_measureBtn = new QPushButton(T("autosc_measure"));
    styleBtn(m_measureBtn, QStringLiteral("#67c23a"), QStringLiteral("#85ce61"));
    connect(m_measureBtn, &QPushButton::clicked, this, &AutoScPage::onMeasure);

    m_saveBtn = new QPushButton(T("formula_save"));
    styleBtn(m_saveBtn, QStringLiteral("#e6a23c"), QStringLiteral("#ebb563"));
    m_saveBtn->setEnabled(false);  // 初始状态禁用，测量后才可点击
    connect(m_saveBtn, &QPushButton::clicked, this, &AutoScPage::onSaveFormula);

    actionRow->addWidget(m_editBtn);
    actionRow->addWidget(m_cancelEditBtn);
    actionRow->addWidget(m_confirmEditBtn);
    actionRow->addWidget(m_measureBtn);
    actionRow->addWidget(m_saveBtn);
    actionRow->addWidget(m_correctBtn);
    actionRow->addWidget(m_smartCorrectBtn);
    actionRow->addStretch();
    recipeLayout->addLayout(actionRow);

    // ── 上下可拖拽分隔条（面板之间）──
    // 记录可拖拽面板 + 设置逻辑最小高度（因为 setFixedHeight 会同时覆盖 minimumHeight）
    m_panelWidgets.clear();
    m_panelWidgets.append(resultPanel);   // index 0
    m_panelWidgets.append(recipePanel);   // index 1
    resultPanel->setProperty("logicalMinH", 120);
    recipePanel->setProperty("logicalMinH", 120);

    // 连接面板高度变化信号
    auto applyHeightDelta = [this](int index, int delta) {
        if (index < 0 || index >= m_panelWidgets.size()) return;
        auto *p = m_panelWidgets[index];
        const int newH = p->height() + delta;
        const int minH = p->property("logicalMinH").toInt();
        if (newH < minH) return;
        p->setFixedHeight(newH);
        updateScrollContainerHeight();
    };
    connect(resultPanel, &ResizablePanel::heightChanged, this, [applyHeightDelta](int d) { applyHeightDelta(0, d); });
    connect(recipePanel, &ResizablePanel::heightChanged, this, [applyHeightDelta](int d) { applyHeightDelta(1, d); });
    // 搜索结果/配方面板折叠/展开时重新计算容器总高度
    connect(resultPanel, &ResizablePanel::collapsedToggled, this, [this]() {
        updateScrollContainerHeight();
    });
    connect(recipePanel, &ResizablePanel::collapsedToggled, this, [this]() {
        updateScrollContainerHeight();
    });

    layout->addWidget(resultPanel);
    layout->addSpacing(8);
    layout->addWidget(recipePanel);

    // ── 可视化模块 ──
    auto *vizPanel = new ResizablePanel(T("autosc_visualization"));
    m_vizPanel = vizPanel;
    auto *vizLayout = vizPanel->contentLayout();
    m_vizLayout = vizLayout;

    m_vizInfoLabel = new QLabel(T("autosc_viz_wait_job"));
    m_vizInfoLabel->setStyleSheet(QStringLiteral("color: #909399; font-size: 12px; padding: 4px;"));
    vizLayout->addWidget(m_vizInfoLabel);

    // 默认添加一个卡片（光谱曲线图，对标前端默认展示）
    addVizCard(QStringLiteral("spectral_curve"));

    m_panelWidgets.append(vizPanel);    // index 2
    vizPanel->setProperty("logicalMinH", 80);
    connect(vizPanel, &ResizablePanel::heightChanged, this, [this](int delta) {
        if (m_panelWidgets.size() > 2) {
            auto *p = m_panelWidgets[2];
            int newH = p->height() + delta;
            int minH = p->property("logicalMinH").toInt();
            if (newH >= minH) {
                p->setFixedHeight(newH);
                updateScrollContainerHeight();
            }
        }
    });

    layout->addSpacing(8);
    layout->addWidget(vizPanel);

    // 设置面板初始固定高度（可独立拖拽调整）
    // 注意：不要先 setMinimumHeight 再 setFixedHeight，setFixedHeight 会覆盖 minimumHeight
    // 配方面板高度自适应内容（不写死固定高度）
    resultPanel->setFixedHeight(350);
    vizPanel->setFixedHeight(280);

    // 配方面板默认折叠（会重新固定高度，且内部会发射 collapsedToggled，但此时信号连接未安装，需手动重新计算）
    recipePanel->toggleCollapse();

    // 容器高度 = 所有面板 + 间距 + 边距（使用 minimumHeight 获取 setFixedHeight 同步值）
    int totalH = 12 + 12;  // 上下边距
    for (auto *p : m_panelWidgets) totalH += p->minimumHeight();
    totalH += 8 * 2;  // 两个 8px 间距
    // 宽度自适应：不设定 fixedWidth，仅设定 minimumWidth 为 0
    container->setMinimumWidth(0);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // 锁定容器高度（min+max），避免 widgetResizable 拉伸导致内部空白
    container->setMinimumHeight(totalH);
    container->setMaximumHeight(totalH);

    m_mainScroll->setWidget(container);
    outerLayout->addWidget(m_mainScroll);
}

void AutoScPage::updateScrollContainerHeight()
{
    auto *container = m_mainScroll ? m_mainScroll->widget() : nullptr;
    if (!container) return;
    int totalH = 12 + 12;  // 上下边距
    for (auto *p : m_panelWidgets) totalH += p->minimumHeight(); // 使用 minimumHeight 保证同步
    totalH += 8 * 2;  // 两个 8px 间距
    // 同时锁定 min 和 max，避免 widgetResizable 把容器拉伸到视口高度造成布局内部空白
    container->setMinimumHeight(totalH);
    container->setMaximumHeight(totalH);
    if (auto *lyt = container->layout()) {
        lyt->invalidate();
        lyt->activate();
    }
    container->updateGeometry();
    // 保持当前浏览位置：面板高度变化时只把越界的偏移夹回最大值，
    // 换数据集（清空配方面板等）由调用方显式回到顶部。
    auto *bar = m_mainScroll->verticalScrollBar();
    bar->setValue(qMin(bar->value(), bar->maximum()));
}

/**
 * 左栏内容高度同步：QScrollArea 的 widgetResizable 只按 sizeHint/minimumSize 给高度，
 * 换行标签等「按宽度求高度」的内容变高时不计入，滚动范围偏小，底部搜色按钮划不到
 * （拖动窗口后才恢复）。这里按当前视口宽度显式补 minimumHeight。
 */
void AutoScPage::syncSidebarContentHeight()
{
    if (!m_sidebarScroll || !m_sidebarContainer) return;
    auto *lyt = m_sidebarContainer->layout();
    const int viewportW = m_sidebarScroll->viewport()->width();
    if (!lyt || viewportW <= 0) return;
    // 先把显隐/文案变化算进布局：刚显示的测量数据卡片只有激活后会计入 sizeHint
    lyt->activate();
    const int needed = std::max(lyt->sizeHint().height(), lyt->totalHeightForWidth(viewportW));
    if (needed > 0 && m_sidebarContainer->minimumHeight() != needed)
        m_sidebarContainer->setMinimumHeight(needed);
}

void AutoScPage::scheduleSidebarSync()
{
    if (m_sidebarSyncPending) return;
    m_sidebarSyncPending = true;
    // 合并同一轮内的多次布局变化，避免在 LayoutRequest 里直接改尺寸造成递归
    QTimer::singleShot(0, this, [this]() {
        m_sidebarSyncPending = false;
        syncSidebarContentHeight();
    });
}

bool AutoScPage::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_sidebarContainer
        && (event->type() == QEvent::LayoutRequest || event->type() == QEvent::Resize)) {
        scheduleSidebarSync();
        return false;   // 事件仍交给容器默认处理
    }
    return QWidget::eventFilter(watched, event);
}

// ─── 数据加载 ────────────────────────────────────────────────
void AutoScPage::loadInitialData()
{
    // 加载梯度数据
    ScApi::instance().getCalsets(1, 100,
        [this](const QJsonObject &res) {
            m_calsets.clear();
            QJsonArray rows = res[QStringLiteral("rows")].toArray();
            if (rows.isEmpty()) rows = res[QStringLiteral("data")].toArray();
            m_calsetCombo->clear();
            m_calsetCombo->addItem(T("autosc_select_gradient"), QString());
            for (const auto &v : rows) {
                auto obj = v.toObject();
                CalsetItem item;
                item.id = QString::number(obj[QStringLiteral("id")].toInt());
                item.name = obj[QStringLiteral("calsetName")].toString();
                item.isDefault = obj[QStringLiteral("isDefault")].toBool();
                item.alias = obj[QStringLiteral("alias")].toString().trimmed().toUpper();
                m_calsets.append(item);
                QString label = item.name;
                if (item.isDefault) label += T("autosc_default_suffix");
                m_calsetCombo->addItem(label, item.id);
            }
        },
        [](const QString &msg, int) {
            qWarning() << "加载梯度数据失败:" << msg;
        });

    // 加载设备
    loadDevices();

    // 加载品牌
    loadBrands();

    // 加载 S&C 分数阈值显示配置
    loadDisplaySettings();
}

// ─── 分数显示配置 ────────────────────────────────────────────
bool AutoScPage::isEffectSearch() const
{
    // 与前端 searchContext.paintCategory === 'METALLIC' 一致
    return m_metalRadio && m_metalRadio->isChecked();
}

QString AutoScPage::scoreTone(double score, const ScoreThreshold &th)
{
    // 与前端 resolveScScoreTone 一致：分数越小越好
    if (!std::isfinite(score)) return QString();
    if (th.green >= 0 && score <= th.green) return QStringLiteral("green");
    if (th.yellow >= 0 && score <= th.yellow) return QStringLiteral("yellow");
    if (th.green >= 0 || th.yellow >= 0) return QStringLiteral("red");
    return QString();
}

void AutoScPage::loadDisplaySettings()
{
    // 与前端 loadEffectiveScDisplaySettings 一致：全局配置 + 用户配置（启用时优先）
    auto global = QSharedPointer<QJsonObject>::create();
    auto applyEffective = QSharedPointer<std::function<void(const QJsonObject &)>>::create();

    *applyEffective = [this](const QJsonObject &settings) {
        auto readNum = [&settings](const QString &key, bool *ok) -> double {
            if (!settings.contains(key)) { if (ok) *ok = false; return -1; }
            const auto v = settings.value(key);
            if (v.isDouble()) { if (ok) *ok = true; return v.toDouble(); }
            bool b = false;
            double d = v.toString().toDouble(&b);
            if (ok) *ok = b;
            return b ? d : -1;
        };
        auto readThreshold = [&readNum](const QString &prefix, const QString &stage) {
            ScoreThreshold th;
            bool ok = false;
            double g = readNum(prefix + stage + QStringLiteral("Green"), &ok);
            if (ok) th.green = g;
            ok = false;
            double y = readNum(prefix + stage + QStringLiteral("Yellow"), &ok);
            if (ok) th.yellow = y;
            return th;
        };
        m_solidInitial    = readThreshold(QStringLiteral("solid"),  QStringLiteral("Initial"));
        m_solidCorrected  = readThreshold(QStringLiteral("solid"),  QStringLiteral("Corrected"));
        m_effectInitial   = readThreshold(QStringLiteral("effect"), QStringLiteral("Initial"));
        m_effectCorrected = readThreshold(QStringLiteral("effect"), QStringLiteral("Corrected"));

        bool ok = false;
        double dp = readNum(QStringLiteral("scoreDecimalPlaces"), &ok);
        m_scoreDecimalPlaces = ok ? qBound(0, int(dp), 6) : 2;
    };

    // 全局配置失败时仍继续拉用户配置，与前端 Promise.all + catch 等价
    auto fetchMine = [global, applyEffective](const QString & /*errMsg*/, int) {
        ScApi::instance().getScMineSettings(
            [global, applyEffective](const QJsonObject &mres) {
                // 全局接口已失败，仅能使用启用状态下的用户配置
                if (isScMineSettingsEnabled(mres)) {
                    QJsonObject eff; mergeScDisplayKeys(eff, mres);
                    (*applyEffective)(eff);
                }
            },
            [](const QString &msg, int) {
                qWarning() << "加载用户 S&C 配置失败:" << msg;
            });
    };

    ScApi::instance().getScSettings(
        [global, applyEffective](const QJsonObject &res) {
            mergeScDisplayKeys(*global, res);
            ScApi::instance().getScMineSettings(
                [global, applyEffective](const QJsonObject &mres) {
                    QJsonObject effective = *global;
                    // 与前端一致：仅当用户配置 enabled 时覆盖全局配置
                    if (isScMineSettingsEnabled(mres)) mergeScDisplayKeys(effective, mres);
                    (*applyEffective)(effective);
                },
                [global, applyEffective](const QString &msg, int) {
                    qWarning() << "加载用户 S&C 配置失败:" << msg;
                    (*applyEffective)(*global);
                });
        },
        [fetchMine](const QString &msg, int) {
            qWarning() << "加载 S&C 全局配置失败:" << msg;
            fetchMine(msg, 0);
        });
}

void AutoScPage::loadDevices()
{
    ScApi::instance().getMyDevices(
        [this](const QJsonObject &res) {
            m_devices.clear();
            QJsonArray rows = res[QStringLiteral("rows")].toArray();
            if (rows.isEmpty()) rows = res[QStringLiteral("data")].toArray();
            // 断开信号避免干扰
            disconnect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                       this, &AutoScPage::onDeviceChanged);
            m_deviceCombo->clear();
            m_deviceCombo->addItem(T("autosc_select_device"), QString());
            for (const auto &v : rows) {
                auto obj = v.toObject();
                DeviceItem item;
                item.id = QString::number(obj[QStringLiteral("id")].toInt());
                // 与前端一致：deviceId ?? serialNo，都为空则回退到 id
                item.deviceId = obj[QStringLiteral("deviceId")].toString();
                if (item.deviceId.isEmpty())
                    item.deviceId = obj[QStringLiteral("serialNo")].toString();
                if (item.deviceId.isEmpty())
                    item.deviceId = item.id;
                // 设备类型：deviceType ?? model
                item.model = obj[QStringLiteral("deviceType")].toString();
                if (item.model.isEmpty())
                    item.model = obj[QStringLiteral("model")].toString();
                m_devices.append(item);
                // 与前端 deviceLabel() 一致：type 存在则 "序号 (类型)"，否则仅序号
                QString label = item.model.isEmpty()
                    ? item.deviceId
                    : QStringLiteral("%1 (%2)").arg(item.deviceId, item.model);
                m_deviceCombo->addItem(label, item.deviceId);
            }
            // 重新连接信号
            connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &AutoScPage::onDeviceChanged);
        },
        [](const QString &msg, int) {
            qWarning() << "加载设备列表失败:" << msg;
        });
}

void AutoScPage::loadMeasurementJobs(const QString &deviceId)
{
    m_measurementCombo->clear();
    m_measurementCombo->addItem(T("sc_select_measurement_job"), QString());
    m_measurementCombo->setEnabled(true);
    m_refreshJobsBtn->setEnabled(true);
    m_jobs.clear();
    clearJobCard();

    ScApi::instance().getMeasurementJobs(deviceId,
        [this](const QJsonObject &res) {
            QJsonArray rows = res[QStringLiteral("rows")].toArray();
            if (rows.isEmpty()) rows = res[QStringLiteral("data")].toArray();
            for (const auto &v : rows) {
                auto obj = v.toObject();
                JobItem item;
                // 与前端一致：id ?? measurementId
                qint64 jobId = obj[QStringLiteral("id")].toVariant().toLongLong();
                if (jobId == 0) jobId = obj[QStringLiteral("measurementId")].toVariant().toLongLong();
                item.id = QString::number(jobId);
                // 标题：title ?? jobName ?? name ?? workName
                item.title = obj[QStringLiteral("title")].toString();
                if (item.title.isEmpty()) item.title = obj[QStringLiteral("jobName")].toString();
                if (item.title.isEmpty()) item.title = obj[QStringLiteral("name")].toString();
                if (item.title.isEmpty()) item.title = obj[QStringLiteral("workName")].toString();
                if (item.title.isEmpty()) item.title = item.id;
                // 色块：swatch ?? hexColor ?? hex ?? colorHex ?? targetHex ?? previewColor
                item.swatch = obj[QStringLiteral("swatch")].toString();
                if (item.swatch.isEmpty()) item.swatch = obj[QStringLiteral("hexColor")].toString();
                if (item.swatch.isEmpty()) item.swatch = obj[QStringLiteral("hex")].toString();
                if (item.swatch.isEmpty()) item.swatch = obj[QStringLiteral("colorHex")].toString();
                if (item.swatch.isEmpty()) item.swatch = obj[QStringLiteral("targetHex")].toString();
                if (item.swatch.isEmpty()) item.swatch = obj[QStringLiteral("previewColor")].toString();
                // 纹理数据：texture.coarseness ?? coarseness, texture.sparkleGradeR15As15, texture.sparkleGradeR15As80
                auto texture = obj[QStringLiteral("texture")].toObject();
                if (texture.isEmpty()) texture = obj[QStringLiteral("targetTexture")].toObject();
                if (!texture.isEmpty()) {
                    item.coarseness = texture[QStringLiteral("coarseness")].toDouble();
                    item.sparkleR15As15 = texture[QStringLiteral("sparkleGradeR15As15")].toDouble();
                    item.sparkleR15As80 = texture[QStringLiteral("sparkleGradeR15As80")].toDouble();
                } else {
                    item.coarseness = obj[QStringLiteral("coarseness")].toDouble();
                }
                m_jobs.append(item);
                m_measurementCombo->addItem(item.title, item.id);
            }
        },
        [](const QString &msg, int) {
            qWarning() << "加载测量作业失败:" << msg;
        });
}

void AutoScPage::loadBrands()
{
    FormulaApi::instance().getBrands(1, 200,
        [this](const QJsonObject &res) {
            m_brands.clear();
            QJsonArray rows = res[QStringLiteral("rows")].toArray();
            if (rows.isEmpty()) rows = res[QStringLiteral("data")].toArray();
            m_brandCombo->clear();
            m_brandCombo->addItem(T("autosc_all"), QString());
            for (const auto &v : rows) {
                auto obj = v.toObject();
                BrandItem item;
                item.id = QString::number(obj[QStringLiteral("id")].toInt());
                item.name = obj[QStringLiteral("brandName")].toString();
                item.nameEn = obj[QStringLiteral("brandNameEn")].toString();
                m_brands.append(item);
                m_brandCombo->addItem(Tn(item.name, item.nameEn), item.id);
            }
        },
        [](const QString &msg, int) {
            qWarning() << "加载品牌失败:" << msg;
        });
}

void AutoScPage::onPigmentSystemChanged(int index)
{
    if (index < 0 || !m_pigmentSystemCombo) return;
    m_pigmentCodeSystem = m_pigmentSystemCombo->itemData(index).toString();
    // 已选中配方时，按新编号体系重新拉取配方详情（对标前端 onPigmentCodeSystemChange）
    if (!m_currentRowObj.isEmpty()) {
        loadRecipePanel(m_currentRowObj);
    }
}

void AutoScPage::loadSeries(const QString &brandId)
{
    m_seriesCombo->clear();
    m_seriesCombo->addItem(T("autosc_all"), QString());
    m_modelCombo->clear();
    m_modelCombo->addItem(T("autosc_all"), QString());
    m_modelCombo->setEnabled(false);

    if (brandId.isEmpty()) {
        m_seriesCombo->setEnabled(false);
        return;
    }

    if (m_seriesCache.contains(brandId)) {
        for (const auto &s : m_seriesCache[brandId]) {
            m_seriesCombo->addItem(Tn(s.name, s.nameEn), s.id);
        }
        m_seriesCombo->setEnabled(true);
        return;
    }

    FormulaApi::instance().getSeriesByBrand(brandId,
        [this, brandId](const QJsonObject &res) {
            QList<SeriesItem> list;
            QJsonArray rows = res[QStringLiteral("rows")].toArray();
            if (rows.isEmpty()) rows = res[QStringLiteral("data")].toArray();
            for (const auto &v : rows) {
                auto obj = v.toObject();
                SeriesItem item;
                item.id = QString::number(obj[QStringLiteral("id")].toInt());
                item.brandId = brandId;
                item.name = obj[QStringLiteral("seriesName")].toString();
                item.nameEn = obj[QStringLiteral("seriesNameEn")].toString();
                list.append(item);
                m_seriesCombo->addItem(Tn(item.name, item.nameEn), item.id);
            }
            m_seriesCache[brandId] = list;
            m_seriesCombo->setEnabled(true);
        },
        [](const QString &msg, int) {
            qWarning() << "加载车系失败:" << msg;
        });
}

void AutoScPage::loadModels(const QString &seriesId)
{
    m_modelCombo->clear();
    m_modelCombo->addItem(T("autosc_all"), QString());

    if (seriesId.isEmpty()) {
        m_modelCombo->setEnabled(false);
        return;
    }

    if (m_modelCache.contains(seriesId)) {
        for (const auto &m : m_modelCache[seriesId]) {
            m_modelCombo->addItem(Tn(m.name, m.nameEn), m.id);
        }
        m_modelCombo->setEnabled(true);
        return;
    }

    FormulaApi::instance().getModelsBySeries(seriesId, 1, 200,
        [this, seriesId](const QJsonObject &res) {
            QList<ModelItem> list;
            QJsonArray rows = res[QStringLiteral("rows")].toArray();
            if (rows.isEmpty()) rows = res[QStringLiteral("data")].toArray();
            for (const auto &v : rows) {
                auto obj = v.toObject();
                ModelItem item;
                item.id = QString::number(obj[QStringLiteral("id")].toInt());
                item.seriesId = seriesId;
                item.name = obj[QStringLiteral("modelName")].toString();
                item.nameEn = obj[QStringLiteral("modelNameEn")].toString();
                list.append(item);
                m_modelCombo->addItem(Tn(item.name, item.nameEn), item.id);
            }
            m_modelCache[seriesId] = list;
            m_modelCombo->setEnabled(true);
        },
        [](const QString &msg, int) {
            qWarning() << "加载车型失败:" << msg;
        });
}

// ─── 事件处理 ────────────────────────────────────────────────
void AutoScPage::onDeviceChanged(int index)
{
    if (index <= 0) {
        m_measurementCombo->clear();
        m_measurementCombo->setEnabled(false);
        m_refreshJobsBtn->setEnabled(false);
        clearJobCard();
        return;
    }
    QString deviceId = m_devices[index - 1].deviceId;
    loadMeasurementJobs(deviceId);
}

void AutoScPage::onMeasurementChanged(int index)
{
    // 换作业后必须作废旧作业带来的目标色/配方行数据，否则色块卡片会把上一个作业
    // 配方的列带进新作业（对标前端 onMeasurementChange 里的状态重置）。
    m_searchTargetRgbByGeometry = QJsonObject();
    m_rowRgbByGeometry = QJsonObject();
    m_formulaVizData = QJsonObject();

    if (index <= 0) {
        clearJobCard();
        m_currentMeasurementId.clear();
        return;
    }
    if (index - 1 < m_jobs.size()) {
        updateJobCard(m_jobs[index - 1]);
        m_currentMeasurementId = m_jobs[index - 1].id;
        loadPreviewImages(m_currentMeasurementId);
        loadVisualization(m_currentMeasurementId);
    }
}

void AutoScPage::onBrandChanged(int index)
{
    QString brandId = index > 0 ? m_brands[index - 1].id : QString();
    loadSeries(brandId);
}

void AutoScPage::onSeriesChanged(int index)
{
    QString seriesId;
    if (index > 0) {
        int brandIdx = m_brandCombo->currentIndex();
        if (brandIdx > 0 && brandIdx - 1 < m_brands.size()) {
            QString brandId = m_brands[brandIdx - 1].id;
            auto &list = m_seriesCache[brandId];
            if (index - 1 < list.size()) seriesId = list[index - 1].id;
        }
    }
    loadModels(seriesId);
}

void AutoScPage::updateJobCard(const JobItem &job)
{
    m_jobCard->setVisible(true);
    QString hex = job.swatch;
    if (!hex.isEmpty()) {
        if (!hex.startsWith(QStringLiteral("#"))) hex.prepend(QStringLiteral("#"));
        // 预览图未加载时以色块背景占位
        m_jobPreviewLabel->setStyleSheet(QStringLiteral(
            "background: %1; border: 1px solid #dcdfe6; border-radius: 6px;").arg(hex));
        m_currentTargetHex = hex;
    } else {
        m_jobPreviewLabel->setStyleSheet(QStringLiteral(
            "background: #f8fafc; border: 1px solid #dcdfe6; border-radius: 6px;"));
    }
    // 纹理数据：与前端一致，数字格式化为 3 位小数，无数据显示 "—"
    auto fmtTexture = [](double val) -> QString {
        if (val == 0.0) return QStringLiteral("—");
        return QString::number(val, 'f', 3);
    };
    m_jobGranularityLabel->setText(T("autosc_granularity_fmt").arg(fmtTexture(job.coarseness)));
    // 颗粒度大于 1 时自动选择金属色
    if (job.coarseness > 1.0 && !m_metalRadio->isChecked()) {
        m_metalRadio->setChecked(true);
    }
    m_jobSparkle15Label->setText(T("autosc_front_sparkle_fmt").arg(fmtTexture(job.sparkleR15As15)));
    m_jobSparkle80Label->setText(T("autosc_side_sparkle_fmt").arg(fmtTexture(job.sparkleR15As80)));
    // 卡片由隐藏变显示，左栏滚动范围要跟着重算，否则底部搜色按钮划不到
    scheduleSidebarSync();
}

// 把所有已加载的角度图垂直无缝拼接到一张画布上
void AutoScPage::rebuildPreviewPixmap()
{
    QList<QPixmap> loaded;
    for (const auto &pm : m_previewPixmaps) {
        if (!pm.isNull()) loaded.append(pm);
    }
    if (loaded.isEmpty()) {
        m_jobPreviewLabel->setText(T("autosc_no_preview"));
        return;
    }

    int w = m_jobPreviewLabel->width() - 2;   // 扣除边框
    if (w <= 0) w = 260;
    const int h = 118;
    QPixmap canvas(w, h);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    const int bandH = h / loaded.size();
    for (int i = 0; i < loaded.size(); ++i) {
        const QRect target(0, i * bandH, w, (i == loaded.size() - 1) ? h - i * bandH : bandH);
        const QPixmap &pm = loaded[i];
        // cover 缩放：保持比例填满整条，居中裁切
        QSize s = pm.size();
        s.scale(target.size(), Qt::KeepAspectRatioByExpanding);
        QRect source(QPoint((pm.width() - s.width()) / 2,
                            (pm.height() - s.height()) / 2), s);
        painter.drawPixmap(target, pm, source);
    }
    painter.end();
    m_jobPreviewLabel->setText(QString());
    m_jobPreviewLabel->setPixmap(canvas);
}

void AutoScPage::loadPreviewImages(const QString &measurementId)
{
    m_previewPixmaps.clear();
    m_jobPreviewLabel->setText(T("autosc_no_preview"));

    ScApi::instance().getMeasurementPreviewImages(measurementId,
        [this, measurementId](const QJsonObject &res) {
            QJsonArray data = res[QStringLiteral("data")].toArray();
            // 解析为带角度码的条目（对齐前端 parsePreviewImageItems），并按角度排序
            const QList<PreviewItem> items = parsePreviewItems(QJsonValue(data));
            QStringList urls;
            m_jobPreviewUrls.clear();
            m_jobPreviewGeoCodes.clear();
            for (const auto &it : items) {
                urls.append(it.url);
                m_jobPreviewUrls.append(it.url);
                m_jobPreviewGeoCodes.append(it.geometryCode);
            }
            if (urls.isEmpty()) {
                return;
            }
            // 预置 null 占位，保持角度顺序
            m_previewPixmaps = QList<QPixmap>(urls.size());
            for (int i = 0; i < urls.size(); ++i) {
                const QString fullUrl = HttpClient::resourceUrl(urls[i]);
                auto *manager = new QNetworkAccessManager(this);
                connect(manager, &QNetworkAccessManager::finished, this,
                    [this, i, urls, manager](QNetworkReply *reply) {
                        if (reply->error() == QNetworkReply::NoError) {
                            QPixmap pm;
                            if (pm.loadFromData(reply->readAll())) {
                                if (i < m_previewPixmaps.size()) {
                                    m_previewPixmaps[i] = pm;
                                    rebuildPreviewPixmap();
                                }
                                // 同步写入对比列缓存并重绘
                                m_previewImgCache.insert(urls[i], pm);
                                m_previewImgPending.remove(urls[i]);
                                if (m_resultTable) m_resultTable->viewport()->update();
                            }
                        }
                        reply->deleteLater();
                        manager->deleteLater();
                    });
                manager->get(QNetworkRequest(QUrl(fullUrl)));
            }
            // 结果已存在时用最新作业图重建对比列
            if (m_resultTable && m_resultTable->rowCount() > 0)
                rebuildCompareColumns();
        },
        [](const QString &msg, int) {
            qWarning() << "加载预览图失败:" << msg;
        });
}

// ─── “作业 | 配方”对比列：图片下载与配对 ────────────────────
void AutoScPage::requestPreviewImage(const QString &url)
{
    if (url.isEmpty()) return;
    if (m_previewImgCache.contains(url) || m_previewImgPending.contains(url)) return;

    // 惰性创建单一共享下载管理器，所有预览图复用（避免每图 new QNetworkAccessManager 导致线程爆炸）
    if (!m_previewNet) {
        m_previewNet = new QNetworkAccessManager(this);
        connect(m_previewNet, &QNetworkAccessManager::finished, this,
            [this](QNetworkReply *reply) {
                const QString raw = reply->property("_rawUrl").toString();
                if (reply->error() == QNetworkReply::NoError) {
                    QPixmap pm;
                    if (pm.loadFromData(reply->readAll()))
                        m_previewImgCache.insert(raw, pm);
                }
                m_previewImgPending.remove(raw);
                reply->deleteLater();
                if (m_resultTable) m_resultTable->viewport()->update();
                // 色块卡片的图像格同样直接重绘（QPointer 可能已随卡片销毁）
                for (const auto &cell : m_colorBlockCells) {
                    if (cell) cell->update();
                }
            });
    }

    m_previewImgPending.insert(url);
    QNetworkRequest req{QUrl(HttpClient::resourceUrl(url))};
    if (auto *reply = m_previewNet->get(req))
        reply->setProperty("_rawUrl", url);
}

void AutoScPage::applyComparePairsForRow(int row)
{
    if (!m_resultTable) return;
    auto *item = m_resultTable->item(row, 0);
    if (!item) return;

    // 作业侧：从已加载的角度码/URL 重建条目
    QList<PreviewItem> jobItems;
    for (int i = 0; i < m_jobPreviewUrls.size(); ++i) {
        PreviewItem it; it.url = m_jobPreviewUrls[i];
        it.geometryCode = (i < m_jobPreviewGeoCodes.size()) ? m_jobPreviewGeoCodes[i] : QString();
        it.sortOrder = i;
        if (!it.url.isEmpty()) jobItems.append(it);
    }
    // 配方侧：从本行原始角色重建
    const QStringList rawUrls = item->data(RoleFormulaRawUrls).toStringList();
    const QStringList rawGeo = item->data(RoleFormulaRawGeo).toStringList();
    QList<PreviewItem> formulaItems;
    for (int i = 0; i < rawUrls.size(); ++i) {
        PreviewItem it; it.url = rawUrls[i];
        it.geometryCode = (i < rawGeo.size()) ? rawGeo[i] : QString();
        it.sortOrder = i;
        if (!it.url.isEmpty()) formulaItems.append(it);
    }

    QStringList jobUrls, formulaUrls;
    buildComparePairs(jobItems, formulaItems, jobUrls, formulaUrls);
    item->setData(RoleJobUrls, jobUrls);
    item->setData(RoleFormulaUrls, formulaUrls);
    // 预热下载
    for (const auto &u : formulaUrls) requestPreviewImage(u);
    for (const auto &u : jobUrls) requestPreviewImage(u);
}

void AutoScPage::rebuildCompareColumns()
{
    if (!m_resultTable) return;
    for (int r = 0; r < m_resultTable->rowCount(); ++r)
        applyComparePairsForRow(r);
    m_resultTable->viewport()->update();
}

void AutoScPage::clearJobCard()
{
    m_jobCard->setVisible(false);
    m_currentTargetHex.clear();
    m_previewPixmaps.clear();
    m_jobPreviewLabel->setText(T("autosc_no_preview"));
    scheduleSidebarSync();
}

// ─── 搜索 ────────────────────────────────────────────────────
QJsonObject AutoScPage::buildSearchBody()
{
    QJsonObject body;

    // 测量作业（必填，数字类型）
    int measIdx = m_measurementCombo->currentIndex();
    if (measIdx > 0 && measIdx - 1 < m_jobs.size()) {
        body[QStringLiteral("measurementId")] = m_jobs[measIdx - 1].id.toLongLong();
    }

    // 梯度数据（数字类型）
    int calsetIdx = m_calsetCombo->currentIndex();
    if (calsetIdx > 0 && calsetIdx - 1 < m_calsets.size()) {
        body[QStringLiteral("sdkCalsetId")] = m_calsets[calsetIdx - 1].id.toLongLong();
    }

    // 搜色类型：paintCategory = METALLIC / SOLID
    if (m_metalRadio->isChecked()) {
        body[QStringLiteral("paintCategory")] = QStringLiteral("METALLIC");
        if (m_threeStepCheck->isChecked()) {
            body[QStringLiteral("onlyThreeStep")] = true;
        }
    } else {
        body[QStringLiteral("paintCategory")] = QStringLiteral("SOLID");
    }

    // 筛选（数字类型）
    int brandIdx = m_brandCombo->currentIndex();
    if (brandIdx > 0 && brandIdx - 1 < m_brands.size()) {
        body[QStringLiteral("brandId")] = m_brands[brandIdx - 1].id.toLongLong();
    }
    int seriesIdx = m_seriesCombo->currentIndex();
    if (seriesIdx > 0 && brandIdx > 0 && brandIdx - 1 < m_brands.size()) {
        QString brandId = m_brands[brandIdx - 1].id;
        auto &list = m_seriesCache[brandId];
        if (seriesIdx - 1 < list.size()) {
            body[QStringLiteral("seriesId")] = list[seriesIdx - 1].id.toLongLong();
        }
    }
    int modelIdx = m_modelCombo->currentIndex();
    if (modelIdx > 0 && seriesIdx > 0 && brandIdx > 0 && brandIdx - 1 < m_brands.size()) {
        QString brandId = m_brands[brandIdx - 1].id;
        QString seriesId = m_seriesCache[brandId][seriesIdx - 1].id;
        auto &list = m_modelCache[seriesId];
        if (modelIdx - 1 < list.size()) {
            body[QStringLiteral("modelId")] = list[modelIdx - 1].id.toLongLong();
        }
    }

    if (!m_yearEdit->text().trimmed().isEmpty()) {
        body[QStringLiteral("year")] = m_yearEdit->text().trimmed().toLongLong();
    }
    if (!m_colorNameEdit->text().trimmed().isEmpty()) {
        body[QStringLiteral("colorName")] = m_colorNameEdit->text().trimmed();
    }
    if (!m_colorCodeEdit->text().trimmed().isEmpty()) {
        body[QStringLiteral("colorCode")] = m_colorCodeEdit->text().trimmed();
    }

    return body;
}

void AutoScPage::doSearch()
{
    if (m_searching) {
        return;
    }

    // 测量作业必填
    int measIdx = m_measurementCombo->currentIndex();
    if (measIdx <= 0 || measIdx - 1 >= m_jobs.size()) {
        QMessageBox::warning(this, T("info"), T("autosc_select_meas_job_first"));
        return;
    }

    m_searching = true;
    // 递增搜索世代号：任何仍在进行的上一轮自动修色将因 gen 失配而中止
    const int gen = ++m_searchGeneration;
    m_searchBtn->setText(T("autosc_searching"));
    m_searchBtn->setEnabled(false);

    // 在搜索结果模块中央显示转圈
    if (m_resultSpinner) static_cast<SpinnerOverlay *>(m_resultSpinner)->start();

    QJsonObject body = buildSearchBody();
    ScApi::instance().searchColor(body,
        [this, gen](const QJsonObject &res) {
            m_searching = false;
            m_searchBtn->setText(T("sc_search_btn"));
            m_searchBtn->setEnabled(true);
            if (m_resultSpinner) static_cast<SpinnerOverlay *>(m_resultSpinner)->stop();

            if (gen != m_searchGeneration) return; // 已被更新的搜索取代，丢弃本次结果

            if (res[QStringLiteral("code")].toInt() == 200) {
                populateResultTable(res);
                // 搜索完成后自动修色（对标前端 doSearch → runAutoCorrectAfterSearch）
                startAutoCorrect(gen);
            } else {
                QMessageBox::warning(this, T("autosc_search_failed"),
                    res[QStringLiteral("msg")].toString());
            }
        },
        [this](const QString &msg, int code) {
            m_searching = false;
            m_searchBtn->setText(T("sc_search_btn"));
            m_searchBtn->setEnabled(true);
            if (m_resultSpinner) static_cast<SpinnerOverlay *>(m_resultSpinner)->stop();
            qWarning() << "搜色请求失败:" << msg << "code:" << code;
            QMessageBox::warning(this, T("autosc_search_failed"), msg);
        });
}

void AutoScPage::populateResultTable(const QJsonObject &res)
{
    // 兼容多种响应格式：data.items / rows / data(array) / data.rows / data.list
    QJsonArray rows;
    auto data = res[QStringLiteral("data")];
    if (data.isObject()) {
        auto obj = data.toObject();
        rows = obj[QStringLiteral("items")].toArray();
        if (rows.isEmpty()) rows = obj[QStringLiteral("rows")].toArray();
        if (rows.isEmpty()) rows = obj[QStringLiteral("list")].toArray();
    }
    if (rows.isEmpty()) rows = res[QStringLiteral("rows")].toArray();
    if (rows.isEmpty() && data.isArray()) rows = data.toArray();

    // 保存目标色五角度 RGB（色块显示用）
    if (data.isObject()) {
        auto dataObj = data.toObject();
        m_searchTargetRgbByGeometry = dataObj[QStringLiteral("targetRgbByGeometry")].toObject();
    }

    m_resultCountLabel->setText(T("autosc_total_records").arg(rows.size()));
    m_resultTable->setRowCount(rows.size());

    const int totalCols = m_resultTable->columnCount();
    // 前端 align="right" 的数值列：分数(O)/分数(AC)/粗糙度/正侧面闪烁度
    const QList<int> numCols{3, 4, 5, 6, 7};
    const QList<int> centerCols{0};
    const bool isEffect = isEffectSearch();
    const int dp = m_scoreDecimalPlaces;

    for (int i = 0; i < rows.size(); ++i) {
        auto row = rows[i].toObject();
        auto formula = row[QStringLiteral("formula")].toObject();

        // 合并 formula 字段（与前端 normalizeScSearchRow 一致）
        QString formulaId = row[QStringLiteral("formulaId")].toString();
        if (formulaId.isEmpty()) formulaId = QString::number(row[QStringLiteral("formulaId")].toDouble());
        if (formulaId == QLatin1String("0")) formulaId = QString::number(formula[QStringLiteral("id")].toDouble());
        if (formulaId.isEmpty() || formulaId == QLatin1String("0")) formulaId.clear();
        QString sampleId = row[QStringLiteral("sampleId")].toString();
        if (sampleId.isEmpty()) sampleId = QString::number(row[QStringLiteral("sampleId")].toDouble());
        if (sampleId == QLatin1String("0")) sampleId = formula[QStringLiteral("sampleId")].toString();
        QString formulaName = row[QStringLiteral("formulaName")].toString();
        if (formulaName.isEmpty()) formulaName = formula[QStringLiteral("formulaName")].toString();
        QString colorCode = row[QStringLiteral("colorCode")].toString();
        if (colorCode.isEmpty()) colorCode = formula[QStringLiteral("colorCode")].toString();
        QString colorName = row[QStringLiteral("colorName")].toString();
        if (colorName.isEmpty()) colorName = formula[QStringLiteral("colorName")].toString();
        colorName = Tn(colorName, formula[QStringLiteral("colorNameEn")].toString());
        QString brandName = Tn(formula, "brandName", "brandNameEn");
        // 前端 model 列取 modelName（兼容 modelName / model）
        QString modelName = Tn(formula, "modelName", "modelNameEn");
        if (modelName.isEmpty()) modelName = row[QStringLiteral("model")].toString();
        const QString coatType = row[QStringLiteral("coatType")].toString().isEmpty()
            ? formula[QStringLiteral("coatType")].toString()
            : row[QStringLiteral("coatType")].toString();
        const QString colorBias = row[QStringLiteral("colorBias")].toString().isEmpty()
            ? formula[QStringLiteral("colorBias")].toString()
            : row[QStringLiteral("colorBias")].toString();

        auto texture = row[QStringLiteral("texture")].toObject();

        // 分数：与前端一致，scoreO / scoreAc 直接取原始值，按阈值着色
        const bool hasScoreO = row[QStringLiteral("scoreO")].isDouble();
        const bool hasScoreAc = row[QStringLiteral("scoreAc")].isDouble();
        const double scoreO = row[QStringLiteral("scoreO")].toDouble();
        const double scoreAc = row[QStringLiteral("scoreAc")].toDouble();
        const bool corrected = row[QStringLiteral("corrected")].toBool();

        // 配方色：优先 rgbByGeometry，回退行/配方 hex
        QString formulaHex;
        auto rgbGeo = row[QStringLiteral("rgbByGeometry")].toObject();
        if (!rgbGeo.isEmpty()) {
            for (auto it = rgbGeo.begin(); it != rgbGeo.end(); ++it) {
                formulaHex = hexFromRgb(it.value().toObject());
                if (!formulaHex.isEmpty()) break;
            }
        }
        if (formulaHex.isEmpty()) formulaHex = normalizeHex(row[QStringLiteral("hexColor")].toString());
        if (formulaHex.isEmpty()) formulaHex = normalizeHex(formula[QStringLiteral("hexColor")].toString());

        // 对比列：解析本行配方预览图（前端 pickPreviewImages 回退链），存原始 url/角度码
        auto *previewItem = new QTableWidgetItem;
        QList<PreviewItem> fItems = parsePreviewItems(row[QStringLiteral("previewImages")]);
        if (fItems.isEmpty()) fItems = parsePreviewItems(formula[QStringLiteral("previewImages")]);
        if (fItems.isEmpty()) fItems = parsePreviewItems(formula[QStringLiteral("previewImageItems")]);
        {
            QStringList rawUrls, rawGeo;
            for (const auto &it : fItems) { rawUrls << it.url; rawGeo << it.geometryCode; }
            previewItem->setData(RoleFormulaRawUrls, rawUrls);
            previewItem->setData(RoleFormulaRawGeo, rawGeo);
        }
        previewItem->setTextAlignment(Qt::AlignCenter);
        m_resultTable->setItem(i, 0, previewItem);
        // 根据当前作业图 + 本行配方图计算对比带（含预热下载）
        applyComparePairsForRow(i);

        auto setText = [this, i, &numCols, &centerCols](int col, const QString &text) {
            auto *item = new QTableWidgetItem(text);
            Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter;
            if (numCols.contains(col)) align = Qt::AlignRight | Qt::AlignVCenter;
            else if (centerCols.contains(col)) align = Qt::AlignCenter;
            item->setTextAlignment(align);
            m_resultTable->setItem(i, col, item);
            return item;
        };

        auto fmtNum = [](const QJsonValue &val, int decimals) -> QString {
            if (!val.isDouble()) return QStringLiteral("—");
            return QString::number(val.toDouble(), 'f', decimals);
        };

        setText(1, sampleId.isEmpty() ? QStringLiteral("—") : sampleId);
        setText(2, formulaName);

        auto *itemO = setText(3, hasScoreO ? fmtFixed(scoreO, dp) : QStringLiteral("—"));
        if (hasScoreO) {
            const QString tone = scoreTone(scoreO, isEffect ? m_effectInitial : m_solidInitial);
            if (!tone.isEmpty()) itemO->setForeground(scoreToneColor(tone));
        }

        auto *itemAc = setText(4, hasScoreAc ? fmtFixed(scoreAc, dp) : QStringLiteral("—"));
        if (hasScoreAc) {
            const QString tone = scoreTone(scoreAc, isEffect ? m_effectCorrected : m_solidCorrected);
            if (!tone.isEmpty()) {
                itemAc->setForeground(scoreToneColor(tone));
            } else if (!corrected) {
                itemAc->setForeground(QColor(QStringLiteral("#c0c4cc")));
            }
        } else {
            itemAc->setForeground(QColor(QStringLiteral("#c0c4cc")));
        }

        setText(5, fmtNum(texture[QStringLiteral("coarseness")], 3));
        setText(6, fmtNum(texture[QStringLiteral("sparkleGradeR15As15")], 3));
        setText(7, fmtNum(texture[QStringLiteral("sparkleGradeR15As80")], 3));
        setText(8, coatTypeDisplay(coatType));
        setText(9, brandName);
        setText(10, colorName);
        setText(11, colorCode);
        setText(12, modelName);
        setText(13, colorBiasDisplay(colorBias));

        // 保存完整行数据（含 formulaId、sampleId、配方色）供配方面板使用
        QJsonObject rowData = row;
        rowData[QStringLiteral("formulaId")] = formulaId;
        rowData[QStringLiteral("sampleId")] = sampleId;
        rowData[QStringLiteral("formulaName")] = formulaName;
        rowData[QStringLiteral("brandName")] = brandName;
        rowData[QStringLiteral("colorName")] = colorName;
        rowData[QStringLiteral("colorCode")] = colorCode;
        rowData[QStringLiteral("modelName")] = modelName;
        rowData[QStringLiteral("coatType")] = coatType;
        rowData[QStringLiteral("colorBias")] = colorBias;
        if (!formulaHex.isEmpty()) rowData[QStringLiteral("formulaHex")] = formulaHex;
        const QByteArray payload = QJsonDocument(rowData).toJson();
        for (int col = 0; col < totalCols; ++col) {
            auto *item = m_resultTable->item(i, col);
            if (item) item->setData(Qt::UserRole, payload);
        }
    }

    if (rows.isEmpty()) {
        m_resultCountLabel->setText(T("autosc_no_match_formula"));
    }
}

void AutoScPage::resetFilters()
{
    m_calsetCombo->setCurrentIndex(0);
    m_deviceCombo->setCurrentIndex(0);
    m_measurementCombo->clear();
    m_measurementCombo->setEnabled(false);
    m_refreshJobsBtn->setEnabled(false);
    clearJobCard();

    m_solidRadio->setChecked(true);
    m_metalRadio->setChecked(false);
    m_threeStepCheck->setChecked(false);
    m_threeStepCheck->setVisible(false);

    m_brandCombo->setCurrentIndex(0);
    m_seriesCombo->clear();
    m_seriesCombo->setEnabled(false);
    m_modelCombo->clear();
    m_modelCombo->setEnabled(false);
    m_yearEdit->clear();
    m_colorNameEdit->clear();
    m_colorCodeEdit->clear();

    m_resultTable->setRowCount(0);
    m_resultCountLabel->clear();
    clearRecipePanel();
}

// ─── 结果行选中 ──────────────────────────────────────────────
void AutoScPage::onResultRowSelected(int row)
{
    if (row < 0) return;
    auto *item = m_resultTable->item(row, 0);
    if (!item) {
        qWarning() << "onResultRowSelected: item 为空，row =" << row;
        return;
    }
    QByteArray json = item->data(Qt::UserRole).toByteArray();
    if (json.isEmpty()) {
        qWarning() << "onResultRowSelected: JSON 数据为空，row =" << row;
        return;
    }
    QJsonObject rowObj = QJsonDocument::fromJson(json).object();
    // 调试：检查 AC 光谱数据字段
    if (rowObj[QStringLiteral("spectralAc")].isObject()) {
    } else if (rowObj[QStringLiteral("spectralAc")].isArray()) {
    }
    loadRecipePanel(rowObj);

    // 刷新可视化卡片（行选择变化后，色块等卡片需要更新）
    for (auto &card : m_vizCards) {
        updateVizCardContent(card);
    }

    // 加载配方可视化数据（配方光谱+修色光谱）
    QString measurementId = m_currentMeasurementId;
    // formulaId 可能是字符串或数字，需要兼容处理
    QString formulaIdStr = rowObj[QStringLiteral("formulaId")].toString();
    if (formulaIdStr.isEmpty()) formulaIdStr = QString::number(rowObj[QStringLiteral("formulaId")].toDouble());
    int formulaId = formulaIdStr.toInt();
    if (formulaId <= 0) {
        formulaIdStr = rowObj[QStringLiteral("id")].toString();
        if (formulaIdStr.isEmpty()) formulaIdStr = QString::number(rowObj[QStringLiteral("id")].toDouble());
        formulaId = formulaIdStr.toInt();
    }
    if (!measurementId.isEmpty() && formulaId > 0) {
        loadFormulaVisualization(measurementId, formulaId);
    } else {
        qWarning() << "无法加载配方可视数据: measurementId=" << measurementId << "formulaId=" << formulaId;
    }

    // 选中搜索结果后，自动展开配方面板（如果处于折叠状态）
    if (m_recipePanel) {
        QMetaObject::invokeMethod(m_recipePanel, "toggleCollapse", Qt::DirectConnection,
            Q_ARG(bool, true)); // 尝试展开（内部会检查是否已折叠）
    }
}

/**
 * 分数(AC-Edited)：对标前端 meta 行 `v-if="hasAcEdited && scoreAcEditedDisplay"`，
 * 取值优先级 scoreAcEdited → predictDetail.scoreAc。
 * 预测成功后必须立刻刷新，否则要重新选中配方行才显示。
 */
void AutoScPage::refreshAcEditedScoreLabel()
{
    const auto pd = m_currentRowObj[QStringLiteral("predictDetail")].toObject();
    double score = 0.0;
    bool has = false;
    if (pd[QStringLiteral("scoreAcEdited")].isDouble()) {
        score = pd[QStringLiteral("scoreAcEdited")].toDouble();
        has = true;
    } else if (pd[QStringLiteral("scoreAc")].isDouble()) {
        score = pd[QStringLiteral("scoreAc")].toDouble();
        has = true;
    }

    if (m_recipeScoreAcEditedGroup) m_recipeScoreAcEditedGroup->setVisible(has);
    if (!m_recipeScoreAcEditedLabel) return;
    if (!has) {
        m_recipeScoreAcEditedLabel->setText(QStringLiteral("—"));
        m_recipeScoreAcEditedLabel->setStyleSheet(QStringLiteral(
            "color: #c0c4cc; font-size: 13px; font-weight: 700;"));
        return;
    }
    const bool isEffect = isEffectSearch();
    m_recipeScoreAcEditedLabel->setText(fmtFixed(score, m_scoreDecimalPlaces));
    m_recipeScoreAcEditedLabel->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 700; color: %1;")
        .arg(scoreToneColor(scoreTone(score, isEffect ? m_effectCorrected : m_solidCorrected)).name()));
}

void AutoScPage::loadRecipePanel(const QJsonObject &row)
{
    m_currentRowObj = row;
    syncCorrectButtonState();   // 已修色的行禁用修色按钮（智能修色不受限）

    // 保存当前行的配方五角度 RGB（色块显示用）
    m_rowRgbByGeometry = row[QStringLiteral("rgbByGeometry")].toObject();

    // 重置修色数据（会在后面检测到 components 时重新设置）
    m_correctedBaseRows = QJsonArray();
    m_correctedTopRows = QJsonArray();
    m_hasCorrection = false;
    m_correctedVendorData = QJsonObject();
    m_correctedVendorFetchKey.clear();

    // 切换配方先丢弃编辑态与 AC-Edited；本行若已有预测结果则在配方行就绪后恢复
    resetEditState();

    // ── 元信息栏（前端 sc-recipe-panel__meta）──
    const QString sampleId = row[QStringLiteral("sampleId")].toString();
    m_recipeMetaLabel->setText(sampleId.isEmpty() ? QStringLiteral("—") : sampleId);
    m_recipeCoatTypeLabel->setText(coatTypeDisplay(row[QStringLiteral("coatType")].toString()));

    const bool isEffect = isEffectSearch();
    auto applyScore = [this](QLabel *label, bool has, double value, const ScoreThreshold &th) {
        if (!has) {
            label->setText(QStringLiteral("—"));
            label->setStyleSheet(QStringLiteral(
                "color: #c0c4cc; font-size: 13px; font-weight: 700;"));
            return;
        }
        label->setText(fmtFixed(value, m_scoreDecimalPlaces));
        label->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 700; color: %1;")
            .arg(scoreToneColor(scoreTone(value, th)).name()));
    };
    const bool hasO = row[QStringLiteral("scoreO")].isDouble();
    applyScore(m_recipeScoreOLabel, hasO, row[QStringLiteral("scoreO")].toDouble(),
               isEffect ? m_effectInitial : m_solidInitial);
    const bool hasAc = row[QStringLiteral("scoreAc")].isDouble();
    applyScore(m_recipeScoreAcLabel, hasAc, row[QStringLiteral("scoreAc")].toDouble(),
               isEffect ? m_effectCorrected : m_solidCorrected);

    // 分数(AC-Edited)：仅在该行有预测结果时出现（对标前端 colScoreAcEdited）
    refreshAcEditedScoreLabel();

    // 配方信息：名称 | 品牌 颜色名 色号 车型
    QStringList parts;
    const QString name = row[QStringLiteral("formulaName")].toString();
    if (!name.isEmpty()) parts << name;
    QStringList tail;
    const QStringList tailKeys = {QStringLiteral("brandName"), QStringLiteral("colorName"),
                                  QStringLiteral("colorCode"), QStringLiteral("modelName")};
    for (const auto &key : tailKeys) {
        const QString v = row[key].toString();
        if (!v.isEmpty()) tail << v;
    }
    if (!tail.isEmpty()) parts << tail.join(QStringLiteral(" "));
    m_recipeInfoLabel->setText(parts.isEmpty() ? QStringLiteral("—") : parts.join(QStringLiteral(" | ")));

    // ── 修色结果：如果有 components 字段，保存修色后的色母数据 ──
    const auto components = row[QStringLiteral("components")].toArray();
    if (!components.isEmpty()) {
        // 将 components 转换为 details 格式
        QJsonArray correctedDetailRows;
        for (const auto &v : components) {
            const auto comp = v.toObject();
            QJsonObject detail;
            detail[QStringLiteral("code")] = comp[QStringLiteral("colorantName")].toString();
            detail[QStringLiteral("colorantName")] = comp[QStringLiteral("colorantName")].toString();
            detail[QStringLiteral("pigmentName")] = comp[QStringLiteral("pigmentName")].toString();
            detail[QStringLiteral("pigmentNameEn")] = comp[QStringLiteral("pigmentNameEn")].toString();
            detail[QStringLiteral("color_type")] = comp[QStringLiteral("colorType")].toString();
            detail[QStringLiteral("weight")] = comp[QStringLiteral("concentration")].toDouble();
            detail[QStringLiteral("layer")] = comp[QStringLiteral("layer")].toInt();
            detail[QStringLiteral("rgb")] = comp[QStringLiteral("rgb")];
            correctedDetailRows.append(detail);
        }
        
        // 按 layer 分组
        QJsonArray correctedBase, correctedTop;
        for (const auto &v : correctedDetailRows) {
            const auto r = v.toObject();
            const int layer = r[QStringLiteral("layer")].toInt();
            if (layer == 2) correctedTop.append(v);
            else correctedBase.append(v);
        }
        m_correctedBaseRows = correctedBase;
        m_correctedTopRows = correctedTop;
        m_hasCorrection = true;
        loadCorrectedVendorData(components);
        // 不 return，继续拉取原始配方详情
    }

    // ── 拉取配方详情（与前端 loadFormulaDetail 一致：row.id || row.formulaId）──
    bool okId = false;
    // 优先使用 toString().toLongLong() 更可靠
    QString formulaIdStr = row[QStringLiteral("formulaId")].toString();
    if (formulaIdStr.isEmpty()) formulaIdStr = QString::number(row[QStringLiteral("formulaId")].toDouble());
    qint64 formulaId = formulaIdStr.toLongLong(&okId);
    if (!okId || formulaId <= 0) {
        formulaIdStr = row[QStringLiteral("id")].toString();
        if (formulaIdStr.isEmpty()) formulaIdStr = QString::number(row[QStringLiteral("id")].toDouble());
        formulaId = formulaIdStr.toLongLong(&okId);
    }
    if (formulaId <= 0) {
        // 无配方 ID 时直接使用搜色行自带的 details
        qWarning() << "loadRecipePanel: 无配方 ID，使用搜色行自带 details";
        m_currentBaseRows = row[QStringLiteral("details")].toArray();
        m_currentTopRows = QJsonArray();
        restoreAcEditedFromRow();
        rebuildTonerTable(m_currentBaseRows, m_currentTopRows);
        return;
    }

    // 保存搜色行自带的 details（含 rgb 颜色），用于构建色块颜色映射表
    m_searchRowDetails = row[QStringLiteral("details")].toArray();

    FormulaApi::instance().getFormulaDetail(int(formulaId), m_pigmentCodeSystem,
        [this, formulaId](const QJsonObject &res) {
            auto data = res[QStringLiteral("data")].toObject();
            if (data.isEmpty()) data = res;
            const auto formula = data[QStringLiteral("formula")].toObject();

            // detailRows 回退链：data.detailRows → data.details → formula.details
            QJsonArray detailRows = data[QStringLiteral("detailRows")].toArray();
            if (detailRows.isEmpty()) detailRows = data[QStringLiteral("details")].toArray();
            if (detailRows.isEmpty()) detailRows = formula[QStringLiteral("details")].toArray();

            const QString coat = formula[QStringLiteral("coatType")].toString();
            if (!coat.isEmpty()) m_recipeCoatTypeLabel->setText(coatTypeDisplay(coat));

            // 与前端 getBaseLayerRows / getTopLayerRows 一致
            const bool threeStep = formula[QStringLiteral("threeStep")].toBool();
            m_isThreeStep = threeStep;

            // 罐容量和表格由 rebuildTonerTable 动态管理

            QJsonArray base, top;
            for (const auto &v : detailRows) {
                const auto r = v.toObject();
                const bool hasLayer = r[QStringLiteral("layer")].isDouble();
                const int layer = hasLayer ? r[QStringLiteral("layer")].toInt() : 1;
                if (layer == 2) { if (threeStep) top.append(v); }
                else if (layer == 1 || !hasLayer) base.append(v);
            }

            m_currentBaseRows = base;
            m_currentTopRows = top;
            restoreAcEditedFromRow();
            rebuildTonerTable(base, top, m_correctedBaseRows, m_correctedTopRows);

            // 更新下载按钮可见性
            if (m_downloadBaseBtn) m_downloadBaseBtn->setVisible(threeStep && !top.isEmpty());
            if (m_downloadSurfaceBtn) m_downloadSurfaceBtn->setVisible(threeStep && !top.isEmpty());
            if (m_downloadSingleBtn) m_downloadSingleBtn->setVisible(!threeStep);
            syncEditActionButtons();
        },
        [this, row, formulaId](const QString &msg, int) {
            qWarning() << "加载配方详情失败: formulaId =" << formulaId << "msg =" << msg;
            // 回退到搜色行自带的色母明细
            m_currentBaseRows = row[QStringLiteral("details")].toArray();
            m_currentTopRows = QJsonArray();
            restoreAcEditedFromRow();
            rebuildTonerTable(m_currentBaseRows, m_currentTopRows,
                              m_correctedBaseRows, m_correctedTopRows);
            // 隐藏下载按钮
            if (m_downloadBaseBtn) m_downloadBaseBtn->setVisible(false);
            if (m_downloadSurfaceBtn) m_downloadSurfaceBtn->setVisible(false);
            if (m_downloadSingleBtn) m_downloadSingleBtn->setVisible(false);
            syncEditActionButtons();
        });
}

namespace {
/** 罐容量单位换算（对标前端 utils/weightUnit.js）：g/ml 为基准，kg/l 为千分之一 */
double weightToDisplayUnit(double grams, const QString &unit)
{
    return (unit == QLatin1String("kg") || unit == QLatin1String("l")) ? grams / 1000.0 : grams;
}

double displayUnitToWeight(double value, const QString &unit)
{
    return (unit == QLatin1String("kg") || unit == QLatin1String("l")) ? value * 1000.0 : value;
}

/** 兼容多种后端字段名取字符串 */
QString rowKeyField(const QJsonObject &obj, const QStringList &keys)
{
    for (const auto &key : keys) {
        const QString v = obj[key].toVariant().toString().trimmed();
        if (!v.isEmpty()) return v;
    }
    return QString();
}

QString rowRawCode(const QJsonObject &obj)
{
    return rowKeyField(obj, {QStringLiteral("code"), QStringLiteral("pigmentCode"),
                             QStringLiteral("pigmentNo"), QStringLiteral("no")});
}

QString rowPigmentName(const QJsonObject &obj)
{
    return rowKeyField(obj, {QStringLiteral("pigmentName"), QStringLiteral("name")});
}

/** 配方行/修色组分的色母编号（对标前端 resolvePigmentCode 的取值顺序） */
QString rowAcCode(const QJsonObject &obj)
{
    return rowKeyField(obj, {QStringLiteral("pigmentCode"), QStringLiteral("pasteCode"),
                             QStringLiteral("code"), QStringLiteral("colorantName")});
}

/**
 * 色母编号体系解析（对标 utils/pigmentVendor.js）
 * 面板按品牌显示厂商编号，而 predict / AC 匹配要用内部编号；
 * 映射数据来自修色后按 AC 组分的内部编号批量查询的 vendor-codes 结果。
 */
struct VendorCodes {
    QJsonObject data;    // 内部 pigmentCode -> { pigmentCode, mengyingCode, ... }
    QString system;      // 空 = 内部编号体系

    QString vendorField() const
    {
        return system.isEmpty() ? QStringLiteral("internalCode")
                                : system + QStringLiteral("Code");
    }

    /** 按 key 取映射行：原样 → 大写 → 忽略大小写扫描（对标 pickVendorRow） */
    QJsonObject pickRow(const QString &code) const
    {
        const QString key = code.trimmed();
        if (key.isEmpty()) return QJsonObject();
        if (data.contains(key)) return data.value(key).toObject();
        const QString upper = key.toUpper();
        for (auto it = data.begin(); it != data.end(); ++it) {
            if (it.key().trimmed().toUpper() == upper) return it.value().toObject();
        }
        return QJsonObject();
    }

    /** 内部编号 → 当前品牌展示编号 */
    QString displayOfInternal(const QString &internalCode) const
    {
        const QString internal = internalCode.trimmed();
        if (internal.isEmpty() || system.isEmpty()) return internal;
        const QString vendor = pickRow(internal).value(vendorField()).toVariant().toString().trimmed();
        return vendor.isEmpty() ? internal : vendor;
    }

    /** 展示编号 → 内部编号；没有映射数据时原样返回（与前端兜底一致） */
    QString internalOfDisplay(const QString &displayCode) const
    {
        const QString display = displayCode.trimmed();
        if (display.isEmpty() || system.isEmpty() || data.isEmpty()) return display;
        const QString direct = pickRow(display).value(QStringLiteral("pigmentCode"))
                                   .toVariant().toString().trimmed();
        if (!direct.isEmpty()) return direct;
        const QString field = vendorField();
        for (auto it = data.begin(); it != data.end(); ++it) {
            const auto row = it.value().toObject();
            const QString vendor = row.value(field).toVariant().toString().trimmed();
            if (!vendor.isEmpty() && vendor.compare(display, Qt::CaseInsensitive) == 0) {
                const QString internal = row.value(QStringLiteral("pigmentCode"))
                                             .toVariant().toString().trimmed();
                return internal.isEmpty() ? display : internal;
            }
        }
        return display;
    }
};

/** 配方行的内部编号 */
QString rowInternalCode(const VendorCodes &vc, const QJsonObject &row)
{
    return vc.internalOfDisplay(rowAcCode(row));
}

/** 修色组分的内部编号（AC 返回的 colorantName 即内部编号） */
QString acInternalCode(const QJsonObject &comp)
{
    return rowKeyField(comp, {QStringLiteral("colorantName"), QStringLiteral("pigmentCode"),
                              QStringLiteral("code")}).trimmed();
}

/** AC-Edited 占比表的键：层 + 显示编号 */
QString editedPctKey(const QString &layerKey, const QString &code)
{
    return layerKey + QLatin1Char('|') + code.trimmed().toUpper();
}

/**
 * 修色组分按内部编号建索引（对标 buildLayerAcMapByCode / ByIndex）：
 * 优先按编号匹配；展示厂商编号时编号对不上就分行展示，不按行序回退。
 */
QMap<QString, QJsonObject> buildLayerAcMap(const QJsonArray &rows, const QJsonArray &components,
                                           const VendorCodes &vc)
{
    QMap<QString, QJsonObject> byCode;
    QSet<QString> layerInternals;
    for (const auto &v : rows) {
        const QString internal = rowInternalCode(vc, v.toObject()).toUpper();
        if (!internal.isEmpty()) layerInternals.insert(internal);
    }
    QSet<QString> used;
    for (const auto &v : components) {
        const auto comp = v.toObject();
        const QString internal = acInternalCode(comp).toUpper();
        if (internal.isEmpty()) continue;
        // 本层没有的编号交给 mergeLayerRowsWithAc 追加成行
        if (!layerInternals.isEmpty() && !layerInternals.contains(internal)) continue;
        byCode.insert(internal, comp);
        used.insert(internal);
    }
    if (byCode.isEmpty() && vc.system.isEmpty() && !components.isEmpty()) {
        for (int i = 0; i < rows.size(); ++i) {
            const QString internal = rowInternalCode(vc, rows[i].toObject()).toUpper();
            if (internal.isEmpty() || i >= components.size()) continue;
            byCode.insert(internal, components[i].toObject());
            used.insert(internal);
        }
    }

    /**
     * 兜底：取不到 vendor-codes 映射（例如没有 colorcr:pigment:query 权限）时，
     * 面板显示的品牌编号与 AC 的内部编号对不上，再按中文名补齐，避免整列空白。
     */
    for (const auto &v : rows) {
        const auto t = v.toObject();
        const QString internal = rowInternalCode(vc, t).toUpper();
        if (internal.isEmpty() || byCode.contains(internal)) continue;
        const QString name = rowPigmentName(t);
        if (name.isEmpty()) continue;
        for (const auto &cv : components) {
            const auto comp = cv.toObject();
            const QString compInternal = acInternalCode(comp).toUpper();
            if (compInternal.isEmpty() || used.contains(compInternal)) continue;
            if (rowPigmentName(comp) != name) continue;
            byCode.insert(internal, comp);
            used.insert(compInternal);
            break;
        }
    }
    return byCode;
}

/** 修色/预测组分按工序层筛选（对标前端 filterAcComponentsForLayer） */
QJsonArray filterComponentsForLayer(const QString &layerKey, const QJsonArray &components,
                                     bool isThreeStep)
{
    if (components.isEmpty() || !isThreeStep || layerKey == QLatin1String("single")) return components;
    const int wantLayer = layerKey == QLatin1String("surface") ? 2 : 1;
    bool hasLayer = false;
    for (const auto &v : components) {
        if (v.toObject()[QStringLiteral("layer")].isDouble()) { hasLayer = true; break; }
    }
    // 无层信息时整表按底层处理，避免面层重复
    if (!hasLayer) return layerKey == QLatin1String("base") ? components : QJsonArray();
    QJsonArray out;
    for (const auto &v : components) {
        if (v.toObject()[QStringLiteral("layer")].toInt() == wantLayer) out.append(v);
    }
    return out;
}

/**
 * 初始配方色母 + 修色多出的色母（SMART 常见），多出的行重量为 0（对标 mergeLayerRowsWithAc）：
 * 编号对不上时再比中文名，避免取不到映射数据时把整层色母重复追加一遍。
 */
QJsonArray mergeLayerRowsWithAc(const QJsonArray &rows, const QJsonArray &components,
                                const VendorCodes &vc)
{
    if (components.isEmpty()) return rows;
    QJsonArray merged = rows;
    QSet<QString> existing;
    QSet<QString> existingNames;
    for (const auto &v : rows) {
        const auto t = v.toObject();
        const QString internal = rowInternalCode(vc, t).toUpper();
        if (!internal.isEmpty()) existing.insert(internal);
        const QString name = rowPigmentName(t);
        if (!name.isEmpty()) existingNames.insert(name);
    }
    for (const auto &v : components) {
        const auto comp = v.toObject();
        const QString internal = acInternalCode(comp);
        if (internal.isEmpty() || existing.contains(internal.toUpper())) continue;
        const QString name = rowPigmentName(comp);
        if (!name.isEmpty() && existingNames.contains(name)) continue;
        existing.insert(internal.toUpper());
        if (!name.isEmpty()) existingNames.insert(name);
        const QString display = vc.displayOfInternal(internal);
        QJsonObject row;
        row[QStringLiteral("pigmentCode")] = display;
        row[QStringLiteral("pasteCode")] = display;
        row[QStringLiteral("code")] = display;
        row[QStringLiteral("colorantName")] = internal;
        row[QStringLiteral("pigmentName")] = name;
        row[QStringLiteral("pigmentNameEn")] = rowKeyField(comp, {QStringLiteral("pigmentNameEn"),
                                                                  QStringLiteral("nameEn")});
        row[QStringLiteral("color_type")] = rowKeyField(comp, {QStringLiteral("colorType"),
                                                               QStringLiteral("color_type")});
        row[QStringLiteral("rgb")] = comp[QStringLiteral("rgb")];
        row[QStringLiteral("weight")] = 0.0;
        row[QStringLiteral("fromAcOnly")] = true;
        merged.append(row);
    }
    return merged;
}

/** 修色组分的层内占比基准：后端返回的 concentration 即 AC 列显示值 */
double acComponentPct(const QJsonObject &comp)
{
    if (comp[QStringLiteral("concentration")].isDouble())
        return comp[QStringLiteral("concentration")].toDouble();
    return parseWeight(comp[QStringLiteral("weight")]);
}

/** 层内三列基准（均与 rows 对齐，<0 表示该色母无此列的值） */
struct LayerPcts {
    QJsonArray rows;          // 初始行 + 修色多出的色母行，表格与草稿都以此为准
    QList<double> initialPct; // 初始层内占比 %
    QList<double> acPct;      // AC 修色后的层内占比（后端 concentration）
    QList<double> editedPct;  // AC-Edited 层内占比 %
    double acPctSum = 0.0;
    double editedPctSum = 0.0;
};

LayerPcts computeLayerPcts(const QJsonArray &rows, const QJsonArray &correctedRows,
                           const QString &layerKey, const QMap<QString, double> &editedPct,
                           const VendorCodes &vc)
{
    LayerPcts out;
    out.rows = mergeLayerRowsWithAc(rows, correctedRows, vc);
    const QMap<QString, QJsonObject> acMap = buildLayerAcMap(out.rows, correctedRows, vc);

    double totalWeight = 0.0;
    for (const auto &v : out.rows) totalWeight += parseWeight(v.toObject()[QStringLiteral("weight")]);

    for (const auto &v : out.rows) {
        const auto t = v.toObject();
        // SMART 新增色母不在初始配方中，初始占比显示为空
        const bool acOnly = t[QStringLiteral("fromAcOnly")].toBool();
        out.initialPct.append((!acOnly && totalWeight > 0)
            ? (parseWeight(t[QStringLiteral("weight")]) / totalWeight) * 100.0 : -1.0);

        const QString internal = rowInternalCode(vc, t).toUpper();
        const auto acIt = acMap.constFind(internal);
        const double ac = (acIt != acMap.constEnd()) ? acComponentPct(acIt.value()) : -1.0;
        out.acPct.append(ac);
        if (ac >= 0) out.acPctSum += ac;

        const double ep = editedPct.value(editedPctKey(layerKey, rowAcCode(t)), -1.0);
        out.editedPct.append(ep);
        if (ep >= 0) out.editedPctSum += ep;
    }
    return out;
}

/** 当前选中列下的每行克重 = 罐容量 × 该列层内占比（对标前端 calcQtyGrams） */
QList<double> gramsFromLayerPcts(const LayerPcts &pcts, double capacity, const QString &col)
{
    QList<double> out;
    for (int i = 0; i < pcts.initialPct.size(); ++i) {
        double grams = -1.0;
        if (col == QLatin1String("ac")) {
            const double pct = pcts.acPct.value(i, -1.0);
            if (pct >= 0 && pcts.acPctSum > 0) grams = capacity * pct / pcts.acPctSum;
        } else if (col == QLatin1String("acEdited")) {
            const double pct = pcts.editedPct.value(i, -1.0);
            if (pct >= 0)
                grams = pcts.editedPctSum > 0 ? capacity * pct / pcts.editedPctSum
                                              : capacity * pct / 100.0;
        } else {
            const double pct = pcts.initialPct.value(i, -1.0);
            if (pct >= 0) grams = capacity * pct / 100.0;
        }
        out.append(grams);
    }
    return out;
}

/** 选中配方列的底色（对标前端 .sc-formula-col--{initial,ac,acEdited} 半透明色叠白底后的实色） */
QColor formulaColTint(const QString &col)
{
    if (col == QLatin1String("ac")) return QColor(224, 247, 233);       // rgba(34,197,94,.14)
    if (col == QLatin1String("acEdited")) return QColor(223, 248, 239); // rgba(52,211,153,.16)
    if (col == QLatin1String("initial")) return QColor(252, 244, 220);  // rgba(234,179,8,.14)
    return QColor();
}
}

/**
 * 修色完成后拉取内部编号 → 品牌编号映射（对标前端 loadCorrectedVendorData）：
 * 面板显示品牌编号，编辑确认提交预测时要用回内部编号。
 * 拉不到（无权限/失败）时保持按内部编号处理，与前端兜底一致。
 */
void AutoScPage::loadCorrectedVendorData(const QJsonArray &components)
{
    QStringList codes;
    for (const auto &v : components) {
        const QString code = acInternalCode(v.toObject());
        if (!code.isEmpty() && !codes.contains(code)) codes.append(code);
    }
    if (codes.isEmpty()) {
        m_correctedVendorData = QJsonObject();
        m_correctedVendorFetchKey.clear();
        return;
    }

    const QString fetchKey = codes.join(QLatin1Char('|'));
    if (fetchKey == m_correctedVendorFetchKey && !m_correctedVendorData.isEmpty()) return;
    m_correctedVendorFetchKey = fetchKey;

    FormulaApi::instance().getPigmentVendorCodes(codes,
        [this, fetchKey](const QJsonObject &res) {
            if (fetchKey != m_correctedVendorFetchKey) return;   // 已切换到别的配方行
            if (res[QStringLiteral("code")].toInt() != 200) return;
            const auto data = res[QStringLiteral("data")].toObject();
            if (data.isEmpty()) return;
            m_correctedVendorData = data;
            if (!m_currentBaseRows.isEmpty() || !m_currentTopRows.isEmpty()) {
                rebuildTonerTable(m_currentBaseRows, m_currentTopRows,
                                  m_correctedBaseRows, m_correctedTopRows);
            }
        },
        [this, fetchKey](const QString &, int) {
            if (fetchKey == m_correctedVendorFetchKey) m_correctedVendorFetchKey.clear();
        });
}

// 重建色母表：占比 = 层内重量 / 层内总重 × 100，数量 = 罐容量 × 占比 / 100
void AutoScPage::rebuildTonerTable(const QJsonArray &baseRows, const QJsonArray &topRows,
                                   const QJsonArray &correctedBaseRows,
                                   const QJsonArray &correctedTopRows)
{
    if (!m_tonerContainer) return;

    // 整表重建期间面板高度会重算，记下当前浏览位置，重建后还原（选行/编辑/切列都不跳）
    const int keptScroll = m_mainScroll ? m_mainScroll->verticalScrollBar()->value() : 0;

    // 清空容器
    auto *layout = m_tonerContainer->layout();
    while (layout && layout->count() > 0) {
        auto *item = layout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
    }
    m_tonerTable = nullptr;
    m_baseTable = nullptr;
    m_surfaceTable = nullptr;
    m_canCapacityEdit = nullptr;
    m_baseCapacityEdit = nullptr;
    m_surfaceCapacityEdit = nullptr;
    m_tonerTotalLabel = nullptr;
    m_baseTotalLabel = nullptr;
    m_surfaceTotalLabel = nullptr;
    // 表格即将整体重建，编辑态的单元格引用随之失效
    m_editCells.clear();
    m_editSections.clear();
    m_formulaViews.clear();

    const bool hasCorrected = !correctedBaseRows.isEmpty() || !correctedTopRows.isEmpty();
    const bool showAcEdited = m_hasAcEdited;
    // 换行/换编号体系后可能没有对应列，选中列随之回落，避免表头高亮与数量列口径不一致
    if (m_selectedFormulaCol == QLatin1String("ac") && !hasCorrected)
        m_selectedFormulaCol = QStringLiteral("initial");
    if (m_selectedFormulaCol == QLatin1String("acEdited") && !showAcEdited)
        m_selectedFormulaCol = QStringLiteral("initial");

    // 面板显示品牌编号，AC 匹配与预测用内部编号
    const VendorCodes vc{m_correctedVendorData, m_pigmentCodeSystem};
    /** 该层数量列的单位（跨重建保持，默认 g） */
    auto layerUnit = [this](const QString &layerKey) {
        const QString unit = m_layerUnits.value(layerKey, QStringLiteral("g"));
        return unit.isEmpty() ? QStringLiteral("g") : unit;
    };
    /** 克 → 该层单位下的显示文本 */
    auto displayWeight = [](double grams, const QString &unit) {
        return formatWeightValue(weightToDisplayUnit(grams, unit));
    };

    struct Section {
        QString title;
        bool isBase;            // 底层标记（标题已多语言化，逻辑判断不再依赖文本）
        QString layerKey;       // single / base / surface（草稿与 AC-Edited 占比的分组键）
        QJsonArray rows;
        QJsonArray correctedRows;
    };
    QList<Section> sections;
    if (!baseRows.isEmpty()) {
        sections.append({T("autosc_base_layer"), true, editLayerKey(true), baseRows, correctedBaseRows});
    }
    if (!topRows.isEmpty()) {
        sections.append({T("autosc_surface_layer"), false, editLayerKey(false), topRows, correctedTopRows});
    }

    const bool multiLayer = sections.size() > 1;

    // 列布局对标前端：成分（颜色/编号/名称/类型）| 配方(%)（初始[/AC][/AC-Edited]）| 配方详细信息（数量/累计）
    const int pctCols = 1 + (hasCorrected ? 1 : 0) + (showAcEdited ? 1 : 0);
    int colCursor = 4;
    const int colInitial = colCursor++;
    const int colAc = hasCorrected ? colCursor++ : -1;
    const int colAcEdited = showAcEdited ? colCursor++ : -1;
    const int colQty = colCursor++;
    const int colCum = colCursor++;
    const int cols = colCursor;
    m_editQtyCol = colQty;
    m_editCumCol = colCum;

    auto firstString = [](const QJsonObject &obj, const QStringList &keys) -> QString {
        for (const auto &key : keys) {
            const QString v = obj[key].toVariant().toString().trimmed();
            if (!v.isEmpty()) return v;
        }
        return QString();
    };

    // 构建色母编号到颜色的映射表
    QMap<QString, QString> pigmentRgbMap;
    auto pushRgbToMap = [&](const QJsonObject &row, const QStringList &codeKeys) {
        const QString codeRaw = firstString(row, codeKeys).trimmed().toUpper();
        if (codeRaw.isEmpty()) return;
        const QString hex = hexFromRgb(row[QStringLiteral("rgb")].toObject());
        if (!hex.isEmpty()) pigmentRgbMap[codeRaw] = hex;
    };
    for (const auto &v : baseRows) pushRgbToMap(v.toObject(), {QStringLiteral("pigmentCode"), QStringLiteral("code")});
    for (const auto &v : topRows) pushRgbToMap(v.toObject(), {QStringLiteral("pigmentCode"), QStringLiteral("code")});
    for (const auto &v : m_searchRowDetails) pushRgbToMap(v.toObject(), {QStringLiteral("pigmentCode"), QStringLiteral("code")});
    for (const auto &v : correctedBaseRows) pushRgbToMap(v.toObject(), {QStringLiteral("pigmentCode"), QStringLiteral("code")});
    for (const auto &v : correctedTopRows) pushRgbToMap(v.toObject(), {QStringLiteral("pigmentCode"), QStringLiteral("code")});

    // 表格样式
    auto tableStyle = QStringLiteral(
        "QTableWidget { border: 1px solid #e4e7ed; border-radius: 4px; background: #fff; "
        "gridline-color: #ebeef5; font-size: 12px; color: #333; }"
        "QHeaderView::section { background: #f5f7fa; border: none; border-bottom: 1px solid #e4e7ed; "
        "padding: 4px 8px; font-size: 12px; color: #606266; }")
        + QLatin1String(kScrollBarQss);

    // 输入框样式
    auto editStyle = QStringLiteral(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 4px; padding: 4px 6px; "
        "font-size: 12px; background: #fff; color: #333; }"
        "QLineEdit:focus { border-color: #409eff; }");

    auto labStyle = QStringLiteral("color: #606266; font-size: 12px;");

        // 单位切换下拉菜单样式
    auto unitComboStyle = QStringLiteral(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 3px 6px; "
        "font-size: 12px; background: #fff; color: #333; min-height: 18px; }"
        "QComboBox:hover { border-color: #409eff; }"
        "QComboBox::drop-down { width: 20px; subcontrol-origin: padding; subcontrol-position: center right; }");
    // 弹出视图样式（单独设置，因为弹出窗口不是 QComboBox 的子控件）
    auto unitPopupStyle = QStringLiteral(
        "QAbstractItemView { border: 1px solid #dcdfe6; background: #fff; "
        "color: #333; selection-background-color: #e8f0fe; selection-color: #333; "
        "padding: 4px; font-size: 13px; }");

    // 跟踪 tonerContainer 内部内容总高度
    int tonerContentH = 0;
    const int tonerSpacing = 4; // tonerContainer layout spacing

    // 为每一层创建独立的表格和罐容量输入（高度自适应，不写死固定位置）
    for (int si = 0; si < sections.size(); ++si) {
        const auto &section = sections[si];
        const QString unit = layerUnit(section.layerKey);

        // 确定当前层的罐容量
        double layerCapacity = m_canCapacity;
        if (m_isThreeStep) {
            if (section.isBase) layerCapacity = m_baseCapacity;
            else layerCapacity = m_surfaceCapacity;
        }

        // 层标题（三工序时显示）
        if (multiLayer) {
            auto *layerTitle = new QLabel(section.title);
            layerTitle->setStyleSheet(QStringLiteral(
                "font-size: 14px; font-weight: 600; color: #1a2332; padding: 4px 0;"));
            layout->addWidget(layerTitle);
            if (tonerContentH > 0) tonerContentH += tonerSpacing;
            tonerContentH += 28; // 层标题高度约 28px
        }

        // 创建表格
        // ── 父级分组表头控件（成分 / 配方(%) / 配方详细信息）──
        // 用独立 HBox 控件放在表格上方，每个分组用带背景色的 QLabel 实现
        auto *groupHeader = new QWidget;
        groupHeader->setStyleSheet(QStringLiteral("background: transparent;"));
        auto *ghLayout = new QHBoxLayout(groupHeader);
        ghLayout->setContentsMargins(2, 0, 2, 0);
        ghLayout->setSpacing(1);
        
        struct HeaderGroup { QString name; int span; QString bg; };
        QList<HeaderGroup> hGroups;
        hGroups.append({T("autosc_component"), 4, "#dae3ed"});
        hGroups.append({T("autosc_formula_percent") , pctCols, "#dce8d8"});
        hGroups.append({T("formula_detail"), 2, "#f0e4d8"});
        for (const auto &g : hGroups) {
            auto *lbl = new QLabel(g.name);
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setStyleSheet(QStringLiteral(
                "background: %1; color: #475569; font-size: 12px; font-weight: 600; "
                "padding: 3px 0; border-radius: 3px;").arg(g.bg));
            lbl->setMinimumHeight(24);
            // 用 stretch factor 模拟列跨占比例
            ghLayout->addWidget(lbl, g.span);
        }
        layout->addWidget(groupHeader);
        if (tonerContentH > 0) tonerContentH += tonerSpacing;
        tonerContentH += 24; // groupHeader 固定高度 24px
        
        auto *table = new QTableWidget;
        table->setColumnCount(cols);
        table->setRowCount(0);
        table->horizontalHeader()->setSectionsMovable(false);
        QStringList headers = {T("autosc_color"), T("autosc_code"), T("sc_name"),
                               T("sc_type"), T("autosc_initial")};
        QVector<int> widths = {56, 90, 120, 100, 88};
        if (hasCorrected) { headers << QStringLiteral("AC"); widths.append(88); }
        if (showAcEdited) { headers << QStringLiteral("AC-Edited"); widths.append(100); }
        headers << T("autosc_qty") + QStringLiteral("(") + unit + QStringLiteral(")")
                << T("autosc_cumulative") + QStringLiteral("(") + unit + QStringLiteral(")");
        widths << 130 << 100;
        table->setHorizontalHeaderLabels(headers);
        // 列宽对标前端 el-table（超出面板宽度时表格底部出现横向滑动条，累计列才可达）
        table->horizontalHeader()->setMinimumSectionSize(52);
        for (int c = 0; c < widths.size() && c < cols; ++c) table->setColumnWidth(c, widths[c]);
        // 配方(%) 三列表头做成可点选列头（对标前端 formula-col-head 按钮）
        for (int c : {colInitial, colAc, colAcEdited}) {
            if (c < 0) continue;
            if (auto *hi = table->horizontalHeaderItem(c)) hi->setTextAlignment(Qt::AlignCenter);
        }
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        // 子表头按分组着色
        QList<QPair<int,int>> childRanges; // {startCol, count}
        int col = 0;
        for (const auto &g : hGroups) {
            for (int c = col; c < col + g.span && c < cols; ++c) {
                auto *hdrItem = table->horizontalHeaderItem(c);
                if (hdrItem) hdrItem->setBackground(QColor(g.bg).lighter(115));
            }
            col += g.span;
        }
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setAlternatingRowColors(true);
        table->verticalHeader()->setVisible(false);
        table->setStyleSheet(tableStyle);

        // 层内三列基准（初始 / AC / AC-Edited），行为初始色母 + 修色多出的色母
        const LayerPcts pcts = computeLayerPcts(section.rows, section.correctedRows,
                                                section.layerKey, m_acEditedPct, vc);

        // 草稿在本层的起始下标（草稿按层顺序构建，与 pcts.rows 顺序一致）
        int draftStart = 0;
        if (m_editing) {
            for (int i = 0; i < m_editDraft.size(); ++i) {
                if (m_editDraft[i].layerKey == section.layerKey) { draftStart = i; break; }
            }
        }

        const QList<double> colGrams = m_editing
            ? QList<double>()
            : gramsFromLayerPcts(pcts, layerCapacity, m_selectedFormulaCol);

        /** 当前选中列下该行的克重；编辑态直接取草稿 */
        auto rowGrams = [&](int idx) -> double {
            if (m_editing) {
                const int di = draftStart + idx;
                return di < m_editDraft.size() ? m_editDraft[di].weightGrams : -1.0;
            }
            return colGrams.value(idx, -1.0);
        };

        // 填充表格数据
        double cum = 0.0;
        int rowIndex = 0;
        for (const auto &v : pcts.rows) {
            table->insertRow(rowIndex);
            const auto t = v.toObject();
            QString code = rowRawCode(t);
            if (code.isEmpty()) code = QStringLiteral("—");
            const QString pigmentName = rowPigmentName(t);
            // 修色列按中文名匹配，展示时再按界面语言切换
            const QString pigmentNameShow = Tn(pigmentName, t[QStringLiteral("pigmentNameEn")].toString());
            const QString type = firstString(t, {QStringLiteral("color_type"), QStringLiteral("colorType")});

            QString chip = hexFromRgb(t[QStringLiteral("rgb")].toObject());
            if (chip.isEmpty()) {
                const QString key = code.trimmed().toUpper();
                if (pigmentRgbMap.contains(key)) chip = pigmentRgbMap[key];
            }
            if (chip.isEmpty()) chip = hashColor(code);

            const double initialPct = pcts.initialPct.value(rowIndex, -1.0);

            // 颜色色块
            auto *colorChip = new QLabel;
            colorChip->setFixedSize(28, 18);
            colorChip->setStyleSheet(QStringLiteral(
                "background: %1; border-radius: 3px; border: 1px solid #dcdfe6;").arg(chip));
            table->setCellWidget(rowIndex, 0, colorChip);

            auto setCell = [table, rowIndex](int col, const QString &text, Qt::Alignment align) {
                auto *item = new QTableWidgetItem(text);
                item->setTextAlignment(align);
                table->setItem(rowIndex, col, item);
            };
            setCell(1, code, Qt::AlignLeft | Qt::AlignVCenter);
            setCell(2, pigmentNameShow.isEmpty() ? QStringLiteral("—") : pigmentNameShow, Qt::AlignLeft | Qt::AlignVCenter);
            setCell(3, type.isEmpty() ? QStringLiteral("—") : type, Qt::AlignLeft | Qt::AlignVCenter);
            setCell(colInitial, initialPct >= 0 ? fmtFixed(initialPct, 2) : QStringLiteral("—"),
                    Qt::AlignRight | Qt::AlignVCenter);

            if (colAc >= 0) {
                // AC 列直接显示修色返回的层内占比（对标前端 parseAcPct）
                const double acPct = pcts.acPct.value(rowIndex, -1.0);
                setCell(colAc, acPct >= 0 ? fmtFixed(acPct, 2) : QStringLiteral("—"),
                        Qt::AlignRight | Qt::AlignVCenter);
            }
            if (colAcEdited >= 0) {
                const double ep = pcts.editedPct.value(rowIndex, -1.0);
                setCell(colAcEdited, ep >= 0 ? fmtFixed(ep, 2) : QStringLiteral("—"),
                        Qt::AlignRight | Qt::AlignVCenter);
            }

            const double grams = rowGrams(rowIndex);
            if (grams >= 0) cum += grams;

            if (m_editing) {
                // 数量列改为输入框（对标前端编辑态的 el-input-number，按该层所选单位输入）
                const int precision = (unit == QLatin1String("kg") || unit == QLatin1String("l")) ? 3 : 1;
                auto *editor = new QLineEdit;
                editor->setValidator(new QDoubleValidator(0.0, 1000000.0, 4, editor));
                editor->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                editor->setStyleSheet(editStyle);
                editor->setText(grams >= 0
                    ? QString::number(weightToDisplayUnit(grams, unit), 'f', precision)
                    : QStringLiteral("0"));
                table->setCellWidget(rowIndex, colQty, editor);

                const int draftIdx = draftStart + rowIndex;
                m_editCells.append({QPointer<QLineEdit>(editor), QPointer<QTableWidget>(table),
                                    rowIndex, draftIdx, unit});
                connect(editor, &QLineEdit::textChanged, this, [this, draftIdx, editor, unit]() {
                    bool ok = false;
                    const double value = editor->text().trimmed().remove(QLatin1Char(',')).toDouble(&ok);
                    if (!ok || value < 0 || draftIdx >= m_editDraft.size()) return;
                    m_editDraft[draftIdx].weightGrams = displayUnitToWeight(value, unit);
                    applyDraftLayerLinkage(m_editDraft[draftIdx].layerKey);
                    applyEditRecalc(editor);
                });
            } else {
                setCell(colQty, grams >= 0 ? displayWeight(grams, unit) : QStringLiteral("—"),
                        Qt::AlignRight | Qt::AlignVCenter);
            }

            setCell(colCum, cum >= 0 ? displayWeight(cum, unit) : QStringLiteral("—"),
                    Qt::AlignRight | Qt::AlignVCenter);
            ++rowIndex;
        }

        // 添加罐容量+合计行作为表格的最后一行
        table->insertRow(rowIndex);
        auto *capWidget = new QWidget;
        auto *capLayout = new QHBoxLayout(capWidget);
        capLayout->setContentsMargins(6, 2, 6, 2);
        capLayout->setSpacing(4);

        // 三个分支原文案一致（罐容量：），直接合并
        auto *capLabel = new QLabel(T("autosc_can_capacity"));
        capLabel->setStyleSheet(labStyle);
        capLayout->addWidget(capLabel);

        auto *capEdit = new QLineEdit;
        capEdit->setValidator(new QDoubleValidator(0.1, 100000.0, 3, capEdit));
        capEdit->setMaximumWidth(100);
        capEdit->setStyleSheet(editStyle);
        capLayout->addWidget(capEdit);

        // 单位切换下拉菜单
        auto *unitCombo = new QComboBox;
        unitCombo->addItems({QStringLiteral("g"), QStringLiteral("kg"), QStringLiteral("l"), QStringLiteral("ml")});
        {
            // 表格重建后回到该层上次选用的单位
            QSignalBlocker block(unitCombo);
            const int unitIdx = unitCombo->findText(unit);
            if (unitIdx >= 0) unitCombo->setCurrentIndex(unitIdx);
        }
        unitCombo->setStyleSheet(unitComboStyle);
        unitCombo->setFixedWidth(70);
        // 弹出视图是独立顶层窗口，需直接在其上设置样式和调色板
        if (auto *popupView = unitCombo->view()) {
            popupView->setMinimumWidth(100);
            popupView->setStyleSheet(unitPopupStyle);
            QPalette pal = popupView->palette();
            pal.setColor(QPalette::Text, QColor("#333333"));
            pal.setColor(QPalette::HighlightedText, QColor("#333333"));
            popupView->setPalette(pal);
        }
        capLayout->addWidget(unitCombo);

        capLayout->addStretch();

        auto *sumLabel = new QLabel;
        sumLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #303133; font-size: 13px;"));
        capLayout->addWidget(sumLabel);

        table->setCellWidget(rowIndex, 0, capWidget);
        table->setSpan(rowIndex, 0, 1, cols);  // 跨所有列

        // 表格高度自适应：header(40) + 数据行×36 + 罐容量行×36 + 底部边距
        // 额外预留横向滚动条高度：列宽超出面板时滑条才不会被固定高度裁掉（累计列可见）
        const int dataRows = rowIndex;
        const int rowH = 36;
        const int headerH = 40;
        const int hBarH = table->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        int tableH = headerH + (dataRows + 1) * rowH + 4 + hBarH;
        table->setFixedHeight(tableH);

        // 保存表格指针
        if (!multiLayer) {
            m_tonerTable = table;
        } else if (section.isBase) {
            m_baseTable = table;
        } else {
            m_surfaceTable = table;
        }

        layout->addWidget(table);
        if (tonerContentH > 0) tonerContentH += tonerSpacing;
        tonerContentH += tableH;

        // 设置初始值（内部始终以克存储，罐容量与总计按所选单位显示）
        const int capPrecision = (unit == QLatin1String("kg") || unit == QLatin1String("l")) ? 3 : 1;
        capEdit->setText(QString::number(weightToDisplayUnit(layerCapacity, unit), 'f', capPrecision));
        sumLabel->setText(T("autosc_total_sum_html").arg(displayWeight(layerCapacity, unit)));
        // 编辑态锁定罐容量与单位（对标前端 :disabled="editing"）
        capEdit->setEnabled(!m_editing);
        unitCombo->setEnabled(!m_editing);

        // 保存指针并连接信号
        if (!multiLayer) {
            m_canCapacityEdit = capEdit;
            m_tonerTotalLabel = sumLabel;
        } else if (section.isBase) {
            m_baseCapacityEdit = capEdit;
            m_baseTotalLabel = sumLabel;
        } else {
            m_surfaceCapacityEdit = capEdit;
            m_surfaceTotalLabel = sumLabel;
        }
        if (m_editing) {
            m_editSections.append({section.layerKey, unit, QPointer<QLabel>(sumLabel)});
        }

        // 记录该层表格视图：点表头切换配方列时按它就地刷新，不重建表格
        FormulaColumnView view;
        view.table = table;
        view.layerKey = section.layerKey;
        view.isBase = section.isBase;
        view.rows = section.rows;
        view.correctedRows = section.correctedRows;
        view.colInitial = colInitial;
        view.colAc = colAc;
        view.colAcEdited = colAcEdited;
        view.colQty = colQty;
        view.colCum = colCum;
        view.unit = unit;
        if (auto *hi = table->horizontalHeaderItem(colInitial))
            view.headerBg = hi->background().color();
        m_formulaViews.append(view);

        const QString layerKey = section.layerKey;
        // 罐容量/单位变化：重算克数后重建表格（编辑态控件已禁用，不会触发）
        auto reloadFromCapacity = [this, capEdit, unitCombo, si]() {
            bool ok = false;
            const double displayVal = capEdit->text().trimmed().toDouble(&ok);
            if (!ok || displayVal <= 0) return;
            const double grams = displayUnitToWeight(displayVal, unitCombo->currentText());
            if (si == 0 && m_isThreeStep) m_baseCapacity = grams;
            else if (si == 1 && m_isThreeStep) m_surfaceCapacity = grams;
            else m_canCapacity = grams;
            QTimer::singleShot(0, this, [this]() {
                rebuildTonerTable(m_currentBaseRows, m_currentTopRows,
                                  m_correctedBaseRows, m_correctedTopRows);
            });
        };
        connect(capEdit, &QLineEdit::editingFinished, this, [reloadFromCapacity]() { reloadFromCapacity(); });
        // 单位变化：容量与草稿的克数不变，只记住单位并整表按新单位重渲染（对标前端 canUnit）
        connect(unitCombo, &QComboBox::currentTextChanged, this, [this, layerKey, unitCombo]() {
            const QString newUnit = unitCombo->currentText();
            if (newUnit.isEmpty() || m_layerUnits.value(layerKey, QStringLiteral("g")) == newUnit) return;
            m_layerUnits[layerKey] = newUnit;
            QTimer::singleShot(0, this, [this]() {
                rebuildTonerTable(m_currentBaseRows, m_currentTopRows,
                                  m_correctedBaseRows, m_correctedTopRows);
            });
        });
    }

    // 设置 tonerContainer 固定高度，确保所有内容可见
    if (m_tonerContainer && tonerContentH > 0) {
        m_tonerContainer->setFixedHeight(tonerContentH);
    }

    // 表头列切换（初始 / AC / AC-Edited）：点表头即切换数量与累计的取数口径，编辑态禁用
    auto connectColSwitch = [this](QTableWidget *table, int initialCol, int acCol, int editedCol) {
        if (!table) return;
        auto *hdr = table->horizontalHeader();
        hdr->setSectionsClickable(true);
        connect(hdr, &QHeaderView::sectionClicked, this,
                [this, initialCol, acCol, editedCol](int column) {
            if (m_editing) return;
            QString newCol;
            if (column == initialCol) newCol = QStringLiteral("initial");
            else if (acCol >= 0 && column == acCol) newCol = QStringLiteral("ac");
            else if (editedCol >= 0 && column == editedCol) newCol = QStringLiteral("acEdited");
            else return;
            if (newCol == QLatin1String("ac") && !m_hasCorrection) return;
            if (newCol == QLatin1String("acEdited") && !m_hasAcEdited) return;
            if (m_selectedFormulaCol != newCol) {
                m_selectedFormulaCol = newCol;
                // 只换显示口径，不重建表格：滚动位置与面板高度保持不变
                applyFormulaColumnSelection();
            }
        });
    };
    connectColSwitch(m_tonerTable, colInitial, colAc, colAcEdited);
    connectColSwitch(m_baseTable, colInitial, colAc, colAcEdited);
    connectColSwitch(m_surfaceTable, colInitial, colAc, colAcEdited);

    applyFormulaColumnSelection();

    // 配方面板高度自适应：延迟到布局完成后测量实际内容高度
    if (m_recipePanel && m_tonerContainer) {
        auto *rp = static_cast<ResizablePanel *>(m_recipePanel);
        QTimer::singleShot(0, this, [this, rp, keptScroll]() {
            auto *rLayout = rp->contentLayout();
            rLayout->invalidate();
            // 关键：用 sizeHint().height() 计算所需高度，不受当前面板 fixed height 压缩影响
            int contentH = rLayout->sizeHint().height();
            // ResizablePanel 固定开销：
            //   collapsedH(46=outer2+content10+header~22+content10+outer2)
            // + bodyLayout addSpacing(8)
            // + dragBar(10) = 64
            const int panelOverhead = 64;
            int panelH = contentH + panelOverhead;
            rp->setFixedHeight(qMax(250, panelH));
            updateScrollContainerHeight();
            // 面板高度定下来后回到重建前的位置
            if (m_mainScroll) {
                auto *bar = m_mainScroll->verticalScrollBar();
                bar->setValue(qMin(keptScroll, bar->maximum()));
            }
        });
    }
}

// 点配方(%)表头切换取数口径：就地改表头高亮与数量/累计文本，不重建表格，页面滚动位置保持不变
void AutoScPage::applyFormulaColumnSelection()
{
    const VendorCodes vc{m_correctedVendorData, m_pigmentCodeSystem};
    const QColor tint = formulaColTint(m_selectedFormulaCol);

    for (const auto &view : m_formulaViews) {
        auto *table = view.table.data();
        if (!table) continue;

        int selCol = view.colInitial;
        if (m_selectedFormulaCol == QLatin1String("ac") && view.colAc >= 0) selCol = view.colAc;
        else if (m_selectedFormulaCol == QLatin1String("acEdited") && view.colAcEdited >= 0)
            selCol = view.colAcEdited;

        // 表头：选中列加粗并上底色，其余恢复分组原色
        for (int c : {view.colInitial, view.colAc, view.colAcEdited}) {
            if (c < 0) continue;
            auto *hi = table->horizontalHeaderItem(c);
            if (!hi) continue;
            hi->setBackground(c == selCol ? tint : view.headerBg);
            QFont f = hi->font();
            f.setBold(c == selCol);
            hi->setFont(f);
        }

        // 编辑态的数量列是输入框，口径切换不参与（确认时按草稿重建）
        if (m_editing) continue;

        double capacity = m_canCapacity;
        if (m_isThreeStep) capacity = view.isBase ? m_baseCapacity : m_surfaceCapacity;
        const LayerPcts pcts = computeLayerPcts(view.rows, view.correctedRows,
                                              view.layerKey, m_acEditedPct, vc);
        const QList<double> grams = gramsFromLayerPcts(pcts, capacity, m_selectedFormulaCol);

        const int dataRows = table->rowCount() - 1;   // 末行为罐容量行
        double cum = 0.0;
        for (int r = 0; r < dataRows; ++r) {
            const double g = grams.value(r, -1.0);
            if (g >= 0) cum += g;

            if (!table->cellWidget(r, view.colQty)) {
                if (auto *qtyItem = table->item(r, view.colQty))
                    qtyItem->setText(g >= 0 ? formatWeightValue(weightToDisplayUnit(g, view.unit))
                                            : QStringLiteral("—"));
            }
            if (auto *cumItem = table->item(r, view.colCum))
                cumItem->setText(formatWeightValue(weightToDisplayUnit(cum, view.unit)));

            for (int c : {view.colInitial, view.colAc, view.colAcEdited}) {
                if (c < 0) continue;
                auto *it = table->item(r, c);
                if (!it) continue;
                it->setBackground(c == selCol ? QBrush(tint) : QBrush(Qt::NoBrush));
            }
        }
    }

    // 修色只针对初始列，切列后按钮启用态要跟着变
    syncCorrectButtonState();
}

QString AutoScPage::editLayerKey(bool isBaseLayer) const
{
    return m_isThreeStep
        ? (isBaseLayer ? QStringLiteral("base") : QStringLiteral("surface"))
        : QStringLiteral("single");
}

void AutoScPage::resetEditState()
{
    m_editing = false;
    m_editDraft.clear();
    m_editCells.clear();
    m_editSections.clear();
    m_acEditedPct.clear();
    m_hasAcEdited = false;
    m_selectedFormulaCol = QStringLiteral("initial");
}

void AutoScPage::syncEditActionButtons()
{
    const bool hasRows = !m_currentBaseRows.isEmpty() || !m_currentTopRows.isEmpty();
    const bool colValid = (m_selectedFormulaCol != QLatin1String("ac") || m_hasCorrection)
                       && (m_selectedFormulaCol != QLatin1String("acEdited") || m_hasAcEdited);
    const bool busy = m_editing || m_predicting;

    if (m_editBtn) {
        m_editBtn->setVisible(!m_editing);
        m_editBtn->setEnabled(hasRows && colValid && !busy);
    }
    if (m_cancelEditBtn) m_cancelEditBtn->setVisible(m_editing);
    if (m_confirmEditBtn) m_confirmEditBtn->setVisible(m_editing);
    if (m_confirmEditBtn) m_confirmEditBtn->setEnabled(!m_predicting);
    if (m_saveBtn) m_saveBtn->setEnabled(m_saveAllowed && !busy);

    // 编辑/预测期间不允许下载与保存以外的操作（对标前端 editing 时的禁用集合）
    if (m_downloadBaseBtn) m_downloadBaseBtn->setEnabled(!busy && hasRows && colValid);
    if (m_downloadSurfaceBtn) m_downloadSurfaceBtn->setEnabled(!busy && hasRows && colValid);
    if (m_downloadSingleBtn) m_downloadSingleBtn->setEnabled(!busy && hasRows && colValid);

    syncCorrectButtonState();
}

void AutoScPage::startEditFormula()
{
    if (m_editing || m_predicting) return;
    if (m_currentRowObj.isEmpty()) {
        QMessageBox::information(this, T("info"), T("autosc_select_formula_first"));
        return;
    }
    if (m_currentBaseRows.isEmpty() && m_currentTopRows.isEmpty()) {
        QMessageBox::information(this, T("info"), T("autosc_select_result_first"));
        return;
    }

    struct LayerSrc { bool isBase; QJsonArray rows; QJsonArray correctedRows; };
    QList<LayerSrc> layers;
    if (!m_currentBaseRows.isEmpty()) layers.append({true, m_currentBaseRows, m_correctedBaseRows});
    if (!m_currentTopRows.isEmpty()) layers.append({false, m_currentTopRows, m_correctedTopRows});

    // 草稿按层顺序展开，下标与 rebuildTonerTable 里的行一一对应
    const VendorCodes vc{m_correctedVendorData, m_pigmentCodeSystem};
    m_editDraft.clear();
    for (const auto &layer : layers) {
        const QString layerKey = editLayerKey(layer.isBase);
        const double capacity = m_isThreeStep
            ? (layer.isBase ? m_baseCapacity : m_surfaceCapacity)
            : m_canCapacity;
        const LayerPcts pcts = computeLayerPcts(layer.rows, layer.correctedRows,
                                              layerKey, m_acEditedPct, vc);
        const QList<double> grams = gramsFromLayerPcts(pcts, capacity, m_selectedFormulaCol);

        for (int i = 0; i < pcts.rows.size(); ++i) {
            const auto t = pcts.rows[i].toObject();
            EditDraftRow draft;
            draft.layerKey = layerKey;
            draft.code = rowAcCode(t).toUpper();
            draft.pigmentName = rowPigmentName(t);
            // 面板按厂商编号体系显示，但 predict 要交给 SDK 的是内部编号
            draft.predictCode = rowInternalCode(vc, t).toUpper();
            // 编号缺失的行也要占位，否则草稿下标与表格行会错位
            draft.weightGrams = qMax(0.0, grams.value(i, 0.0));
            m_editDraft.append(draft);
        }
    }
    if (m_editDraft.isEmpty()) return;

    m_editing = true;
    rebuildTonerTable(m_currentBaseRows, m_currentTopRows, m_correctedBaseRows, m_correctedTopRows);
    syncEditActionButtons();
}

void AutoScPage::cancelEditFormula()
{
    if (!m_editing) return;
    m_editing = false;
    m_editDraft.clear();
    rebuildTonerTable(m_currentBaseRows, m_currentTopRows, m_correctedBaseRows, m_correctedTopRows);
    syncEditActionButtons();
}

/** 编辑态：本层总量变化后，另一层按原比例跟随（对标前端 applyLayerRatios） */
void AutoScPage::applyDraftLayerLinkage(const QString &layerKey)
{
    if (!m_isThreeStep) return;
    const QString otherKey = layerKey == QLatin1String("surface")
        ? QStringLiteral("base") : QStringLiteral("surface");
    double curTotal = 0.0, otherTotal = 0.0;
    for (const auto &d : m_editDraft) {
        if (d.layerKey == layerKey) curTotal += d.weightGrams;
        else if (d.layerKey == otherKey) otherTotal += d.weightGrams;
    }
    if (curTotal <= 0 || otherTotal <= 0) return;
    const double ratio = curTotal / otherTotal;
    for (auto &d : m_editDraft) {
        if (d.layerKey == otherKey) d.weightGrams *= ratio;
    }
}

void AutoScPage::applyEditRecalc(QLineEdit *skip)
{
    if (!m_editing) return;

    QMap<QString, double> layerTotals;
    for (const auto &d : m_editDraft) layerTotals[d.layerKey] += d.weightGrams;

    // 数量列（输入框）与累计列（文本）按草稿顺序联动，均按该层单位显示
    QMap<QString, double> running;
    for (const auto &cell : m_editCells) {
        if (!cell.editor || !cell.table) continue;
        if (cell.draftIdx < 0 || cell.draftIdx >= m_editDraft.size()) continue;
        const EditDraftRow &d = m_editDraft[cell.draftIdx];
        const QString unit = cell.unit.isEmpty() ? QStringLiteral("g") : cell.unit;
        running[d.layerKey] += d.weightGrams;
        if (cell.editor.data() != skip) {
            QSignalBlocker block(cell.editor.data());   // 程序回填不再触发联动，避免与输入互相回写
            const int precision = (unit == QLatin1String("kg") || unit == QLatin1String("l")) ? 3 : 1;
            cell.editor->setText(QString::number(weightToDisplayUnit(d.weightGrams, unit), 'f', precision));
        }
        if (auto *cumItem = cell.table->item(cell.row, m_editCumCol))
            cumItem->setText(formatWeightValue(weightToDisplayUnit(running.value(d.layerKey), unit)));
    }

    // 编辑态只更新该层合计；罐容量保持用户设定值，确认时按它等比归一数量
    for (const auto &sec : m_editSections) {
        const double total = layerTotals.value(sec.layerKey, 0.0);
        const QString unit = sec.unit.isEmpty() ? QStringLiteral("g") : sec.unit;
        if (sec.sumLabel) {
            sec.sumLabel->setText(T("autosc_total_sum_html")
                                      .arg(formatWeightValue(weightToDisplayUnit(total, unit))));
        }
    }
}

void AutoScPage::confirmEditFormula()
{
    if (!m_editing) return;

    QMap<QString, double> layerTotals;
    for (const auto &d : m_editDraft) layerTotals[d.layerKey] += d.weightGrams;

    // 编辑改的是比例，罐容量保持不变（对标「按罐容量等比归一」口径）
    auto capacityOfLayer = [this](const QString &layerKey) {
        if (!m_isThreeStep) return m_canCapacity;
        if (layerKey == QLatin1String("base")) return m_baseCapacity;
        if (layerKey == QLatin1String("surface")) return m_surfaceCapacity;
        return m_canCapacity;
    };

    m_acEditedPct.clear();
    for (const auto &d : m_editDraft) {
        const double total = layerTotals.value(d.layerKey, 0.0);
        if (d.code.isEmpty() || total <= 0) continue;
        m_acEditedPct[editedPctKey(d.layerKey, d.code)] = (d.weightGrams / total) * 100.0;
    }

    QJsonArray components;
    for (const auto &d : m_editDraft) {
        if (d.predictCode.isEmpty() || d.weightGrams < 0) continue;
        const double total = layerTotals.value(d.layerKey, 0.0);
        const double capacity = capacityOfLayer(d.layerKey);
        const double grams = (total > 0 && capacity > 0)
            ? capacity * d.weightGrams / total
            : d.weightGrams;
        QJsonObject comp;
        comp[QStringLiteral("colorantName")] = d.predictCode;
        comp[QStringLiteral("pigmentName")] = d.pigmentName;
        comp[QStringLiteral("concentration")] = std::round(grams * 10000.0) / 10000.0;
        components.append(comp);
    }

    m_editing = false;
    m_editDraft.clear();
    m_hasAcEdited = !m_acEditedPct.isEmpty();
    m_selectedFormulaCol = QStringLiteral("acEdited");
    rebuildTonerTable(m_currentBaseRows, m_currentTopRows, m_correctedBaseRows, m_correctedTopRows);
    syncEditActionButtons();

    runPredict(components);
}

void AutoScPage::restoreAcEditedFromRow()
{
    const auto predictDetail = m_currentRowObj[QStringLiteral("predictDetail")].toObject();
    if (!predictDetail.isEmpty())
        applyEditedComponents(predictDetail[QStringLiteral("components")].toArray());
    syncEditActionButtons();
}

bool AutoScPage::applyEditedComponents(const QJsonArray &components)
{
    if (components.isEmpty()) return false;

    struct LayerSrc { bool isBase; QJsonArray rows; QJsonArray correctedRows; };
    QList<LayerSrc> layers;
    if (!m_currentBaseRows.isEmpty()) layers.append({true, m_currentBaseRows, m_correctedBaseRows});
    if (!m_currentTopRows.isEmpty()) layers.append({false, m_currentTopRows, m_correctedTopRows});
    if (layers.isEmpty()) return false;

    const VendorCodes vc{m_correctedVendorData, m_pigmentCodeSystem};
    QMap<QString, double> next;
    for (const auto &layer : layers) {
        const QString layerKey = editLayerKey(layer.isBase);
        // 行集与表格一致：初始色母 + 修色多出的色母
        const QJsonArray rows = mergeLayerRowsWithAc(layer.rows, layer.correctedRows, vc);
        const QMap<QString, QJsonObject> acMap = buildLayerAcMap(
            rows, filterComponentsForLayer(layerKey, components, m_isThreeStep), vc);
        QList<double> weights;
        for (const auto &v : rows) {
            const auto it = acMap.constFind(rowInternalCode(vc, v.toObject()).toUpper());
            weights.append(it != acMap.constEnd() ? acComponentPct(it.value()) : 0.0);
        }
        double total = 0.0;
        for (double w : weights) total += w;
        if (total <= 0) continue;
        for (int i = 0; i < rows.size(); ++i) {
            const QString code = rowAcCode(rows[i].toObject());
            if (code.isEmpty()) continue;
            next[editedPctKey(layerKey, code)] = weights.value(i) / total * 100.0;
        }
    }
    if (next.isEmpty()) return false;

    m_acEditedPct = next;
    m_hasAcEdited = true;

    // 预测回来的只是比例来源，罐容量以面板现值为准（数量按容量等比换算）
    if (!m_editing) m_selectedFormulaCol = QStringLiteral("acEdited");
    return true;
}

void AutoScPage::runPredict(const QJsonArray &components)
{
    if (m_currentMeasurementId.isEmpty()) {
        QMessageBox::information(this, T("info"), T("autosc_select_meas_data_first"));
        return;
    }
    if (components.isEmpty()) {
        QMessageBox::information(this, T("info"), T("autosc_predict_no_components"));
        return;
    }

    bool okId = false;
    qint64 formulaId = m_currentRowObj[QStringLiteral("formulaId")].toVariant().toString().toLongLong(&okId);
    if (!okId || formulaId <= 0)
        formulaId = m_currentRowObj[QStringLiteral("id")].toVariant().toString().toLongLong(&okId);

    QJsonObject body;
    body[QStringLiteral("measurementId")] = m_currentMeasurementId.toLongLong();
    body[QStringLiteral("components")] = components;
    if (formulaId > 0) body[QStringLiteral("formulaId")] = formulaId;
    const int calsetIdx = m_calsetCombo->currentIndex();
    if (calsetIdx > 0 && calsetIdx - 1 < m_calsets.size())
        body[QStringLiteral("sdkCalsetId")] = m_calsets[calsetIdx - 1].id.toLongLong();
    body[QStringLiteral("industryType")] = coatTypeCanonical(m_currentRowObj[QStringLiteral("coatType")].toString())
        == QStringLiteral("素色") ? QStringLiteral("PAINT") : QStringLiteral("EFFECTS");

    m_predicting = true;
    syncEditActionButtons();

    ScApi::instance().predictColor(body,
        [this](const QJsonObject &res) {
            m_predicting = false;
            if (res[QStringLiteral("code")].toInt() != 200) {
                QMessageBox::warning(this, T("autosc_predict_failed"),
                    res[QStringLiteral("msg")].toString());
                syncEditActionButtons();
                return;
            }
            auto data = res[QStringLiteral("data")].toObject();
            if (data.isEmpty()) data = res;
            applyPredictDetail(data);
            syncEditActionButtons();
            QMessageBox::information(this, T("autosc_predict"), T("autosc_predict_done"));
        },
        [this](const QString &msg, int) {
            m_predicting = false;
            syncEditActionButtons();
            QMessageBox::warning(this, T("autosc_predict_failed"), msg);
        });
}

void AutoScPage::applyPredictDetail(const QJsonObject &predictData)
{
    if (predictData.isEmpty()) return;

    // 预测的色度/光谱归到 acEdited 来源（对标前端 remapScPredictRawToAcEdited）
    QJsonObject detail = predictData;
    static const QStringList kVizArrayKeys = {
        QStringLiteral("correctedColorimetry"), QStringLiteral("correctedSpectra"),
        QStringLiteral("colorimetry"), QStringLiteral("spectra"),
        QStringLiteral("spectralData")};
    for (const auto &key : kVizArrayKeys) {
        QJsonArray arr = detail[key].toArray();
        if (arr.isEmpty()) continue;
        for (int i = 0; i < arr.size(); ++i) {
            QJsonObject o = arr[i].toObject();
            if (o.isEmpty()) continue;
            o[QStringLiteral("sourceKey")] = QStringLiteral("acEdited");
            o[QStringLiteral("sourceName")] = QStringLiteral("AC-Edited");
            if (o[QStringLiteral("name")].toString().isEmpty())
                o[QStringLiteral("name")] = QStringLiteral("AC-Edited");
            arr[i] = o;
        }
        detail[key] = arr;
    }
    detail[QStringLiteral("scoreAcEdited")] = predictData[QStringLiteral("scoreAc")];
    detail[QStringLiteral("shapeAcEdited")] = predictData[QStringLiteral("shape")];
    detail[QStringLiteral("spectralAcEdited")] = predictData[QStringLiteral("spectral")];

    QJsonObject updated = m_currentRowObj;
    updated[QStringLiteral("predictDetail")] = detail;
    updated[QStringLiteral("scoreAcEdited")] = detail[QStringLiteral("scoreAcEdited")];
    m_currentRowObj = updated;
    refreshAcEditedScoreLabel();

    // 写回结果表行，切换选中或换编号体系后仍可恢复 AC-Edited
    if (m_resultTable && m_resultTable->currentRow() >= 0) {
        const int row = m_resultTable->currentRow();
        if (auto *item0 = m_resultTable->item(row, 0)) {
            QJsonObject p = QJsonDocument::fromJson(item0->data(Qt::UserRole).toByteArray()).object();
            p[QStringLiteral("predictDetail")] = detail;
            p[QStringLiteral("scoreAcEdited")] = detail[QStringLiteral("scoreAcEdited")];
            const QByteArray payload = QJsonDocument(p).toJson();
            for (int c = 0; c < m_resultTable->columnCount(); ++c)
                if (auto *it = m_resultTable->item(row, c)) it->setData(Qt::UserRole, payload);
        }
    }

    // 预测返回的组分是权威克重，用它重建 AC-Edited 占比
    if (!applyEditedComponents(predictData[QStringLiteral("components")].toArray())) {
        m_hasAcEdited = !m_acEditedPct.isEmpty();
    }

    rebuildTonerTable(m_currentBaseRows, m_currentTopRows, m_correctedBaseRows, m_correctedTopRows);
    for (auto &card : m_vizCards) updateVizCardContent(card);
    adjustVizPanelHeight();
}

void AutoScPage::clearRecipePanel()
{
    m_recipeMetaLabel->setText(QStringLiteral("—"));
    m_recipeCoatTypeLabel->setText(QStringLiteral("—"));
    m_recipeScoreOLabel->setText(QStringLiteral("—"));
    m_recipeScoreAcLabel->setText(QStringLiteral("—"));
    m_recipeInfoLabel->setText(T("autosc_select_result_hint"));
    m_currentBaseRows = QJsonArray();
    m_currentTopRows = QJsonArray();
    m_correctedBaseRows = QJsonArray();
    m_correctedTopRows = QJsonArray();
    m_hasCorrection = false;
    // 清空动态表格容器
    if (m_tonerContainer) {
        auto *tLayout = m_tonerContainer->layout();
        while (tLayout && tLayout->count() > 0) {
            auto *item = tLayout->takeAt(0);
            if (item->widget()) item->widget()->deleteLater();
        }
    }
    m_tonerTable = nullptr;
    m_baseTable = nullptr;
    m_surfaceTable = nullptr;
    m_canCapacityEdit = nullptr;
    m_baseCapacityEdit = nullptr;
    m_surfaceCapacityEdit = nullptr;
    m_tonerTotalLabel = nullptr;
    m_baseTotalLabel = nullptr;
    m_surfaceTotalLabel = nullptr;
    m_currentRowObj = QJsonObject();
    resetEditState();
    if (m_recipeScoreAcEditedGroup) m_recipeScoreAcEditedGroup->setVisible(false);
    if (m_recipeScoreAcEditedLabel) m_recipeScoreAcEditedLabel->setText(QStringLiteral("—"));
    syncEditActionButtons();
    syncCorrectButtonState();
    // 隐藏下载按钮
    if (m_downloadBaseBtn) m_downloadBaseBtn->setVisible(false);
    if (m_downloadSurfaceBtn) m_downloadSurfaceBtn->setVisible(false);
    if (m_downloadSingleBtn) m_downloadSingleBtn->setVisible(false);
    // 重置配方面板高度（清空后内容变少）
    if (m_recipePanel) {
        // 清空后只有 metaRow + infoLabel + actionRow + 间距
        int contentH = 30 + 20 + 35 + 24;  // 109px
        int panelH = contentH + 72;  // +ResizablePanel 开销
        static_cast<ResizablePanel *>(m_recipePanel)->setFixedHeight(qMax(200, panelH));
        updateScrollContainerHeight();
        // 配方内容已清空（换数据集）：这时才回顶部
        if (m_mainScroll) m_mainScroll->verticalScrollBar()->setValue(0);
    }
}

// ─── 修色/预测 ───────────────────────────────────────────────
void AutoScPage::syncCorrectButtonState()
{
    // 修色(CLASSIC)：当前选中行已修色则禁用；智能修色(SMART)：始终可用
    // 编辑态/预测中不触发修色，且修色只针对初始列（对标前端 canCorrect）
    const bool corrected = m_currentRowObj[QStringLiteral("corrected")].toBool();
    const bool busy = m_editing || m_predicting || m_searching;
    const bool initialCol = m_selectedFormulaCol == QLatin1String("initial");
    if (m_correctBtn) m_correctBtn->setEnabled(!corrected && !busy && initialCol);
    if (m_smartCorrectBtn) m_smartCorrectBtn->setEnabled(!busy && initialCol);
}

void AutoScPage::onCorrect()
{
    runCorrect(QStringLiteral("CLASSIC"));
}

void AutoScPage::onSmartCorrect()
{
    runCorrect(QStringLiteral("SMART"));
}

void AutoScPage::runCorrect(const QString &method)
{
    if (m_resultTable->currentRow() < 0) {
        QMessageBox::information(this, T("info"), T("autosc_select_result_first"));
        return;
    }
    if (m_currentMeasurementId.isEmpty()) {
        QMessageBox::information(this, T("info"), T("autosc_select_meas_data_first"));
        return;
    }

    auto *item = m_resultTable->item(m_resultTable->currentRow(), 0);
    if (!item) return;
    QJsonObject rowObj = QJsonDocument::fromJson(item->data(Qt::UserRole).toByteArray()).object();

    // 与前端 buildScCorrectBody 一致：measurementId + formulaIds[] + correctionMethod
    bool ok = false;
    const double formulaId = rowObj[QStringLiteral("formulaId")].toVariant().toDouble(&ok);
    if (!ok || formulaId <= 0) {
        QMessageBox::information(this, T("info"), T("autosc_missing_formula_id"));
        return;
    }

    QJsonObject body;
    body[QStringLiteral("measurementId")] = m_currentMeasurementId.toLongLong();
    body[QStringLiteral("formulaIds")] = QJsonArray{formulaId};
    body[QStringLiteral("correctionMethod")] = method == QLatin1String("SMART")
        ? QStringLiteral("SMART") : QStringLiteral("CLASSIC");
    int calsetIdx = m_calsetCombo->currentIndex();
    if (calsetIdx > 0 && calsetIdx - 1 < m_calsets.size()) {
        body[QStringLiteral("sdkCalsetId")] = m_calsets[calsetIdx - 1].id.toLongLong();
    }
    // inferScIndustryType：素色 → PAINT，其余 → EFFECTS
    body[QStringLiteral("industryType")] = coatTypeCanonical(rowObj[QStringLiteral("coatType")].toString())
        == QStringLiteral("素色") ? QStringLiteral("PAINT") : QStringLiteral("EFFECTS");

    m_correctBtn->setEnabled(false);   // 请求进行中临时禁用修色；智能修色始终可点

    ScApi::instance().correctColor(body,
        [this, method, formulaId](const QJsonObject &res) {
            syncCorrectButtonState();
            if (res[QStringLiteral("code")].toInt() != 200) {
                QMessageBox::warning(this, T("autosc_correct_failed"),
                    res[QStringLiteral("msg")].toString());
                return;
            }
            const auto data = res[QStringLiteral("data")].toObject();
            
            // 修色结果在 data.results[formulaId] 中
            QJsonObject resultForFormula;
            const auto results = data[QStringLiteral("results")].toObject();
            const QString formulaIdStr = QString::number(int(formulaId));
            if (results.contains(formulaIdStr)) {
                resultForFormula = results[formulaIdStr].toObject();
            }
            
            if (!resultForFormula.isEmpty()) {
                // 修色结果合入当前行后重刷配方面板
                QJsonObject updated = m_currentRowObj;
                // 合入修色结果
                for (auto it = resultForFormula.begin(); it != resultForFormula.end(); ++it) {
                    updated[it.key()] = it.value();
                }
                // 标记为已修色
                updated[QStringLiteral("corrected")] = true;
                loadRecipePanel(updated);
            }
            QMessageBox::information(this,
                method == QLatin1String("SMART") ? T("autosc_smart_correct") : T("autosc_correct"),
                T("autosc_correct_done"));
        },
        [this](const QString &msg, int) {
            syncCorrectButtonState();
            QMessageBox::warning(this, T("autosc_correct_failed"), msg);
        });
}

// ─── 搜索后自动修色（对标前端 autoCorrectScSearchRows） ────────
namespace {
constexpr double kAutoCorrectMinScoreO = 1.0;   // 前端 AUTO_CORRECT_MIN_SCORE_O
constexpr int    kAutoCorrectBatchSize = 2;     // 前端 batchSize
constexpr int    kColScoreAc = 4;               // 结果表"分数(AC)"列

/** 驱动不可用时的说明文案：区分「文件没找到」和「找到了但加载失败」 */
QString sdx4UnavailableHint(const XriteDevice &dev)
{
    return dev.driverStatus() == XriteDevice::DriverStatus::LoadFailed
        ? T("autosc_sdx4_load_failed").arg(dev.driverError())
        : T("autosc_sdx4_dll_missing");
}
}

void AutoScPage::markRowCorrectPending(int row)
{
    if (!m_resultTable) return;
    auto *cell = m_resultTable->item(row, kColScoreAc);
    if (!cell) return;
    cell->setText(QStringLiteral("…"));
    cell->setForeground(QColor(0x40, 0x9e, 0xff));   // el-color-primary
}

void AutoScPage::markRowCorrectFailed(int row)
{
    if (!m_resultTable) return;
    if (auto *item0 = m_resultTable->item(row, 0)) {
        QJsonObject p = QJsonDocument::fromJson(item0->data(Qt::UserRole).toByteArray()).object();
        p[QStringLiteral("correctFailed")] = true;
        const QByteArray payload = QJsonDocument(p).toJson();
        for (int c = 0; c < m_resultTable->columnCount(); ++c)
            if (auto *it = m_resultTable->item(row, c)) it->setData(Qt::UserRole, payload);
    }
    auto *cell = m_resultTable->item(row, kColScoreAc);
    if (!cell) return;
    cell->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    cell->setText(T("autosc_correct_failed"));        // 前端 sc-score-fail
    cell->setForeground(QColor(0xf5, 0x6c, 0x6c));    // el-color-danger
}

void AutoScPage::patchRowCorrected(int row, const QJsonObject &entry)
{
    if (!m_resultTable) return;
    auto *item0 = m_resultTable->item(row, 0);
    if (!item0) return;
    QJsonObject p = QJsonDocument::fromJson(item0->data(Qt::UserRole).toByteArray()).object();

    // 与 applyScCorrectToRow 一致：标记已修色并回填关键字段
    p[QStringLiteral("corrected")] = true;
    p[QStringLiteral("correctFailed")] = false;
    for (const QString &k : {QStringLiteral("scoreAc"), QStringLiteral("shapeAc"),
                             QStringLiteral("components"), QStringLiteral("rgbByGeometry"),
                             QStringLiteral("targetRgbByGeometry"), QStringLiteral("targetTexture")})
        if (entry.contains(k)) p[k] = entry[k];
    p[QStringLiteral("correctDetail")] = entry;

    const QByteArray payload = QJsonDocument(p).toJson();
    for (int c = 0; c < m_resultTable->columnCount(); ++c)
        if (auto *it = m_resultTable->item(row, c)) it->setData(Qt::UserRole, payload);

    // 更新"分数(AC)"单元格
    auto *cell = m_resultTable->item(row, kColScoreAc);
    if (cell) {
        cell->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        const auto sa = p[QStringLiteral("scoreAc")];
        if (sa.isDouble()) {
            cell->setText(fmtFixed(sa.toDouble(), m_scoreDecimalPlaces));
            const QString tone = scoreTone(sa.toDouble(),
                isEffectSearch() ? m_effectCorrected : m_solidCorrected);
            cell->setForeground(tone.isEmpty() ? QColor(QStringLiteral("#303133")) : scoreToneColor(tone));
        } else {
            cell->setText(QStringLiteral("—"));
            cell->setForeground(QColor(0xa8, 0xab, 0xb2));
        }
    }

    // 若正是当前选中行，刷新配方面板
    if (row == m_resultTable->currentRow()) {
        loadRecipePanel(p);
        // 同时刷新可视化卡片（合并 AC 修色数据）
        for (auto &card : m_vizCards) {
            updateVizCardContent(card);
        }
    }
}

void AutoScPage::applyAutoCorrectResult(const AutoCorrectBatch &batch, const QJsonObject &res)
{
    if (res[QStringLiteral("code")].toInt() != 200) {
        for (int r : batch.rows) markRowCorrectFailed(r);
        return;
    }
    const QJsonObject data = res[QStringLiteral("data")].toObject();

    // 收集 formulaId -> 结果条目（兼容 data.results 对象字典 / 数组 / items / list / 单条）
    QMap<double, QJsonObject> byId;
    auto addEntry = [&](const QJsonObject &e, double fallbackId) {
        if (e.isEmpty()) return;
        double id = fallbackId;
        if (e[QStringLiteral("formulaId")].isDouble()) id = e[QStringLiteral("formulaId")].toDouble();
        if (id > 0) byId.insert(id, e);
    };
    const auto results = data[QStringLiteral("results")];
    if (results.isObject()) {
        const QJsonObject ro = results.toObject();
        for (auto it = ro.begin(); it != ro.end(); ++it)
            addEntry(it.value().toObject(), it.key().toDouble());
    } else if (results.isArray()) {
        for (const auto &v : results.toArray()) { const QJsonObject e = v.toObject(); addEntry(e, e[QStringLiteral("formulaId")].toDouble()); }
    } else if (data[QStringLiteral("items")].isArray()) {
        for (const auto &v : data[QStringLiteral("items")].toArray()) { const QJsonObject e = v.toObject(); addEntry(e, e[QStringLiteral("formulaId")].toDouble()); }
    } else if (data[QStringLiteral("list")].isArray()) {
        for (const auto &v : data[QStringLiteral("list")].toArray()) { const QJsonObject e = v.toObject(); addEntry(e, e[QStringLiteral("formulaId")].toDouble()); }
    } else if (data.contains(QStringLiteral("scoreAc")) || data.contains(QStringLiteral("components"))
               || data[QStringLiteral("formulaId")].isDouble()) {
        addEntry(data, batch.formulaIds.value(0, -1));
    }

    for (int i = 0; i < batch.formulaIds.size(); ++i) {
        const double id = batch.formulaIds[i];
        const int r = batch.rows.value(i, -1);
        if (r < 0) continue;
        if (byId.contains(id)) {
            const QJsonObject &e = byId.value(id);
            patchRowCorrected(r, e);
        } else markRowCorrectFailed(r);
    }
}

void AutoScPage::processNextAutoCorrectBatch()
{
    if (m_autoGeneration != m_searchGeneration) return;               // 已被新搜索取代
    if (m_autoBatchIdx < 0 || m_autoBatchIdx >= m_autoBatches.size()) return;

    const AutoCorrectBatch batch = m_autoBatches[m_autoBatchIdx];
    const int gen = m_autoGeneration;

    QJsonObject body;
    body[QStringLiteral("measurementId")] = m_currentMeasurementId.toLongLong();
    QJsonArray ids;
    for (double id : batch.formulaIds) ids.append(id);
    body[QStringLiteral("formulaIds")] = ids;
    body[QStringLiteral("correctionMethod")] = QStringLiteral("CLASSIC");
    if (m_autoHasSdkCalset) body[QStringLiteral("sdkCalsetId")] = m_autoSdkCalsetId;
    body[QStringLiteral("industryType")] = batch.industryType;

    ScApi::instance().correctColor(body,
        [this, gen, batch](const QJsonObject &res) {
            if (gen != m_searchGeneration) return;
            applyAutoCorrectResult(batch, res);
            m_autoBatchIdx++;
            processNextAutoCorrectBatch();
        },
        [this, gen, batch](const QString &, int) {
            if (gen != m_searchGeneration) return;
            for (int r : batch.rows) markRowCorrectFailed(r);
            m_autoBatchIdx++;
            processNextAutoCorrectBatch();
        });
}

void AutoScPage::startAutoCorrect(int generation)
{
    m_autoBatches.clear();
    m_autoBatchIdx = 0;
    m_autoGeneration = generation;
    if (!m_resultTable || m_currentMeasurementId.isEmpty()) return;

    // 捕获发起时的梯度配置
    m_autoHasSdkCalset = false;
    const int calsetIdx = m_calsetCombo ? m_calsetCombo->currentIndex() : 0;
    if (calsetIdx > 0 && calsetIdx - 1 < m_calsets.size()) {
        m_autoSdkCalsetId = m_calsets[calsetIdx - 1].id.toLongLong();
        m_autoHasSdkCalset = true;
    }

    // 按 industryType 分组收集满足条件的行（scoreO 有限且 >=1、未 corrected、有有效 formulaId）
    QMap<QString, QList<double>> gIds;
    QMap<QString, QList<int>>    gRows;
    for (int r = 0; r < m_resultTable->rowCount(); ++r) {
        auto *item0 = m_resultTable->item(r, 0);
        if (!item0) continue;
        const QJsonObject p = QJsonDocument::fromJson(item0->data(Qt::UserRole).toByteArray()).object();
        const auto scoreVal = p[QStringLiteral("scoreO")];
        if (!scoreVal.isDouble()) continue;
        if (scoreVal.toDouble() < kAutoCorrectMinScoreO) continue;
        if (p[QStringLiteral("corrected")].toBool()) continue;
        const double fid = p[QStringLiteral("formulaId")].toVariant().toDouble();
        if (fid <= 0) continue;
        const QString type = coatTypeCanonical(p[QStringLiteral("coatType")].toString()) == QStringLiteral("素色")
            ? QStringLiteral("PAINT") : QStringLiteral("EFFECTS");
        gIds[type].append(fid);
        gRows[type].append(r);
    }

    // 每组按 batchSize 切成顺序批次
    for (auto it = gIds.constBegin(); it != gIds.constEnd(); ++it) {
        const QList<double> &ids = it.value();
        const QList<int> &rows = gRows[it.key()];
        for (int start = 0; start < ids.size(); start += kAutoCorrectBatchSize) {
            AutoCorrectBatch b;
            b.industryType = it.key();
            for (int k = start; k < ids.size() && k < start + kAutoCorrectBatchSize; ++k) {
                b.formulaIds.append(ids[k]);
                b.rows.append(rows[k]);
                markRowCorrectPending(rows[k]);
            }
            m_autoBatches.append(b);
        }
    }

    if (m_autoBatches.isEmpty()) return;
    processNextAutoCorrectBatch();
}

void AutoScPage::onEditFormula()
{
    startEditFormula();
}

void AutoScPage::onMeasure()
{
    // 不直接测量：是否测量、测几次、用什么数据模式都由测量弹窗决定
    showMeasureDialog();
}

// ─── 测量弹窗 ───────────────────────────────────────────────────

void AutoScPage::showMeasureDialog()
{
    if (m_measureDlg) {
        m_measureDlg->raise();
        m_measureDlg->activateWindow();
        return;
    }

    // 模式无关窗口：测量在后台异步进行，弹窗需要实时显示进度与设备状态
    auto *dlg = new QWidget(this, Qt::Dialog);
    dlg->setWindowTitle(T("autosc_measure"));
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    auto *v = new QVBoxLayout(dlg);
    v->setContentsMargins(16, 16, 16, 16);
    v->setSpacing(12);

    const auto makeRowLabel = [](const QString &text) {
        auto *l = new QLabel(text);
        l->setStyleSheet(QStringLiteral("font-size: 12px; color: #606266;"));
        return l;
    };

    // 设备连接状态
    auto *statusRow = new QHBoxLayout;
    statusRow->setSpacing(10);
    statusRow->addWidget(makeRowLabel(T("autosc_measure_device_status")));
    m_measureDlgStatus = new QLabel(dlg);
    m_measureDlgStatus->setStyleSheet(QStringLiteral(
        "font-size: 11px; padding: 4px 8px; border-radius: 4px;"));
    statusRow->addWidget(m_measureDlgStatus);
    statusRow->addStretch();
    v->addLayout(statusRow);

    // 测量次数（连测取平均）
    auto *countRow = new QHBoxLayout;
    countRow->setSpacing(10);
    countRow->addWidget(makeRowLabel(T("autosc_measure_count_label")));
    auto *countSpin = new QSpinBox(dlg);
    countSpin->setRange(1, 10);
    countSpin->setValue(QSettings().value(QStringLiteral("autosc/measureCount"), 3).toInt());
    countSpin->setFixedWidth(80);
    countRow->addWidget(countSpin);
    countRow->addStretch();
    v->addLayout(countRow);

    // 数据模式（SDX4 走多角度光谱数据）
    auto *modeRow = new QHBoxLayout;
    modeRow->setSpacing(10);
    modeRow->addWidget(makeRowLabel(T("autosc_measure_data_mode")));
    auto *modeCombo = new QComboBox(dlg);
    modeCombo->addItem(QStringLiteral("ma9x"));
    modeCombo->setFixedWidth(140);
    modeRow->addWidget(modeCombo);
    modeRow->addStretch();
    v->addLayout(modeRow);

    m_measureDlgHint = new QLabel(T("autosc_measure_dlg_hint"), dlg);
    m_measureDlgHint->setWordWrap(true);
    m_measureDlgHint->setStyleSheet(QStringLiteral("font-size: 12px; color: #909399;"));
    v->addWidget(m_measureDlgHint);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);
    btnRow->addStretch();
    auto *measureBtn = new QPushButton(T("autosc_measure"), dlg);
    measureBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border: 1px solid #67c23a; border-radius: 4px; padding: 5px 15px; "
        "background: #67c23a; font-size: 12px; color: #fff; }"
        "QPushButton:hover { background: #85ce61; }"
        "QPushButton:disabled { background: #c8cdd4; border-color: #c8cdd4; color: #fff; }"));
    auto *closeBtn = new QPushButton(T("autosc_measure_close"), dlg);
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px 15px; "
        "background: #fff; font-size: 12px; color: #606266; }"
        "QPushButton:hover { border-color: #c0c4cc; }"));
    btnRow->addWidget(measureBtn);
    btnRow->addWidget(closeBtn);
    v->addLayout(btnRow);

    m_measureDlg = dlg;
    m_measureDlgBtn = measureBtn;
    m_measureDlgCount = countSpin;
    m_measureDlgMode = modeCombo;

    connect(countSpin, &QSpinBox::valueChanged, dlg, [](int value) {
        QSettings().setValue(QStringLiteral("autosc/measureCount"), value);
    });
    connect(measureBtn, &QPushButton::clicked, dlg, [this]() { startMeasureFromDialog(); });
    connect(closeBtn, &QPushButton::clicked, dlg, [dlg]() { dlg->close(); });
    connect(dlg, &QObject::destroyed, this, [this]() {
        m_measurePending = false;
        m_measureDlgStatus = nullptr;
        m_measureDlgHint = nullptr;
        m_measureDlgBtn = nullptr;
        m_measureDlgCount = nullptr;
        m_measureDlgMode = nullptr;
    });

    updateSdx4StatusLabel();
    if (auto &dev = XriteDevice::instance(); !dev.isAvailable()) {
        // 驱动不可用时直接说明原因，不必让用户再点一次「测量」
        setMeasureDlgHint(sdx4UnavailableHint(dev), true);
    }
    dlg->adjustSize();
    dlg->move(mapToGlobal(rect().center()) - QPoint(dlg->width() / 2, dlg->height() / 2));
    dlg->show();
}

void AutoScPage::startMeasureFromDialog()
{
    auto &dev = XriteDevice::instance();
    if (!dev.isAvailable()) {
        setMeasureDlgHint(sdx4UnavailableHint(dev), true);
        return;
    }

    if (!dev.isConnected()) {
        m_measurePending = true;
        setMeasureDlgBusy(true);
        setMeasureDlgHint(T("autosc_connecting"));
        qInfo() << "弹窗触发连接设备...";
        dev.connectDevice();
        return;
    }

    runMeasureFromDialog();
}

void AutoScPage::runMeasureFromDialog()
{
    m_measurePending = false;
    auto &dev = XriteDevice::instance();

    const int count = m_measureDlgCount ? m_measureDlgCount->value() : 1;

    qInfo() << "弹窗触发测量：连测" << count << "次取平均（数据模式 ma9x）";

    setMeasureDlgBusy(true);
    setMeasureDlgHint(T("autosc_measuring"));
    if (m_measureBtn) {
        m_measureBtn->setEnabled(false);
        m_measureBtn->setText(T("autosc_measuring"));
    }
    dev.measureAveraged(count);
}

void AutoScPage::setMeasureDlgBusy(bool busy)
{
    if (!m_measureDlg) return;
    if (m_measureDlgBtn) m_measureDlgBtn->setEnabled(busy);
    if (m_measureDlgCount) m_measureDlgCount->setEnabled(busy);
    if (m_measureDlgMode) m_measureDlgMode->setEnabled(busy);
}

void AutoScPage::setMeasureDlgHint(const QString &text, bool error)
{
    if (!m_measureDlgHint) return;
    m_measureDlgHint->setStyleSheet(QStringLiteral("font-size: 12px; color: %1;")
        .arg(error ? QStringLiteral("#f56c6c") : QStringLiteral("#909399")));
    m_measureDlgHint->setText(text);
}

// ─── SDX4 设备信号槽实现 ────────────────────────────────────────

void AutoScPage::onSdx4Connected()
{
    qInfo() << "SDX4 设备已连接";
    m_measureBtn->setEnabled(true);
    m_measureBtn->setText(T("autosc_measure"));
    updateSdx4StatusLabel();

    // 由弹窗发起的连接：连上后按弹窗设定开始测量
    if (m_measurePending) {
        runMeasureFromDialog();
        return;
    }
}

void AutoScPage::onSdx4Disconnected()
{
    qInfo() << "SDX4 设备已断开";
    m_measureBtn->setEnabled(true);
    m_measureBtn->setText(T("autosc_measure"));
    updateSdx4StatusLabel();
}

void AutoScPage::onSdx4MeasurementReady(const QList<XriteDevice::SpectralSet> &spectra)
{
    m_measureCount++;
    qInfo() << "SDX4 测量完成，第" << m_measureCount << "次，光谱集数量:" << spectra.size();
    m_measureBtn->setEnabled(true);
    m_measureBtn->setText(T("autosc_measure"));

    int totalPoints = 0;
    for (const auto &set : spectra) {
        totalPoints += set.values.size();
    }

    if (m_measureDlg) {
        // 结果就写在弹窗里，不再叠加一个详情窗
        setMeasureDlgBusy(false);
        setMeasureDlgHint(T("autosc_measure_result").arg(spectra.size()).arg(totalPoints));
    } else {
        // 弹窗已关闭（例如测量途中被关掉）：保留原来的详情提示
        QString detail;
        detail += T("autosc_measure_times").arg(m_measureCount);
        detail += QStringLiteral("─────────────────\n");
        detail += T("autosc_measure_angles").arg(spectra.size());
        for (const auto &set : spectra) {
            detail += T("autosc_measure_angle_line")
                .arg(set.geometryCode)
                .arg(set.startWavelength)
                .arg(set.endWavelength)
                .arg(set.step)
                .arg(set.values.size());
        }
        detail += QStringLiteral("─────────────────\n");
        detail += T("autosc_measure_points").arg(totalPoints);

        // 显示第一个角度的前几个反射率值作为示例
        if (!spectra.isEmpty() && !spectra.first().values.isEmpty()) {
            const auto &first = spectra.first();
            detail += T("autosc_measure_example").arg(first.geometryCode);
            int showCount = qMin(5, first.values.size());
            for (int i = 0; i < showCount; ++i) {
                int wl = first.startWavelength + i * first.step;
                detail += QStringLiteral("  %1nm → %2%\n")
                    .arg(wl)
                    .arg(first.values[i], 0, 'f', 2);
            }
            if (first.values.size() > showCount) {
                detail += QStringLiteral("  ...\n");
            }
        }
        QMessageBox::information(this, T("autosc_measure_done"), detail);
    }

    // 上传测量数据到后端
    uploadAndUseMeasurement(spectra);
}

void AutoScPage::onSdx4MeasurementProgress(int done, int total)
{
    if (m_measureDlg) {
        setMeasureDlgHint(T("autosc_measure_progress").arg(done).arg(total));
    }
}

void AutoScPage::onSdx4Error(const QString &msg)
{
    qWarning() << "SDX4 错误:" << msg;
    m_measurePending = false;
    m_measureBtn->setEnabled(true);
    m_measureBtn->setText(T("autosc_measure"));

    if (m_measureDlg) {
        setMeasureDlgBusy(false);
        setMeasureDlgHint(msg, true);
        return;
    }
    QMessageBox::warning(this, T("autosc_sdx4_error"), msg);
}

void AutoScPage::uploadAndUseMeasurement(const QList<XriteDevice::SpectralSet> &spectra)
{
    auto &dev = XriteDevice::instance();

    // 配方面板的测量是「试喷色」：后端按 uuid 把它复制进配方光谱/配方纹理，
    // 不会进入左侧作业目标光谱下拉（那里只列 TARGET）
    QJsonObject body;

    QString measurementUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    measurementUuid.remove(QLatin1Char('-'));
    body[QStringLiteral("measurementUuid")] = measurementUuid;
    body[QStringLiteral("measurementType")] = QStringLiteral("TRIAL");
    body[QStringLiteral("deviceType")] = QStringLiteral("MAC");
    body[QStringLiteral("spectralValuesAsPercent")] = true;

    QString deviceId = dev.serialNumber();
    if (deviceId.isEmpty()) deviceId = QStringLiteral("sdx4-unknown");
    body[QStringLiteral("deviceId")] = deviceId;

    QJsonObject spectralData;
    for (const auto &set : spectra) {
        QJsonObject specObj;
        specObj[QStringLiteral("startWavelength")] = set.startWavelength;
        specObj[QStringLiteral("endWavelength")] = set.endWavelength;
        specObj[QStringLiteral("step")] = set.step;

        QJsonArray valuesArr;
        for (double v : set.values) {
            valuesArr.append(v);
        }
        specObj[QStringLiteral("spectralValues")] = valuesArr;
        spectralData[set.geometryCode] = specObj;
    }
    body[QStringLiteral("spectralData")] = spectralData;

    // 纹理指标名 → 后端字段，与网桥 WebSyncTextureMetricsMapper 保持一致
    auto metricOf = [](const QMap<QString, double> &m,
                       std::initializer_list<const char *> names) -> double {
        for (const char *n : names) {
            const QString key = QLatin1String(n);
            if (m.contains(key)) return m.value(key);
        }
        return qQNaN();
    };

    QJsonArray textureAngles;
    for (const auto &t : dev.lastTextures()) {
        QJsonObject obj;
        obj[QStringLiteral("angle")] = t.angle;
        obj[QStringLiteral("version")] = t.version;

        const auto put = [&obj, &metricOf, &t](const char *field,
                                               std::initializer_list<const char *> names) {
            const double v = metricOf(t.metrics, names);
            if (!qIsNaN(v)) obj[QString::fromLatin1(field)] = v;
        };
        put("coarseness",                    { "C", "DC" });
        put("sparkleGrade",                  { "SG" });
        put("sparkleGradeDensity",           { "SD" });
        put("sparkleGradeIntensity",         { "SC" });
        put("sparkleGradeColorVariation",    { "CO" });
        put("colorSparkleVariance",          { "CV" });

        if (obj.size() > 2) textureAngles.append(obj);
    }
    if (!textureAngles.isEmpty()) {
        body[QStringLiteral("textureAngles")] = textureAngles;
    }

    // 色块相机图（JPEG，按角度一张）
    QJsonArray previews;
    for (const auto &p : dev.lastPreviewImages()) {
        if (p.data.isEmpty()) continue;
        QJsonObject obj;
        obj[QStringLiteral("geometryCode")] = p.geometryCode;
        obj[QStringLiteral("format")] = p.format;
        obj[QStringLiteral("data")] = QString::fromLatin1(p.data.toBase64());
        previews.append(obj);
    }
    if (!previews.isEmpty()) {
        body[QStringLiteral("previewImages")] = previews;
    }

    m_lastMeasurementHasTexture = !textureAngles.isEmpty();


    ScApi::instance().uploadMeasurement(body,
        [this, measurementUuid](const QJsonObject &res) {
            // 标准入口返回 DeviceMeasurement，data.id 为数字主键
            auto data = res[QStringLiteral("data")].toObject();
            QString measurementId = QString::number(data[QStringLiteral("id")].toVariant().toLongLong());

            if (measurementId.isEmpty() || measurementId == QStringLiteral("0")) {
                measurementId = data[QStringLiteral("measurementUuid")].toString();
            }

            m_lastMeasurementId = measurementId;
            m_lastMeasurementUuid = measurementUuid;  // 保存配方时按此 uuid 复制光谱/纹理

            // 启用保存按钮（有测量数据后才可保存）
            m_saveAllowed = true;
            if (m_saveBtn) m_saveBtn->setEnabled(true);

            // 注意：不覆盖 m_currentMeasurementId —— 那是左侧作业目标色，
            // 修色请求以它为基准；试喷色只随保存配方一起进配方光谱。
            QMessageBox::information(this, T("autosc_measure_success"),
                T("autosc_measure_uploaded").arg(measurementId));
        },
        [this](const QString &msg, int code) {
            qWarning() << "测量数据上传失败:" << msg << "code:" << code;
            QMessageBox::warning(this, T("autosc_upload_failed"),
                T("autosc_upload_failed_detail").arg(msg));
        });
}

void AutoScPage::updateSdx4StatusLabel()
{
    if (!m_deviceStatusLabel && !m_measureDlgStatus) return;

    auto &dev = XriteDevice::instance();
    QString text;
    QString color;
    QString bg;
    QString detail;   // 驱动加载失败的具体原因（悬浮提示）
    if (!dev.isAvailable()) {
        if (dev.driverStatus() == XriteDevice::DriverStatus::LoadFailed) {
            text = T("autosc_sdx4_status_none");
            detail = T("autosc_sdx4_load_failed").arg(dev.driverError());
        } else {
            text = T("autosc_sdx4_status_missing");
        }
        color = QStringLiteral("#f56c6c");
        bg = QStringLiteral("#fef0f0");
    } else if (dev.isConnected()) {
        text = T("autosc_sdx4_status_ok").arg(dev.serialNumber());
        color = QStringLiteral("#67c23a");
        bg = QStringLiteral("#f0f9eb");
    } else {
        text = T("autosc_sdx4_status_off");
        color = QStringLiteral("#909399");
        bg = QStringLiteral("#f5f7fa");
    }

    const QString style = QStringLiteral(
        "font-size: 11px; padding: 4px 8px; border-radius: 4px; background: %1; color: %2;")
        .arg(bg, color);

    if (m_deviceStatusLabel) {
        m_deviceStatusLabel->setText(text);
        m_deviceStatusLabel->setStyleSheet(style);
        m_deviceStatusLabel->setToolTip(detail);
        scheduleSidebarSync();   // 状态文案换行会改变左栏内容高度
    }
    if (m_measureDlgStatus) {
        m_measureDlgStatus->setText(text);
        m_measureDlgStatus->setStyleSheet(style);
        m_measureDlgStatus->setToolTip(detail);
    }
}

void AutoScPage::onSaveFormula()
{
    // 检查是否有测量 UUID
    if (m_lastMeasurementUuid.isEmpty()) {
        QMessageBox::information(this, T("info"),
            T("autosc_measure_before_save"));
        return;
    }

    // 后端把测量复制进配方光谱时要求已有纹理，缺纹理直接说明，不必等后端报错
    if (!m_lastMeasurementHasTexture) {
        QMessageBox::warning(this, T("autosc_save_failed"), T("autosc_save_no_texture"));
        return;
    }

    // 检查是否有配方明细
    QJsonArray details = buildSaveDetails();
    if (details.isEmpty()) {
        QMessageBox::information(this, T("info"),
            T("autosc_no_save_data"));
        return;
    }

    showSaveFormulaDialog();
}

void AutoScPage::showSaveFormulaDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(T("sc_save_recipe"));
    dialog.setMinimumWidth(460);

    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setSpacing(10);

    // 配方名称（必填，默认 年月日_三位随机数）
    auto *nameEdit = new QLineEdit;
    const QString defaultName = QDate::currentDate().toString(QStringLiteral("yyyyMMdd_"))
        + QString::number(QRandomGenerator::global()->bounded(1000)).rightJustified(3, QLatin1Char('0'));
    nameEdit->setText(defaultName);
    nameEdit->setMaxLength(12);
    form->addRow(T("autosc_dlg_formula_name"), nameEdit);

    // 颜色类型（必填）：选了 2K 梯度数据时默认 2K
    auto *colorTypeCombo = new QComboBox;
    colorTypeCombo->addItems({ QStringLiteral("1K"), QStringLiteral("2K") });
    {
        const int calsetIdx = m_calsetCombo ? m_calsetCombo->currentIndex() : 0;
        const bool is2K = calsetIdx > 0 && calsetIdx - 1 < m_calsets.size()
            && m_calsets[calsetIdx - 1].alias == QStringLiteral("2K");
        colorTypeCombo->setCurrentIndex(is2K ? 1 : 0);
    }
    form->addRow(T("autosc_dlg_color_type"), colorTypeCombo);

    // 品牌 / 车系（显示跟随语言，提交固定中文，英文名一并带上）
    auto *brandCombo = new QComboBox;
    brandCombo->addItem(T("autosc_dlg_select"), QString());
    for (const auto &b : m_brands) {
        brandCombo->addItem(Tn(b.name, b.nameEn), b.id);
        brandCombo->setItemData(brandCombo->count() - 1, b.name, Qt::UserRole + 1);
        brandCombo->setItemData(brandCombo->count() - 1, b.nameEn, Qt::UserRole + 2);
    }
    form->addRow(T("autosc_dlg_brand"), brandCombo);

    auto *seriesCombo = new QComboBox;
    seriesCombo->addItem(T("autosc_dlg_select"), QString());
    seriesCombo->setEnabled(false);
    form->addRow(T("autosc_dlg_series"), seriesCombo);

    // 车系下拉内容：命中缓存直接填充，否则异步取（弹窗可能已关闭，用 QPointer 兜住）
    QPointer<QComboBox> seriesGuard = seriesCombo;
    auto fillSeries = [this, seriesGuard](const QString &brandId, const QString &keepSeriesId) {
        if (!seriesGuard) return;
        seriesGuard->clear();
        seriesGuard->addItem(T("autosc_dlg_select"), QString());
        if (brandId.isEmpty()) {
            seriesGuard->setEnabled(false);
            return;
        }

        const auto appendList = [seriesGuard, keepSeriesId](const QList<SeriesItem> &list) {
            for (const auto &s : list) {
                seriesGuard->addItem(Tn(s.name, s.nameEn), s.id);
                seriesGuard->setItemData(seriesGuard->count() - 1, s.name, Qt::UserRole + 1);
                seriesGuard->setItemData(seriesGuard->count() - 1, s.nameEn, Qt::UserRole + 2);
            }
            seriesGuard->setEnabled(true);
            if (!keepSeriesId.isEmpty()) {
                const int idx = seriesGuard->findData(keepSeriesId);
                if (idx >= 0) seriesGuard->setCurrentIndex(idx);
            }
        };

        if (m_seriesCache.contains(brandId)) {
            appendList(m_seriesCache.value(brandId));
            return;
        }
        FormulaApi::instance().getSeriesByBrand(brandId,
            [seriesGuard, brandId, appendList](const QJsonObject &res) {
                if (!seriesGuard) return;
                QList<SeriesItem> list;
                QJsonArray rows = res[QStringLiteral("rows")].toArray();
                if (rows.isEmpty()) rows = res[QStringLiteral("data")].toArray();
                for (const auto &v : rows) {
                    auto obj = v.toObject();
                    SeriesItem item;
                    item.id = QString::number(obj[QStringLiteral("id")].toInt());
                    item.brandId = brandId;
                    item.name = obj[QStringLiteral("seriesName")].toString();
                    item.nameEn = obj[QStringLiteral("seriesNameEn")].toString();
                    list.append(item);
                }
                appendList(list);
            },
            [seriesGuard](const QString &msg, int) {
                qWarning() << "保存弹窗加载车系失败:" << msg;
                if (seriesGuard) seriesGuard->setEnabled(true);
            });
    };

    // 年份 / 颜色名称 / 颜色英文名
    auto *yearEdit = new QLineEdit;
    yearEdit->setValidator(new QIntValidator(1900, 2999, yearEdit));
    yearEdit->setPlaceholderText(T("autosc_dlg_year_ph"));
    form->addRow(T("autosc_dlg_year"), yearEdit);

    auto *colorNameEdit = new QLineEdit;
    colorNameEdit->setMaxLength(64);
    form->addRow(T("autosc_dlg_color_name"), colorNameEdit);

    auto *colorNameEnEdit = new QLineEdit;
    colorNameEnEdit->setMaxLength(64);
    form->addRow(T("autosc_dlg_color_name_en"), colorNameEnEdit);

    // 颜色偏向 / 涂层类型：显示文字走翻译，提交给后端的值保持中文（userData）
    auto *colorBiasCombo = new QComboBox;
    colorBiasCombo->addItem(QString(), QString());
    colorBiasCombo->addItem(T("bias_standard"), QStringLiteral("标准"));
    colorBiasCombo->addItem(T("bias_darker"), QStringLiteral("偏深"));
    colorBiasCombo->addItem(T("bias_lighter"), QStringLiteral("偏浅"));
    colorBiasCombo->addItem(T("autosc_bias_green"), QStringLiteral("偏绿"));
    colorBiasCombo->addItem(T("autosc_bias_red"), QStringLiteral("偏红"));
    colorBiasCombo->addItem(T("autosc_bias_blue"), QStringLiteral("偏蓝"));
    colorBiasCombo->addItem(T("autosc_bias_yellow"), QStringLiteral("偏黄"));
    colorBiasCombo->addItem(T("bias_variant"), QStringLiteral("差异色"));
    form->addRow(T("autosc_dlg_bias"), colorBiasCombo);

    auto *coatTypeCombo = new QComboBox;
    coatTypeCombo->addItem(QString(), QString());
    coatTypeCombo->addItem(T("autosc_coat_solid"), QStringLiteral("素色"));
    coatTypeCombo->addItem(T("autosc_coat_silver"), QStringLiteral("银粉"));
    coatTypeCombo->addItem(T("autosc_coat_pearl"), QStringLiteral("珍珠"));
    coatTypeCombo->addItem(T("autosc_coat_mixed"), QStringLiteral("混合色"));
    coatTypeCombo->addItem(T("autosc_coat_three_pearl"), QStringLiteral("三工序白珍珠"));
    coatTypeCombo->addItem(T("autosc_coat_candy"), QStringLiteral("糖果色"));
    form->addRow(T("autosc_dlg_coat_type"), coatTypeCombo);

    // 三工序
    auto *threeStepCheck = new QCheckBox(T("autosc_dlg_three_step"));
    form->addRow(QString(), threeStepCheck);

    layout->addLayout(form);

    // 按钮
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto *cancelBtn = new QPushButton(T("cancel"));
    auto *okBtn = new QPushButton(T("confirm"));
    okBtn->setDefault(true);
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(okBtn);
    layout->addLayout(btnLayout);

    // 预填充：搜色面板的筛选优先，其次取当前配方行（含嵌套 formula 字段）
    const QJsonObject row = m_currentRowObj;
    const QJsonObject formulaObj = row[QStringLiteral("formula")].toObject();
    const auto pick = [&row, &formulaObj](const char *key) {
        const QString k = QLatin1String(key);
        QString v = row[k].toVariant().toString().trimmed();
        if (v.isEmpty()) v = formulaObj[k].toVariant().toString().trimmed();
        return v;
    };
    const auto pickBool = [&row, &formulaObj](const char *key) {
        const QString k = QLatin1String(key);
        if (row.contains(k)) return row[k].toVariant().toBool();
        return formulaObj[k].toVariant().toBool();
    };

    QString brandId = m_brandCombo ? m_brandCombo->currentData().toString() : QString();
    if (brandId.isEmpty()) brandId = pick("brandId");
    if (!brandId.isEmpty()) {
        const int idx = brandCombo->findData(brandId);
        if (idx >= 0) brandCombo->setCurrentIndex(idx);
    }
    fillSeries(brandCombo->currentData().toString(), pick("seriesId"));

    const QString year = pick("year");
    if (!year.isEmpty()) yearEdit->setText(year);
    colorNameEdit->setText(pick("colorName"));
    colorNameEnEdit->setText(pick("colorNameEn"));
    const QString coatType = coatTypeCanonical(pick("coatType"));
    if (!coatType.isEmpty()) {
        const int idx = coatTypeCombo->findData(coatType);
        if (idx >= 0) coatTypeCombo->setCurrentIndex(idx);
    }
    const QString colorBias = pick("colorBias");
    if (!colorBias.isEmpty()) {
        const int idx = colorBiasCombo->findData(colorBias);
        if (idx >= 0) colorBiasCombo->setCurrentIndex(idx);
    }
    threeStepCheck->setChecked(pickBool("threeStep"));

    // 换品牌后重新取车系（预填充完成之后再连接，避免重复拉取）
    connect(brandCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            &dialog, [seriesCombo, brandCombo, fillSeries](int index) {
        fillSeries(brandCombo->itemData(index).toString(), QString());
    });

    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, [&]() {
        // 验证必填字段
        QString formulaName = nameEdit->text().trimmed();
        if (formulaName.isEmpty()) {
            QMessageBox::warning(&dialog, T("error"), T("autosc_dlg_name_empty"));
            return;
        }
        if (formulaName.length() > 12) {
            QMessageBox::warning(&dialog, T("error"), T("autosc_dlg_name_too_long"));
            return;
        }

        // 构建提交数据（名称固定提交中文，界面显示的是英文名也不影响后端数据）
        QJsonObject formData;
        formData[QStringLiteral("formulaName")] = formulaName;
        formData[QStringLiteral("colorType")] = colorTypeCombo->currentText();

        const QString pickedBrandId = brandCombo->currentData().toString();
        if (!pickedBrandId.isEmpty()) {
            formData[QStringLiteral("brandId")] = pickedBrandId.toLongLong();
            formData[QStringLiteral("brandName")] =
                brandCombo->itemData(brandCombo->currentIndex(), Qt::UserRole + 1).toString();
            formData[QStringLiteral("brandNameEn")] =
                brandCombo->itemData(brandCombo->currentIndex(), Qt::UserRole + 2).toString();
        }

        const QString pickedSeriesId = seriesCombo->currentData().toString();
        if (!pickedSeriesId.isEmpty()) {
            formData[QStringLiteral("seriesId")] = pickedSeriesId.toLongLong();
            formData[QStringLiteral("seriesName")] =
                seriesCombo->itemData(seriesCombo->currentIndex(), Qt::UserRole + 1).toString();
            formData[QStringLiteral("seriesNameEn")] =
                seriesCombo->itemData(seriesCombo->currentIndex(), Qt::UserRole + 2).toString();
        }

        const QString yearText = yearEdit->text().trimmed();
        if (!yearText.isEmpty()) {
            formData[QStringLiteral("year")] = yearText.toInt();
        }

        const QString colorName = colorNameEdit->text().trimmed();
        if (!colorName.isEmpty()) {
            formData[QStringLiteral("colorName")] = colorName;
        }
        const QString colorNameEn = colorNameEnEdit->text().trimmed();
        if (!colorNameEn.isEmpty()) {
            formData[QStringLiteral("colorNameEn")] = colorNameEn;
        }

        const QString bias = colorBiasCombo->currentData().toString();
        if (!bias.isEmpty()) {
            formData[QStringLiteral("colorBias")] = bias;
        }
        const QString coat = coatTypeCombo->currentData().toString();
        if (!coat.isEmpty()) {
            formData[QStringLiteral("coatType")] = coat;
        }

        formData[QStringLiteral("threeStep")] = threeStepCheck->isChecked();

        dialog.accept();
        submitSaveFormula(formData);
    });

    dialog.exec();
}

void AutoScPage::submitSaveFormula(const QJsonObject &form)
{
    // 构建请求体
    QJsonObject body = form;
    body[QStringLiteral("uuid")] = m_lastMeasurementUuid;

    // 添加色母明细
    QJsonArray details = buildSaveDetails();
    body[QStringLiteral("details")] = details;


    FormulaApi::instance().uploadUserFormula(body,
        [this](const QJsonObject &res) {
            qInfo() << "配方保存成功";
            auto data = res[QStringLiteral("data")].toObject();
            // 后端 UserFormulaUploadResultVo 的字段名是 id
            QString formulaId = QString::number(data[QStringLiteral("id")].toVariant().toLongLong());
            QMessageBox::information(this, T("autosc_save_success"),
                T("autosc_saved_id").arg(formulaId));
        },
        [this](const QString &msg, int code) {
            qWarning() << "配方保存失败:" << msg << "code:" << code;
            QMessageBox::warning(this, T("autosc_save_failed"),
                T("autosc_save_failed_detail").arg(msg));
        });
}

QJsonArray AutoScPage::buildSaveDetails()
{
    // 与前端 normalizeLayerWeightsTo100 一致：每层按克重折算成百分比，合计 100，末行兜底吸收舍入误差
    const auto layerDetails = [](const QJsonArray &rows, int layer) {
        struct Row { QString code; double grams; };
        QJsonArray out;
        QList<Row> items;
        for (const auto &v : rows) {
            const auto row = v.toObject();
            const QString code = row[QStringLiteral("code")].toString().trimmed();
            const double grams = parseWeight(row[QStringLiteral("weight")]);
            if (code.isEmpty() || grams <= 0) continue;
            items.append({ code, grams });
        }

        double total = 0;
        for (const auto &it : items) total += it.grams;
        if (total <= 0) return out;

        const auto round4 = [](double v) { return std::round(v * 10000.0) / 10000.0; };
        double allocated = 0;
        for (int i = 0; i < items.size(); ++i) {
            double weight;
            if (i == items.size() - 1) {
                weight = round4(100.0 - allocated);
            } else {
                weight = round4(items.at(i).grams / total * 100.0);
                allocated += weight;
            }
            if (!(weight > 0)) continue;

            QJsonObject detail;
            detail[QStringLiteral("pigmentCode")] = items.at(i).code;
            detail[QStringLiteral("colorantName")] = items.at(i).code;
            detail[QStringLiteral("weight")] = weight;
            detail[QStringLiteral("concentration")] = weight;
            detail[QStringLiteral("layer")] = layer;
            out.append(detail);
        }
        return out;
    };

    // 保存哪一列就取哪一列的克重（初始 / AC / AC-Edited），与前端 buildSaveDetails 一致
    struct LayerSrc { bool isBase; QJsonArray rows; QJsonArray correctedRows; };
    QList<LayerSrc> layers;
    if (!m_currentBaseRows.isEmpty()) layers.append({true, m_currentBaseRows, m_correctedBaseRows});
    if (!m_currentTopRows.isEmpty()) layers.append({false, m_currentTopRows, m_correctedTopRows});

    QJsonArray saveRows[2];     // [0]=底层/单层，[1]=面层
    const VendorCodes vc{m_correctedVendorData, m_pigmentCodeSystem};
    for (const auto &layer : layers) {
        const QString layerKey = editLayerKey(layer.isBase);
        const double capacity = m_isThreeStep
            ? (layer.isBase ? m_baseCapacity : m_surfaceCapacity)
            : m_canCapacity;
        const LayerPcts pcts = computeLayerPcts(layer.rows, layer.correctedRows,
                                               layerKey, m_acEditedPct, vc);
        const QList<double> grams = gramsFromLayerPcts(pcts, capacity, m_selectedFormulaCol);
        for (int i = 0; i < pcts.rows.size(); ++i) {
            const QString code = rowAcCode(pcts.rows[i].toObject());
            const double g = grams.value(i, -1.0);
            if (code.isEmpty() || g <= 0) continue;
            QJsonObject row;
            row[QStringLiteral("code")] = code;
            row[QStringLiteral("weight")] = g;
            saveRows[layerKey == QLatin1String("surface") ? 1 : 0].append(row);
        }
    }
    if (!saveRows[0].isEmpty() || !saveRows[1].isEmpty()) {
        QJsonArray details = layerDetails(saveRows[0], 1);
        details += layerDetails(saveRows[1], 2);
        return details;
    }

    // 配方行不可用时回退到修色结果
    if (m_hasCorrection && (!m_correctedBaseRows.isEmpty() || !m_correctedTopRows.isEmpty())) {
        QJsonArray details = layerDetails(m_correctedBaseRows, 1);
        details += layerDetails(m_correctedTopRows, 2);
        return details;
    }
    return {};
}

void AutoScPage::onDownloadFormula(const QString &layerKey)
{
    // 确定要导出的色母行
    QJsonArray rows;
    int layerNo = 1;
    if (layerKey == QStringLiteral("base")) {
        rows = m_currentBaseRows;
        layerNo = 1;
    } else if (layerKey == QStringLiteral("surface")) {
        rows = m_currentTopRows;
        layerNo = 2;
    } else {
        // single: 合并底层+面层
        for (const auto &v : m_currentBaseRows) rows.append(v);
        for (const auto &v : m_currentTopRows) rows.append(v);
        layerNo = 1;
    }

    if (rows.isEmpty()) {
        QMessageBox::information(this, T("info"), T("autosc_no_export_formula"));
        return;
    }

    // 构建导出项
    QList<ExportItem> items;
    double totalWeight = 0.0;
    for (const auto &v : rows) {
        const auto obj = v.toObject();
        ExportItem item;
        item.colorant = obj[QStringLiteral("code")].toString();
        if (item.colorant.isEmpty()) item.colorant = obj[QStringLiteral("pigmentCode")].toString();
        item.amount = obj[QStringLiteral("weight")].toDouble();
        if (!item.colorant.isEmpty() && item.amount > 0) {
            items.append(item);
            totalWeight += item.amount;
        }
    }

    if (items.isEmpty()) {
        QMessageBox::information(this, T("info"), T("autosc_no_export_toner"));
        return;
    }

    // 构建元数据
    QJsonObject meta;
    meta[QStringLiteral("sampleId")] = m_currentRowObj[QStringLiteral("sampleId")].toString();
    meta[QStringLiteral("formulaName")] = m_currentRowObj[QStringLiteral("formulaName")].toString();
    meta[QStringLiteral("colorName")] = m_currentRowObj[QStringLiteral("colorName")].toString();
    meta[QStringLiteral("colorCode")] = m_currentRowObj[QStringLiteral("colorCode")].toString();

    // 选择格式（默认 xml）
    FormulaExport::Format fmt = FormulaExport::Format::Xml;
    QString ext = QStringLiteral("xml");
    QString filter = T("autosc_xml_filter");

    // 弹出保存对话框
    QString sampleId = meta[QStringLiteral("sampleId")].toString();
    if (sampleId.isEmpty()) sampleId = QStringLiteral("recipe");
    QString layerTag = (layerKey == QStringLiteral("base")) ? (QStringLiteral("_") + T("export_layer_base"))
                     : (layerKey == QStringLiteral("surface")) ? (QStringLiteral("_") + T("export_layer_surface"))
                     : QString();
    QString defaultName = sampleId + layerTag + QStringLiteral(".") + ext;

    QString filePath = QFileDialog::getSaveFileName(this, T("sc_save_recipe"), defaultName, filter);
    if (filePath.isEmpty()) return;

    // 构建导出选项
    ExportOptions opts;
    opts.layerNo = layerNo;
    opts.meta = meta;
    opts.items = items;
    opts.unit = QStringLiteral("g");
    opts.quantity = totalWeight;
    opts.sampleId = sampleId;
    opts.filename = filePath;

    // 生成并保存
    QString content = FormulaExport::buildContent(fmt, opts);
    if (FormulaExport::saveToFile(filePath, content)) {
        QMessageBox::information(this, T("success"), T("autosc_saved_to") + filePath);
    } else {
        QMessageBox::warning(this, T("autosc_fail"), T("autosc_save_formula_failed"));
    }
}

// ─── 可视化辅助（对标前端 scViewTypes / scAngleFilter）──────────
namespace {

/** 前端 SC_VIZ_SOURCE_ORDER：来源显示顺序 */
const QStringList &scSourceOrder()
{
    static const QStringList order = {
        QStringLiteral("target"), QStringLiteral("initial"), QStringLiteral("recipe"),
        QStringLiteral("ac"), QStringLiteral("acEdited"), QStringLiteral("trial")
    };
    return order;
}

/** 前端 SC_ANGLE_ORDER（MEASUREMENT_GEOMETRY_ORDER） */
const QStringList &scGeometryOrder()
{
    static const QStringList order = {
        QStringLiteral("45as15"), QStringLiteral("45as25"), QStringLiteral("45as45"),
        QStringLiteral("45as75"), QStringLiteral("45as110")
    };
    return order;
}

/** 前端 SC_GEOMETRY_COLORS 的调色板 */
const QList<QColor> &scGeometryPalette()
{
    static const QList<QColor> palette = {
        QColor(QStringLiteral("#3b82f6")), QColor(QStringLiteral("#eab308")),
        QColor(QStringLiteral("#ef4444")), QColor(QStringLiteral("#22c55e")),
        QColor(QStringLiteral("#ec4899"))
    };
    return palette;
}

/** 去掉角度码前缀 r（前端 normalizeScGeometry） */
QString scNormalizeGeometry(const QString &code)
{
    const QString s = code.trimmed();
    return s.startsWith(QLatin1Char('r')) ? s.mid(1) : s;
}

/** 角度 → 颜色：标准角度取固定色，其余按出现序循环调色板 */
QColor scGeometryColor(const QString &geometry, int fallbackIndex)
{
    const QString geo = scNormalizeGeometry(geometry);
    const int idx = scGeometryOrder().indexOf(geo);
    const QList<QColor> &palette = scGeometryPalette();
    const int pick = idx >= 0 ? idx : fallbackIndex;
    return palette[pick % palette.size()];
}

/** 前端 formatScSourceLabel：来源 → 显示名（target 用作业标题，缺省「标样」） */
QString scSourceLabel(const QString &sourceKey, const QString &targetLabel)
{
    const QString sk = sourceKey.trimmed();
    if (sk.compare(QLatin1String("trial"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("TRIAL");
    if (sk.compare(QLatin1String("initial"), Qt::CaseInsensitive) == 0)
        return T("autosc_initial");
    if (sk.compare(QLatin1String("ac"), Qt::CaseInsensitive) == 0
        || sk.compare(QLatin1String("recipe"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("AC");
    if (sk.compare(QLatin1String("acEdited"), Qt::CaseInsensitive) == 0
        || sk.compare(QLatin1String("ac-edited"), Qt::CaseInsensitive) == 0
        || sk.compare(QLatin1String("ac_edited"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("AC-Edited");
    const QString label = targetLabel.trimmed();
    return label.isEmpty() ? T("autosc_standard_sample") : label;
}

/** 前端 formatScSourceDisplayName：来源名 + 角度 */
QString scSourceDisplayName(const QString &sourceKey, const QString &geometry, const QString &targetLabel)
{
    const QString label = scSourceLabel(sourceKey, targetLabel);
    const QString geo = geometry.trimmed();
    return geo.isEmpty() ? label : label + QLatin1Char(' ') + geo;
}

/** 前端 SOURCE_LINE_STYLE：initial 虚线、recipe/ac 点线、acEdited 虚线，其余实线 */
Qt::PenStyle scSourceLineStyle(const QString &sourceKey)
{
    const QString sk = sourceKey.trimmed().toLower();
    if (sk == QLatin1String("initial")) return Qt::DashLine;
    if (sk == QLatin1String("recipe") || sk == QLatin1String("ac")) return Qt::DotLine;
    // 前端 acEdited 与 initial 同为虚线，靠颜色分不开；这里独立成点划线，图例才可读
    if (sk == QLatin1String("acedited") || sk == QLatin1String("ac-edited")
        || sk == QLatin1String("ac_edited")) return Qt::DashDotLine;
    return Qt::SolidLine;
}

/** 排序键：未知来源/角度排在标准项之后 */
int scSourceRank(const QString &sourceKey)
{
    const int idx = scSourceOrder().indexOf(sourceKey);
    return idx < 0 ? scSourceOrder().size() : idx;
}

int scGeometryRank(const QString &geometry)
{
    const int idx = scGeometryOrder().indexOf(scNormalizeGeometry(geometry));
    return idx < 0 ? scGeometryOrder().size() : idx;
}

/** 取行角度（兼容 geometryCode / geometry，去前缀 r） */
QString scRowGeometry(const QJsonObject &obj)
{
    QString geo = obj[QStringLiteral("geometryCode")].toString();
    if (geo.isEmpty()) geo = obj[QStringLiteral("geometry")].toString();
    return scNormalizeGeometry(geo);
}

/** 自动换行的流式布局（对标前端 flex-wrap），用于可视化卡片底部工具条 */
class ScFlowLayout : public QLayout
{
public:
    explicit ScFlowLayout(QWidget *parent = nullptr, int hSpacing = 8, int vSpacing = 8)
        : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing) { }
    ~ScFlowLayout() override { qDeleteAll(m_items); }

    void addItem(QLayoutItem *item) override { m_items.append(item); }
    int count() const override { return static_cast<int>(m_items.size()); }
    QLayoutItem *itemAt(int index) const override
    {
        return (index >= 0 && index < m_items.size()) ? m_items.at(index) : nullptr;
    }
    QLayoutItem *takeAt(int index) override
    {
        return (index >= 0 && index < m_items.size()) ? m_items.takeAt(index) : nullptr;
    }
    Qt::Orientations expandingDirections() const override { return {}; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override { return doLayout(QRect(0, 0, width, 0), true); }
    void setGeometry(const QRect &rect) override
    {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }
    QSize sizeHint() const override { return minimumSize(); }
    QSize minimumSize() const override
    {
        QSize size;
        for (const auto *item : m_items) size = size.expandedTo(item->minimumSize());
        const QMargins m = contentsMargins();
        return size + QSize(m.left() + m.right(), m.top() + m.bottom());
    }

private:
    int doLayout(const QRect &rect, bool testOnly) const
    {
        const QMargins m = contentsMargins();
        const QRect effective = rect.adjusted(m.left(), m.top(), -m.right(), -m.bottom());
        int x = effective.x();
        int y = effective.y();
        int lineHeight = 0;
        for (auto *item : m_items) {
            const QSize hint = item->sizeHint();
            int nextX = x + hint.width() + m_hSpace;
            if (nextX - m_hSpace > effective.right() && lineHeight > 0) {
                x = effective.x();
                y += lineHeight + m_vSpace;
                nextX = x + hint.width() + m_hSpace;
                lineHeight = 0;
            }
            if (!testOnly) item->setGeometry(QRect(QPoint(x, y), hint));
            x = nextX;
            lineHeight = qMax(lineHeight, hint.height());
        }
        return y + lineHeight - rect.y() + m.bottom();
    }

    QList<QLayoutItem *> m_items;
    int m_hSpace;
    int m_vSpace;
};

/**
 * 色块单元格，对标前端 .sc-color-block__cell。
 * 样式表做不了三件事，所以整体自绘：无数据格的 45° 斜纹、列间 1px 分隔线、
 * 整条两端圆角（Qt 的 border-radius 不会裁剪子控件，直接铺色会露出方角）。
 * 图像模式下传 imageUrl + resolver：绘制时才查缓存，预览图晚到只需 update()。
 */
class ScColorBlockCell : public QWidget
{
public:
    using Resolver = std::function<QPixmap(const QString &)>;

    ScColorBlockCell(const QString &hex, bool roundedLeft, bool roundedRight, bool showRightBorder,
                     QWidget *parent = nullptr)
        : ScColorBlockCell(hex, QString(), Resolver(), roundedLeft, roundedRight, showRightBorder, parent)
    {
    }

    ScColorBlockCell(const QString &hex, const QString &imageUrl, const Resolver &resolver,
                     bool roundedLeft, bool roundedRight, bool showRightBorder,
                     QWidget *parent = nullptr)
        : QWidget(parent)
        , m_hex(hex)
        , m_imageUrl(imageUrl)
        , m_resolver(resolver)
        , m_roundedLeft(roundedLeft)
        , m_roundedRight(roundedRight)
        , m_showRightBorder(showRightBorder)
    {
        setMinimumWidth(40);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const QRectF b(0, 0, width(), height());
        const QPixmap pm = m_imageUrl.isEmpty() ? QPixmap() : m_resolver(m_imageUrl);
        QPainter p(this);
        p.setClipPath(clipPath(b));

        if (!pm.isNull()) {
            // 与「作业 | 配方」对比列一致：保持比例填满、居中裁切
            QSize s = pm.size();
            s.scale(b.toRect().size(), Qt::KeepAspectRatioByExpanding);
            const QRect src((pm.width() - s.width()) / 2, (pm.height() - s.height()) / 2,
                            s.width(), s.height());
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            p.drawPixmap(b.toRect(), pm, src);
        } else {
            p.fillRect(b, m_hex.isEmpty() ? QColor(QStringLiteral("#f5f7fa")) : QColor(m_hex));
        }

        if (m_hex.isEmpty() && pm.isNull()) {
            // 前端 repeating-linear-gradient(-45deg, 透明 4px, rgba(148,163,184,.18) 4px)
            p.setRenderHint(QPainter::Antialiasing);
            QPen hatch(QColor(148, 163, 184, 46));
            hatch.setWidth(3);
            p.setPen(hatch);
            for (int x = -height(); x < width() + 8; x += 8) {
                p.drawLine(x, height(), x + height(), 0);
            }
        }

        if (m_showRightBorder) {
            p.setClipping(false);
            p.setRenderHint(QPainter::Antialiasing, false);
            p.setPen(QColor(QStringLiteral("#ebeef5")));
            p.drawLine(width() - 1, 0, width() - 1, height() - 1);
        }
    }

private:
    /** Qt 6.8 的 addRoundedRect 只支持四角同半径，这里按需要只圆外侧两角 */
    QPainterPath clipPath(const QRectF &b) const
    {
        const qreal r = kRadius;
        const qreal lt = m_roundedLeft ? r : 0.0;
        const qreal rt = m_roundedRight ? r : 0.0;
        QPainterPath path;
        path.moveTo(b.left() + lt, b.top());
        path.lineTo(b.right() - rt, b.top());
        if (rt > 0) path.quadTo(b.right(), b.top(), b.right(), b.top() + rt);
        path.lineTo(b.right(), b.bottom() - rt);
        if (rt > 0) path.quadTo(b.right(), b.bottom(), b.right() - rt, b.bottom());
        path.lineTo(b.left() + lt, b.bottom());
        if (lt > 0) path.quadTo(b.left(), b.bottom(), b.left(), b.bottom() - lt);
        path.lineTo(b.left(), b.top() + lt);
        if (lt > 0) path.quadTo(b.left(), b.top(), b.left() + lt, b.top());
        path.closeSubpath();
        return path;
    }

    // 色条圆角 4px 减去它自己的 1px 边框，格子贴边后整体才像前端的 overflow:hidden
    static constexpr qreal kRadius = 3.0;

    QString m_hex;
    QString m_imageUrl;
    Resolver m_resolver;
    bool m_roundedLeft;
    bool m_roundedRight;
    bool m_showRightBorder;
};

} // namespace

// ─── 可视化 ──────────────────────────────────────────────────
void AutoScPage::loadVisualization(const QString &measurementId)
{
    m_vizInfoLabel->setText(T("sc_loading"));
    m_vizInfoLabel->setVisible(true);

    ScApi::instance().getVisualization(measurementId,
        [this](const QJsonObject &res) {
            auto data = res[QStringLiteral("data")].toObject();
            if (data.isEmpty()) data = res;

            // 保存完整数据供各卡片使用
            m_vizData = data;

            // 打印原始 keys 便于调试

            // 兼容前端 pickSpectralList：spectra / spectrophotometry / spectral / spectralData
            QJsonArray spectralData;
            for (const auto &key : {QStringLiteral("spectra"), QStringLiteral("spectrophotometry"),
                                    QStringLiteral("spectral"), QStringLiteral("spectralData")}) {
                spectralData = data[key].toArray();
                if (!spectralData.isEmpty()) break;
            }
            // 兼容前端 colorimetry / colorData / measurements
            QJsonArray colorData = data[QStringLiteral("colorimetry")].toArray();
            if (colorData.isEmpty()) colorData = data[QStringLiteral("colorData")].toArray();
            if (colorData.isEmpty()) colorData = data[QStringLiteral("measurements")].toArray();


            // 更新所有卡片的内容
            for (auto &card : m_vizCards) {
                updateVizCardContent(card);
            }

            int total = colorData.size() + spectralData.size();
            m_vizInfoLabel->setText(T("autosc_viz_loaded").arg(total));
            if (total > 0) m_vizInfoLabel->setVisible(false); // 有数据时隐藏提示
        },
        [this](const QString &msg, int) {
            qWarning() << "可视数据加载失败:" << msg;
            m_vizInfoLabel->setText(T("autosc_viz_load_failed").arg(msg));
        });
}

void AutoScPage::loadFormulaVisualization(const QString &measurementId, int formulaId)
{

    ScApi::instance().getVisualizationWithFormula(measurementId, formulaId,
        [this](const QJsonObject &res) {
            auto data = res[QStringLiteral("data")].toObject();
            if (data.isEmpty()) data = res;

            // 保存配方可视化数据
            m_formulaVizData = data;

            // 调试：打印所有可能的 AC 相关字段
            // 检查 spectra 数组中是否有 ac sourceKey
            auto spectraArr = data[QStringLiteral("spectra")].toArray();
            for (int i = 0; i < spectraArr.size(); ++i) {
                auto item = spectraArr[i].toObject();
            }

            // 兼容前端 pickSpectralList
            QJsonArray spectralData;
            for (const auto &key : {QStringLiteral("spectra"), QStringLiteral("spectrophotometry"),
                                    QStringLiteral("spectral"), QStringLiteral("spectralData")}) {
                spectralData = data[key].toArray();
                if (!spectralData.isEmpty()) break;
            }
            QJsonArray colorData = data[QStringLiteral("colorimetry")].toArray();
            if (colorData.isEmpty()) colorData = data[QStringLiteral("colorData")].toArray();
            if (colorData.isEmpty()) colorData = data[QStringLiteral("measurements")].toArray();


            // 更新所有卡片的内容（合并测量+配方数据）
            for (auto &card : m_vizCards) {
                updateVizCardContent(card);
            }
        },
        [this](const QString &msg, int) {
            qWarning() << "配方可视数据加载失败:" << msg;
        });
}

// ─── 可视化数据合并 ──────────────────────────────────────────

QString AutoScPage::currentJobTitle() const
{
    for (const auto &job : m_jobs) {
        if (job.id == m_currentMeasurementId && !job.title.trimmed().isEmpty())
            return job.title.trimmed();
    }
    return QString();
}

QMap<double, double> AutoScPage::spectralValuesFromItem(const QJsonObject &obj)
{
    QMap<double, double> out;
    // 1. 对象格式：spectral / reflectance / values 直接是 {波长: 反射率}
    for (const auto &key : {QStringLiteral("spectral"), QStringLiteral("reflectance"), QStringLiteral("values")}) {
        const auto val = obj[key];
        if (val.isObject() && !val.toObject().isEmpty()) {
            const auto map = val.toObject();
            for (auto it = map.begin(); it != map.end(); ++it) {
                bool ok = false;
                const double wl = it.key().toDouble(&ok);
                if (ok) out.insert(wl, it.value().toDouble());
            }
            return out;
        }
    }
    // 2. 数组格式：values + wavelengths
    const auto valuesArr = obj[QStringLiteral("values")].toArray();
    if (!valuesArr.isEmpty()) {
        const auto wavelengthsArr = obj[QStringLiteral("wavelengths")].toArray();
        if (wavelengthsArr.size() == valuesArr.size()) {
            for (int i = 0; i < valuesArr.size(); ++i)
                out.insert(wavelengthsArr[i].toDouble(), valuesArr[i].toDouble());
            return out;
        }
        // 3. 数组格式：values + startWavelength + step
        const double start = obj[QStringLiteral("startWavelength")].toDouble(400);
        const double step = obj[QStringLiteral("step")].toDouble(10);
        for (int i = 0; i < valuesArr.size(); ++i)
            out.insert(start + i * step, valuesArr[i].toDouble());
    }
    return out;
}

QJsonArray AutoScPage::buildMergedSpectralData() const
{
    auto extractSpectralArray = [](const QJsonObject &vizData) -> QJsonArray {
        for (const auto &key : {QStringLiteral("spectra"), QStringLiteral("spectrophotometry"),
                                QStringLiteral("spectral"), QStringLiteral("spectralData")}) {
            auto arr = vizData[key].toArray();
            if (!arr.isEmpty()) return arr;
        }
        return QJsonArray{};
    };

    const QJsonArray spectralData = extractSpectralArray(m_vizData);
    const QJsonArray formulaSpectralData = extractSpectralArray(m_formulaVizData);

    // 从当前选中行的 correctDetail 中提取 AC 修色光谱数据
    QJsonArray acSpectralData;
    if (m_currentRowObj[QStringLiteral("corrected")].toBool()) {
        const QJsonObject cd = m_currentRowObj[QStringLiteral("correctDetail")].toObject();
        if (!cd.isEmpty()) {
            auto tryPick = [&cd](const QString &key) -> QJsonArray {
                auto arr = cd[key].toArray();
                if (arr.isEmpty()) {
                    auto sub = cd[QStringLiteral("spectralAc")].toObject()[key].toArray();
                    if (!sub.isEmpty()) arr = sub;
                }
                return arr;
            };
            for (const auto &key : {QStringLiteral("correctedSpectra"), QStringLiteral("spectra"),
                                    QStringLiteral("spectrophotometry"), QStringLiteral("spectralData")}) {
                acSpectralData = tryPick(key);
                if (!acSpectralData.isEmpty()) break;
            }
            QJsonArray normalizedAc;
            for (const auto &v : acSpectralData) {
                QJsonObject o = v.toObject();
                if (o.isEmpty()) continue;
                if (o[QStringLiteral("sourceKey")].toString().isEmpty()) {
                    o[QStringLiteral("sourceKey")] = QStringLiteral("ac");
                    if (o[QStringLiteral("sourceName")].toString().isEmpty())
                        o[QStringLiteral("sourceName")] = QStringLiteral("AC");
                }
                normalizedAc.append(o);
            }
            acSpectralData = normalizedAc;
        }
    }

    // 编辑预测（predictDetail）的光谱归到 acEdited 来源
    QJsonArray editedSpectralData;
    {
        const QJsonObject pd = m_currentRowObj[QStringLiteral("predictDetail")].toObject();
        if (!pd.isEmpty()) {
            for (const auto &key : {QStringLiteral("correctedSpectra"), QStringLiteral("spectra"),
                                    QStringLiteral("spectrophotometry"), QStringLiteral("spectralData")}) {
                const auto arr = pd[key].toArray();
                if (arr.isEmpty()) continue;
                for (const auto &v : arr) {
                    QJsonObject o = v.toObject();
                    if (o.isEmpty()) continue;
                    o[QStringLiteral("sourceKey")] = QStringLiteral("acEdited");
                    o[QStringLiteral("sourceName")] = QStringLiteral("AC-Edited");
                    if (o[QStringLiteral("name")].toString().isEmpty())
                        o[QStringLiteral("name")] = QStringLiteral("AC-Edited");
                    editedSpectralData.append(o);
                }
                break;
            }
        }
    }

    // 合并：标样（measurementViz）→ 配方（formulaViz）→ AC 补位（correctDetail）→ AC-Edited（predictDetail）
    QJsonArray merged;
    for (const auto &item : spectralData) {
        const auto obj = item.toObject();
        const QString sk = obj[QStringLiteral("sourceKey")].toString();
        if (sk.isEmpty() || sk == QStringLiteral("target")) merged.append(item);
    }
    for (const auto &item : formulaSpectralData) merged.append(item);
    bool formulaHasAc = false;
    for (const auto &item : formulaSpectralData) {
        const QString sk = item.toObject()[QStringLiteral("sourceKey")].toString().toLower();
        if (sk == QStringLiteral("ac") || sk == QStringLiteral("recipe")) { formulaHasAc = true; break; }
    }
    if (!formulaHasAc) {
        for (const auto &item : acSpectralData) merged.append(item);
    }
    for (const auto &item : editedSpectralData) merged.append(item);

    // 去重：同一 sourceKey:geometry 只保留第一条
    QJsonArray deduped;
    QSet<QString> seen;
    for (const auto &item : merged) {
        const auto obj = item.toObject();
        if (obj.isEmpty()) continue;
        const QString sk = obj[QStringLiteral("sourceKey")].toString(QStringLiteral("target"));
        const QString key = sk + QLatin1Char(':') + scRowGeometry(obj);
        if (seen.contains(key)) continue;
        seen.insert(key);
        deduped.append(item);
    }

    // 排序：来源顺序 → 角度顺序（前端 SC_VIZ_SOURCE_ORDER + SC_ANGLE_ORDER）
    std::vector<QJsonObject> ordered;
    ordered.reserve(static_cast<size_t>(deduped.size()));
    for (const auto &item : deduped) ordered.push_back(item.toObject());
    std::stable_sort(ordered.begin(), ordered.end(), [](const QJsonObject &a, const QJsonObject &b) {
        const int ra = scSourceRank(a[QStringLiteral("sourceKey")].toString(QStringLiteral("target")));
        const int rb = scSourceRank(b[QStringLiteral("sourceKey")].toString(QStringLiteral("target")));
        if (ra != rb) return ra < rb;
        const int ga = scGeometryRank(scRowGeometry(a));
        const int gb = scGeometryRank(scRowGeometry(b));
        return ga < gb;
    });
    QJsonArray out;
    for (const auto &obj : ordered) out.append(obj);
    return out;
}

// ─── 可视化卡片管理 ──────────────────────────────────────────────

void AutoScPage::addVizCard(const QString &type)
{
    if (!m_vizLayout) return;

    VizCard card;
    // 色块显示模式沿用上次设置（所有色块卡片共用同一个默认值）
    card.colorBlockMode = QSettings().value(QStringLiteral("autosc/colorBlockMode"),
                                             QStringLiteral("render")).toString();
    // 卡片容器
    auto *cardWidget = new QWidget;
    card.widget = cardWidget;
    cardWidget->setStyleSheet(QStringLiteral(
        "QWidget { background: #fff; border: 1px solid #e4e7ed; border-radius: 6px; margin: 4px 0; }"));
    auto *cardLayout = new QVBoxLayout(cardWidget);
    cardLayout->setContentsMargins(8, 8, 8, 8);
    cardLayout->setSpacing(6);

    // 顶部控制栏：类型下拉 + 添加/删除按钮
    auto *controlRow = new QHBoxLayout;
    controlRow->setSpacing(8);

    card.typeCombo = new QComboBox;
    card.typeCombo->addItems({
        T("spectral_data"),
        T("spectral_curve"),
        T("autosc_card_color_data"),
        T("color_block")
    });
    // 设置初始类型
    if (type == QStringLiteral("spectral_data")) card.typeCombo->setCurrentIndex(0);
    else if (type == QStringLiteral("spectral_curve")) card.typeCombo->setCurrentIndex(1);
    else if (type == QStringLiteral("color_data")) card.typeCombo->setCurrentIndex(2);
    else if (type == QStringLiteral("color_block")) card.typeCombo->setCurrentIndex(3);
    card.type = type;
    card.typeCombo->setStyleSheet(QStringLiteral(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 4px 8px; "
        "font-size: 12px; background: #fff; color: #303133; min-width: 100px; }"
        "QComboBox:hover { border-color: #409eff; }"
        "QComboBox QAbstractItemView { border: 1px solid #dcdfe6; background: #fff; "
        "color: #303133; selection-background-color: #e8f0fe; selection-color: #303133; }"));
    controlRow->addWidget(new QLabel(T("autosc_card_type_label")));
    controlRow->addWidget(card.typeCombo);

    // 添加卡片按钮
    auto *addBtn = new QPushButton(QStringLiteral("+"));
    addBtn->setFixedSize(24, 24);
    addBtn->setToolTip(T("autosc_add_card"));
    addBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border: 1px solid #67c23a; border-radius: 4px; background: #67c23a; "
        "color: #fff; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background: #85ce61; }"));
    controlRow->addWidget(addBtn);

    // 删除卡片按钮
    auto *delBtn = new QPushButton(QStringLiteral("×"));
    delBtn->setFixedSize(24, 24);
    delBtn->setToolTip(T("autosc_del_card"));
    delBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border: 1px solid #f56c6c; border-radius: 4px; background: #f56c6c; "
        "color: #fff; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #f78989; }"));
    controlRow->addWidget(delBtn);

    controlRow->addStretch();

    // 右上角设置按钮：目前只有色块卡片有可设项（显示模式），所以仅色块卡片显示
    card.settingsBtn = new QPushButton(T("settings"));
    card.settingsBtn->setCursor(Qt::PointingHandCursor);
    card.settingsBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border: 1px solid #dcdfe6; border-radius: 4px; background: #fff; "
        "padding: 2px 10px; font-size: 12px; color: #606266; }"
        "QPushButton:hover { border-color: #409eff; color: #409eff; }"));
    card.settingsBtn->setVisible(false);
    // 按下标找卡片会在删除卡片后错位，所以按按钮自身找当前下标
    auto *settingsBtn = card.settingsBtn;
    connect(settingsBtn, &QPushButton::clicked, this, [this, settingsBtn]() {
        int cardIndex = -1;
        for (int i = 0; i < m_vizCards.size(); ++i) {
            if (m_vizCards[i].settingsBtn == settingsBtn) { cardIndex = i; break; }
        }
        showColorBlockSettingsDialog(cardIndex);
    });
    controlRow->addWidget(card.settingsBtn);

    cardLayout->addLayout(controlRow);

    // 内容区域（占位）
    auto *contentPlaceholder = new QLabel(T("autosc_content_placeholder"));
    contentPlaceholder->setAlignment(Qt::AlignCenter);
    contentPlaceholder->setMinimumHeight(80);
    contentPlaceholder->setStyleSheet(QStringLiteral(
        "QLabel { background: #f5f7fa; border: 1px dashed #dcdfe6; border-radius: 4px; "
        "color: #909399; font-size: 12px; }"));
    card.contentWidget = contentPlaceholder;
    cardLayout->addWidget(contentPlaceholder);

    // 信号连接
    const int cardIndex = m_vizCards.size();
    connect(card.typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, cardIndex](int idx) {
        if (cardIndex < m_vizCards.size()) {
            QStringList types = {"spectral_data", "spectral_curve", "color_data", "color_block"};
            m_vizCards[cardIndex].type = types.value(idx, "color_data");
            updateVizCardContent(m_vizCards[cardIndex]);
        }
    });
    connect(addBtn, &QPushButton::clicked, this, [this]() {
        addVizCard(QStringLiteral("color_data"));
        adjustVizPanelHeight();
    });
    connect(delBtn, &QPushButton::clicked, this, [this, cardIndex]() {
        if (m_vizCards.size() > 1) {
            removeVizCard(cardIndex);
            adjustVizPanelHeight();
        }
    });

    // 隐藏提示标签
    if (m_vizInfoLabel) m_vizInfoLabel->setVisible(false);

    // 添加到布局
    m_vizLayout->addWidget(cardWidget);
    m_vizCards.append(card);
    // 立即填充内容：色块卡片的「设置」按钮只在该类型下显示，靠这里生效
    updateVizCardContent(m_vizCards.last());

    // 更新删除按钮状态（只有一个卡片时禁用删除）
    updateDeleteButtonState();
}

void AutoScPage::removeVizCard(int index)
{
    if (index < 0 || index >= m_vizCards.size()) return;
    if (m_vizCards.size() <= 1) return; // 至少保留一个

    auto &card = m_vizCards[index];
    if (card.widget) {
        m_vizLayout->removeWidget(card.widget);
        card.widget->deleteLater();
    }
    m_vizCards.removeAt(index);

    // 如果没有卡片了（理论上不会发生），显示提示
    if (m_vizCards.isEmpty() && m_vizInfoLabel) {
        m_vizInfoLabel->setVisible(true);
    }

    // 更新删除按钮状态
    updateDeleteButtonState();
}

/**
 * 角度筛选按钮：勾选状态来自 card.angles，取消时至少保留一个角度。
 * 回调里按指针位置回查卡片索引，因为 m_vizCards 增删会让引用失效。
 */
void AutoScPage::addAngleToggleButtons(QHBoxLayout *row, VizCard &card, const QStringList &availableAngles)
{
    for (const auto &angleCode : availableAngles) {
        auto *btn = new QToolButton;
        btn->setText(angleCode);
        btn->setCheckable(true);
        btn->setChecked(card.angles.contains(angleCode));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral(
            "QToolButton { border: 1px solid #dcdfe6; border-radius: 4px; padding: 3px 10px; "
            "font-size: 11px; color: #606266; background: #fff; }"
            "QToolButton:hover { border-color: #409eff; color: #409eff; }"
            "QToolButton:checked { background: #409eff; color: #fff; border-color: #409eff; }"));
        const int cidx = [&]() {
            for (int k = 0; k < m_vizCards.size(); ++k)
                if (&m_vizCards[k] == &card) return k;
            return -1;
        }();
        connect(btn, &QToolButton::clicked, this, [this, cidx, angleCode]() {
            if (cidx < 0 || cidx >= m_vizCards.size()) return;
            auto &c = m_vizCards[cidx];
            if (c.angles.contains(angleCode)) {
                if (c.angles.size() > 1) c.angles.removeAll(angleCode);
            } else {
                c.angles.append(angleCode);
            }
            const int scrollPos = m_mainScroll ? m_mainScroll->verticalScrollBar()->value() : 0;
            updateVizCardContent(c);
            if (m_mainScroll) {
                QTimer::singleShot(0, [this, scrollPos]() {
                    m_mainScroll->verticalScrollBar()->setValue(scrollPos);
                });
            }
        });
        row->addWidget(btn);
    }
}

/** 色块卡片设置弹窗：目前只有一项「显示模式」（图像 / 渲染） */
void AutoScPage::showColorBlockSettingsDialog(int cardIndex)
{
    if (cardIndex < 0 || cardIndex >= m_vizCards.size()) return;

    QDialog dlg(this);
    dlg.setWindowTitle(T("settings"));
    auto *lay = new QVBoxLayout;
    lay->setContentsMargins(16, 16, 16, 16);
    lay->setSpacing(10);

    auto *row = new QHBoxLayout;
    row->setSpacing(8);
    row->addWidget(new QLabel(T("autosc_display_mode")));
    auto *modeCombo = new QComboBox;
    modeCombo->addItems({T("autosc_mode_image"), T("autosc_mode_render")});
    modeCombo->setCurrentIndex(m_vizCards[cardIndex].colorBlockMode == QStringLiteral("image") ? 0 : 1);
    row->addWidget(modeCombo, 1);
    lay->addLayout(row);

    auto *btns = new QDialogButtonBox;
    auto *confirmBtn = new QPushButton(T("confirm"));
    auto *cancelBtn = new QPushButton(T("cancel"));
    btns->addButton(confirmBtn, QDialogButtonBox::AcceptRole);
    btns->addButton(cancelBtn, QDialogButtonBox::RejectRole);
    lay->addWidget(btns);
    dlg.setLayout(lay);

    connect(confirmBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    if (cardIndex >= m_vizCards.size()) return;
    auto &c = m_vizCards[cardIndex];
    c.colorBlockMode = (modeCombo->currentIndex() == 0) ? QStringLiteral("image")
                                                        : QStringLiteral("render");
    // 记住选择：下次启动、以及之后新建的色块卡片都沿用，不用每次重新设置
    QSettings().setValue(QStringLiteral("autosc/colorBlockMode"), c.colorBlockMode);
    const int scrollPos = m_mainScroll ? m_mainScroll->verticalScrollBar()->value() : 0;
    updateVizCardContent(c);
    if (m_mainScroll) {
        QTimer::singleShot(0, [this, scrollPos]() {
            m_mainScroll->verticalScrollBar()->setValue(scrollPos);
        });
    }
}

void AutoScPage::updateVizCardContent(VizCard &card)
{
    if (!card.contentWidget) return;

    if (card.settingsBtn)
        card.settingsBtn->setVisible(card.type == QStringLiteral("color_block"));

    // 获取卡片的内容布局
    auto *cardWidget = card.widget;
    if (!cardWidget) return;
    auto *cardLayout = qobject_cast<QVBoxLayout*>(cardWidget->layout());
    if (!cardLayout) return;

    // 移除旧的内容控件（保留第一个控制栏）
    while (cardLayout->count() > 1) {
        auto *item = cardLayout->takeAt(cardLayout->count() - 1);
        if (item->widget()) {
            item->widget()->setParent(nullptr);
            item->widget()->deleteLater();
        }
        delete item;
    }

    QWidget *newContent = nullptr;

    if (card.type == QStringLiteral("spectral_data")) {
        // 光谱数据表格：显示名称、角度、各波长反射率
        auto *table = new QTableWidget;
        
        // 合并 标样 + 配方 + 修色 光谱数据（先到先去重 + 来源/角度排序）
        QJsonArray spectralData = buildMergedSpectralData();
        for (int si = 0; si < spectralData.size(); ++si) {
            const auto obj = spectralData[si].toObject();
        }

        // 收集所有波长
        QSet<int> wavelengthSet;
        for (int si = 0; si < spectralData.size(); ++si) {
            const auto obj = spectralData[si].toObject();
            if (si == 0) {
            }
            const auto values = spectralValuesFromItem(obj);
            for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
                const int wl = static_cast<int>(std::lround(it.key()));
                if (wl >= 380 && wl <= 780) wavelengthSet.insert(wl);
            }
        }
        QList<int> wavelengths = wavelengthSet.values();
        std::sort(wavelengths.begin(), wavelengths.end());
        
        // 如果没数据，使用默认波长
        if (wavelengths.isEmpty()) {
            for (int wl = 400; wl <= 700; wl += 10) wavelengths.append(wl);
        }
        
        // 设置列：名称 + 角度 + 波长列
        int cols = 2 + wavelengths.size();
        table->setColumnCount(cols);
        QStringList headers = {T("sc_name"), T("autosc_angle")};
        for (int wl : wavelengths) headers << QString::number(wl) + QStringLiteral("nm");
        table->setHorizontalHeaderLabels(headers);
        table->horizontalHeader()->setStretchLastSection(true);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setAlternatingRowColors(true);
        table->verticalHeader()->setVisible(false);
        table->setStyleSheet(QStringLiteral(
            "QTableWidget { border: 1px solid #e4e7ed; border-radius: 4px; background: #fff; "
            "gridline-color: #ebeef5; font-size: 11px; color: #333; }"
            "QHeaderView::section { background: #f5f7fa; border: none; border-bottom: 1px solid #e4e7ed; "
            "padding: 3px 6px; font-size: 11px; color: #606266; }")
            + QLatin1String(kScrollBarQss));

        if (!spectralData.isEmpty()) {
            table->setRowCount(spectralData.size());
            for (int i = 0; i < spectralData.size(); ++i) {
                auto item = spectralData[i].toObject();
                auto setText = [table, i](int col, const QString &text) {
                    auto *cell = new QTableWidgetItem(text);
                    cell->setTextAlignment(Qt::AlignCenter);
                    table->setItem(i, col, cell);
                };
                
                // 名称
                const QString sourceKey = item[QStringLiteral("sourceKey")].toString(QStringLiteral("target"));
                setText(0, scSourceLabel(sourceKey, currentJobTitle()));
                
                // 角度（兼容 geometryCode / geometry，去除前缀 r）
                const QString geo = scRowGeometry(item);
                setText(1, geo.isEmpty() ? QStringLiteral("—") : geo);
                
                // 波长反射率（兼容多格式）
                const auto values = spectralValuesFromItem(item);
                for (int j = 0; j < wavelengths.size(); ++j) {
                    const auto it = values.constFind(static_cast<double>(wavelengths[j]));
                    const QString text = (it == values.constEnd())
                                             ? QStringLiteral("—")
                                             : QString::number(it.value(), 'f', 2);
                    setText(2 + j, text);
                }
            }
            table->setFixedHeight(40 + spectralData.size() * 28);
        } else {
            table->setRowCount(0);
        }
        newContent = table;
    }
    else if (card.type == QStringLiteral("spectral_curve")) {
        // 光谱曲线图：对标前端 ScSpectralCurvePanel.vue（R / ΔR + 角度筛选）
        auto *container = new QWidget;
        auto *vbox = new QVBoxLayout(container);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->setSpacing(6);

        auto *chart = new SpectralCurveChart;
        chart->setMinimumHeight(200);
        chart->setMode(m_curveMode == QLatin1String("dR")
                           ? SpectralCurveChart::Mode::DeltaReflectance
                           : SpectralCurveChart::Mode::Reflectance);
        vbox->addWidget(chart, 1);

        // 可用角度（仅标准几何，按标准顺序）——对标前端 extractAvailableAngles
        {
            const QJsonArray rows = buildMergedSpectralData();
            QSet<QString> seen;
            for (const auto &v : rows) {
                const QString geo = scRowGeometry(v.toObject());
                if (scGeometryOrder().contains(geo)) seen.insert(geo);
            }
            QStringList available;
            for (const auto &geo : scGeometryOrder()) {
                if (seen.contains(geo)) available << geo;
            }
            // 可用角度集合变化时默认全选（对标前端 watch availableAngles）
            if (available != m_curveAvailableAngles) {
                m_curveAvailableAngles = available;
                m_curveSelectedAngles.clear();
                for (const auto &geo : available) m_curveSelectedAngles.insert(geo);
            }
        }

        // 按当前模式/角度筛选构建曲线数据
        auto seriesCount = std::make_shared<int>(0);
        auto applySeries = [this, chart, seriesCount]() {
            const QJsonArray rows = buildMergedSpectralData();
            QVector<SpectralCurveChart::Series> list;
            int idx = 0;
            for (const auto &v : rows) {
                const auto obj = v.toObject();
                const QString geo = scRowGeometry(obj);
                const QString sourceKey = obj[QStringLiteral("sourceKey")].toString(QStringLiteral("target"));
                const int paletteIdx = idx++;
                // 非标准几何始终保留；标准几何按已选角度过滤（前端 filterRowsByAngles）
                if (scGeometryOrder().contains(geo) && !m_curveSelectedAngles.contains(geo))
                    continue;
                SpectralCurveChart::Series s;
                s.sourceKey = sourceKey;
                s.geometry = geo;
                s.name = scSourceDisplayName(sourceKey, geo, currentJobTitle());
                s.legendName = scSourceLabel(sourceKey, currentJobTitle());
                s.color = scGeometryColor(geo, paletteIdx);
                s.lineStyle = scSourceLineStyle(sourceKey);
                s.values = spectralValuesFromItem(obj);
                if (!s.values.isEmpty()) list.append(s);
            }
            *seriesCount = list.size();
            chart->setSeries(list);
        };
        applySeries();

        // 底部工具条：模式分段控件（R / ΔR）+ 角度标签筛选
        // 对标前端 ScSpectralCurvePanel.vue（sc-spectral-curve__modes / ScAngleFilter.vue）
        auto *footerBar = new QWidget;
        auto *flow = new ScFlowLayout(footerBar, 8, 8);
        footerBar->setLayout(flow);

        auto *modeFrame = new QFrame(footerBar);
        modeFrame->setObjectName(QStringLiteral("scModeGroup"));
        modeFrame->setStyleSheet(QStringLiteral(
            "QFrame#scModeGroup { background: #f5f7fa; border: 1px solid #dcdfe6; border-radius: 6px; }"
            "QPushButton#scModeBtnFirst, QPushButton#scModeBtnLast { border: 0; padding: 4px 10px; "
            "font-size: 12px; font-weight: 600; color: #909399; background: transparent; }"
            "QPushButton#scModeBtnFirst { border-right: 1px solid #dcdfe6; "
            "border-top-left-radius: 5px; border-bottom-left-radius: 5px; }"
            "QPushButton#scModeBtnLast { border-top-right-radius: 5px; border-bottom-right-radius: 5px; }"
            "QPushButton#scModeBtnFirst:hover:!checked, QPushButton#scModeBtnLast:hover:!checked "
            "{ background: #eef0f3; color: #606266; }"
            "QPushButton#scModeBtnFirst:checked, QPushButton#scModeBtnLast:checked "
            "{ background: #409eff; color: #ffffff; }"));
        auto *modeLay = new QHBoxLayout(modeFrame);
        modeLay->setContentsMargins(0, 0, 0, 0);
        modeLay->setSpacing(0);

        auto *rBtn = new QPushButton(QStringLiteral("R"), modeFrame);
        auto *dRBtn = new QPushButton(QStringLiteral("ΔR"), modeFrame);
        rBtn->setObjectName(QStringLiteral("scModeBtnFirst"));
        dRBtn->setObjectName(QStringLiteral("scModeBtnLast"));
        for (auto *btn : {rBtn, dRBtn}) {
            btn->setCheckable(true);
            btn->setAutoExclusive(true); // 互斥：对标前端 el-radio 语义，同一父控件下仅一个选中
            btn->setCursor(Qt::PointingHandCursor);
            btn->setFixedHeight(24);
            modeLay->addWidget(btn);
        }
        (m_curveMode == QLatin1String("dR") ? dRBtn : rBtn)->setChecked(true);
        connect(rBtn, &QPushButton::clicked, this, [this, chart]() {
            m_curveMode = QStringLiteral("R");
            chart->setMode(SpectralCurveChart::Mode::Reflectance);
        });
        connect(dRBtn, &QPushButton::clicked, this, [this, chart]() {
            m_curveMode = QStringLiteral("dR");
            chart->setMode(SpectralCurveChart::Mode::DeltaReflectance);
        });
        flow->addWidget(modeFrame);

        const QString tagStyle = QStringLiteral(
            "QPushButton#scAngleTag { background: #ecf5ff; border: 1px solid #d9ecff; border-radius: 4px; "
            "color: #409eff; font-size: 12px; padding: 2px 8px; }"
            "QPushButton#scAngleTag:hover { background: #d9ecff; border-color: #c6e2ff; }");

        // 角度标签（可关闭，仅剩一个时不可关闭）+ “添加角度”下拉
        auto refreshFooter = std::make_shared<std::function<void()>>();
        *refreshFooter = [this, footerBar, flow, modeFrame, tagStyle, applySeries, refreshFooter]() {
            for (int i = flow->count() - 1; i >= 0; --i) {
                QLayoutItem *item = flow->itemAt(i);
                if (item->widget() == modeFrame) continue;
                flow->takeAt(i);
                if (QWidget *w = item->widget()) {
                    w->setParent(nullptr);
                    w->deleteLater();
                }
                delete item;
            }
            const bool closable = m_curveSelectedAngles.size() > 1;
            for (const auto &geo : m_curveAvailableAngles) {
                if (!m_curveSelectedAngles.contains(geo)) continue;
                auto *tag = new QPushButton(closable ? geo + QStringLiteral(" ×") : geo, footerBar);
                tag->setObjectName(QStringLiteral("scAngleTag"));
                tag->setStyleSheet(tagStyle);
                tag->setCursor(Qt::PointingHandCursor);
                tag->setFixedHeight(24);
                flow->addWidget(tag);
                connect(tag, &QPushButton::clicked, this, [this, geo, applySeries, refreshFooter]() {
                    if (m_curveSelectedAngles.size() <= 1) return; // 仅剩一个时不可移除
                    m_curveSelectedAngles.remove(geo);
                    applySeries();
                    (*refreshFooter)();
                });
            }
            QStringList remaining;
            for (const auto &geo : m_curveAvailableAngles) {
                if (!m_curveSelectedAngles.contains(geo)) remaining << geo;
            }
            if (!remaining.isEmpty()) {
                auto *addCombo = new QComboBox(footerBar);
                addCombo->setObjectName(QStringLiteral("scAngleAdd"));
                addCombo->setStyleSheet(QStringLiteral(
                    "QComboBox#scAngleAdd { background: #fff; border: 1px solid #dcdfe6; border-radius: 4px; "
                    "padding: 2px 8px; font-size: 12px; color: #606266; }"));
                addCombo->setFixedSize(120, 24);
                addCombo->setPlaceholderText(T("autosc_add_angle"));
                addCombo->addItems(remaining);
                addCombo->setCurrentIndex(-1);
                flow->addWidget(addCombo);
                connect(addCombo, &QComboBox::activated, this,
                        [this, addCombo, applySeries, refreshFooter](int index) {
                    const QString geo = addCombo->itemText(index);
                    if (geo.isEmpty()) return;
                    m_curveSelectedAngles.insert(geo);
                    applySeries();
                    (*refreshFooter)();
                });
            }
        };
        (*refreshFooter)();
        footerBar->setVisible(*seriesCount > 0); // 无曲线时隐藏工具条（对标前端 v-if="seriesData.length"）
        vbox->addWidget(footerBar);

        newContent = container;
    }
    else if (card.type == QStringLiteral("color_data")) {
        // 色彩数据表格：名称、角度、L*、a*、b*、C*、h*、色差等
        // 归一化光源标签
        auto normalizeIlluminant = [](const QString &raw) -> QString {
            QString s = raw.trimmed().toUpper().remove(' ');
            if (s.isEmpty()) return {};
            if (s.contains(QStringLiteral("D65"))) return QStringLiteral("D65-10");
            if (s == QStringLiteral("A") || s.startsWith(QStringLiteral("A-")) || s.contains(QStringLiteral("A10")))
                return QStringLiteral("A-10");
            if (s.contains(QStringLiteral("FL12")) || s.contains(QStringLiteral("F12")) || s.contains(QStringLiteral("TL84")))
                return QStringLiteral("FL12-10");
            return s;
        };
    
        // 合并测量色彩（标样）+ 配方可视化数据（初始配方+修色后）
        QJsonArray measurementColorData = extractColorimetry(m_vizData);
        QJsonArray formulaColorData = extractColorimetry(m_formulaVizData);
    
        // 从当前选中行的 correctDetail 中提取 AC 修色色彩数据
        QJsonArray acColorData;
        if (m_currentRowObj[QStringLiteral("corrected")].toBool()) {
            QJsonObject cd = m_currentRowObj[QStringLiteral("correctDetail")].toObject();
            if (!cd.isEmpty()) {
                for (const auto &key : {QStringLiteral("correctedColorimetry"), QStringLiteral("colorimetry"),
                                        QStringLiteral("colorData"), QStringLiteral("measurements")}) {
                    auto arr = cd[key].toArray();
                    if (!arr.isEmpty()) { acColorData = arr; break; }
                }
                QJsonArray normalizedAc;
                for (const auto &v : acColorData) {
                    QJsonObject o = v.toObject();
                    if (o.isEmpty()) continue;
                    if (o[QStringLiteral("sourceKey")].toString().isEmpty()) {
                        o[QStringLiteral("sourceKey")] = QStringLiteral("ac");
                        if (o[QStringLiteral("sourceName")].toString().isEmpty())
                            o[QStringLiteral("sourceName")] = QStringLiteral("AC");
                    }
                    normalizedAc.append(o);
                }
                acColorData = normalizedAc;
            }
        }
    
        QJsonArray colorData;
        for (const auto &item : measurementColorData) {
            auto obj = item.toObject();
            QString sourceKey = obj[QStringLiteral("sourceKey")].toString();
            if (sourceKey.isEmpty() || sourceKey == QStringLiteral("target"))
                colorData.append(item);
        }
        for (const auto &item : formulaColorData) colorData.append(item);
        {
            bool formulaColorHasAc = false;
            for (const auto &item : formulaColorData) {
                QString sk = item.toObject()[QStringLiteral("sourceKey")].toString().toLower();
                if (sk == QStringLiteral("ac") || sk == QStringLiteral("recipe")) { formulaColorHasAc = true; break; }
            }
            if (!formulaColorHasAc) {
                for (const auto &item : acColorData) colorData.append(item);
            }
        }


        // 展开 labRgbByIlluminant（多光源 Lab 数据）并合并到 colorData
        auto expandLabRgbByIlluminant = [&normalizeIlluminant](const QJsonObject &raw, const QString &sourceKey) -> QJsonArray {
            QJsonArray rows;
            if (raw.isEmpty()) return rows;
            for (auto illumIt = raw.begin(); illumIt != raw.end(); ++illumIt) {
                QString illuminant = normalizeIlluminant(illumIt.key());
                if (illuminant.isEmpty()) continue;
                auto byGeo = illumIt.value().toObject();
                for (auto geoIt = byGeo.begin(); geoIt != byGeo.end(); ++geoIt) {
                    auto lab = geoIt.value().toObject();
                    if (lab.isEmpty()) continue;
                    QJsonObject row;
                    row[QStringLiteral("geometryCode")] = geoIt.key();
                    row[QStringLiteral("geometry")] = geoIt.key();
                    row[QStringLiteral("sourceKey")] = sourceKey;
                    row[QStringLiteral("sourceName")] = sourceKey.toUpper();
                    row[QStringLiteral("illuminant")] = illuminant;
                    row[QStringLiteral("l")] = lab[QStringLiteral("l")];
                    row[QStringLiteral("a")] = lab[QStringLiteral("a")];
                    row[QStringLiteral("b")] = lab[QStringLiteral("b")];
                    row[QStringLiteral("c")] = lab[QStringLiteral("c")];
                    row[QStringLiteral("h")] = lab[QStringLiteral("h")];
                    rows.append(row);
                }
            }
            return rows;
        };

        // 从修色详情中展开 AC 修色多光源数据（对标前端 correctDetail.labRgbByIlluminant）
        QJsonObject labRgbByIllum;
        if (m_currentRowObj[QStringLiteral("corrected")].toBool()) {
            labRgbByIllum = m_currentRowObj[QStringLiteral("correctDetail")].toObject()[QStringLiteral("labRgbByIlluminant")].toObject();
        }
        if (!labRgbByIllum.isEmpty()) {
            QJsonArray acIllumRows = expandLabRgbByIlluminant(labRgbByIllum, QStringLiteral("ac"));
            // 移除 colorData 中已有的旧 AC 行，用多光源版本替代
            QJsonArray filtered;
            for (const auto &item : colorData) {
                QString sk = item.toObject()[QStringLiteral("sourceKey")].toString().toLower();
                if (sk != QStringLiteral("ac") && sk != QStringLiteral("recipe"))
                    filtered.append(item);
            }
            colorData = filtered;
            for (const auto &item : acIllumRows) colorData.append(item);
        }

        // 编辑预测（predictDetail）的色彩数据归到 acEdited 来源
        {
            const QJsonObject pd = m_currentRowObj[QStringLiteral("predictDetail")].toObject();
            if (!pd.isEmpty()) {
                QJsonArray editedColorData;
                for (const auto &key : {QStringLiteral("correctedColorimetry"), QStringLiteral("colorimetry"),
                                        QStringLiteral("colorData")}) {
                    const auto arr = pd[key].toArray();
                    if (!arr.isEmpty()) { editedColorData = arr; break; }
                }
                for (const auto &v : editedColorData) {
                    QJsonObject o = v.toObject();
                    if (o.isEmpty()) continue;
                    o[QStringLiteral("sourceKey")] = QStringLiteral("acEdited");
                    o[QStringLiteral("sourceName")] = QStringLiteral("AC-Edited");
                    colorData.append(o);
                }
                // 多光源 Lab：带 labRgbByIlluminant 时替换上面的单光源行
                const auto editedIllum = pd[QStringLiteral("labRgbByIlluminant")].toObject();
                if (!editedIllum.isEmpty()) {
                    const QJsonArray illumRows = expandLabRgbByIlluminant(editedIllum, QStringLiteral("acEdited"));
                    QJsonArray kept;
                    for (const auto &item : colorData) {
                        if (item.toObject()[QStringLiteral("sourceKey")].toString().toLower()
                            != QStringLiteral("acedited"))
                            kept.append(item);
                    }
                    colorData = kept;
                    for (const auto &item : illumRows) colorData.append(item);
                }
            }
        }


        // 检测是否含多光源字段
        bool hasMultiIlluminant = false;
        for (const auto &item : colorData) {
            auto obj = item.toObject();
            QString illum = normalizeIlluminant(obj[QStringLiteral("illuminant")].toString());
            if (!illum.isEmpty()) { hasMultiIlluminant = true; break; }
        }
    
        // 按当前光源过滤
        QJsonArray filteredData;
        if (hasMultiIlluminant) {
            for (const auto &item : colorData) {
                auto obj = item.toObject();
                QString illum = normalizeIlluminant(obj[QStringLiteral("illuminant")].toString());
                if (illum.isEmpty() || illum == card.illuminant)
                    filteredData.append(item);
            }
        } else {
            filteredData = colorData;
        }

        // 提取可用角度并初始化 card.angles（首次或角度列表变化时）
        static const QStringList ANGLE_ORDER = {
            QStringLiteral("45as15"), QStringLiteral("45as25"), QStringLiteral("45as45"),
            QStringLiteral("45as75"), QStringLiteral("45as110")
        };
        static const QMap<QString, QString> ANGLE_LABELS = {
            {"45as15", "15°"}, {"45as25", "25°"}, {"45as45", "45°"},
            {"45as75", "75°"}, {"45as110", "110°"}
        };
        QStringList availableAngles;
        {
            QSet<QString> geoSet;
            for (const auto &item : filteredData) {
                auto obj = item.toObject();
                QString geo = obj[QStringLiteral("geometryCode")].toString();
                if (geo.isEmpty()) geo = obj[QStringLiteral("geometry")].toString();
                if (ANGLE_ORDER.contains(geo)) geoSet.insert(geo);
            }
            for (const auto &a : ANGLE_ORDER) {
                if (geoSet.contains(a)) availableAngles.append(a);
            }
        }
        // 首次或角度列表变化时，默认全选
        if (card.angles.isEmpty() || card.angles != availableAngles) {
            // 保留用户已选且仍可用的角度
            QStringList preserved;
            for (const auto &a : card.angles) {
                if (availableAngles.contains(a)) preserved.append(a);
            }
            if (preserved.isEmpty()) {
                card.angles = availableAngles;
            } else {
                card.angles = preserved;
            }
        }

        // 按当前选中角度过滤
        QJsonArray angleFilteredData;
        {
            QSet<QString> selectedAngles(card.angles.begin(), card.angles.end());
            for (const auto &item : filteredData) {
                auto obj = item.toObject();
                QString geo = obj[QStringLiteral("geometryCode")].toString();
                if (geo.isEmpty()) geo = obj[QStringLiteral("geometry")].toString();
                // 非标准角度始终保留
                if (!ANGLE_ORDER.contains(geo) || selectedAngles.contains(geo))
                    angleFilteredData.append(item);
            }
        }
    
        // 外层容器：表格 + 可选光源切换
        auto *container = new QWidget;
        auto *outerLayout = new QVBoxLayout(container);
        outerLayout->setContentsMargins(0, 0, 0, 0);
        outerLayout->setSpacing(4);
    
        auto *table = new QTableWidget;
        table->setColumnCount(11);
        table->setHorizontalHeaderLabels({
            T("sc_name"), T("autosc_angle"), QStringLiteral("L*"), QStringLiteral("a*"),
            QStringLiteral("b*"), QStringLiteral("C*"), QStringLiteral("h°"),
            QStringLiteral("ΔE"), QStringLiteral("ΔL*"), QStringLiteral("Δa*"), QStringLiteral("Δb*")
        });
        table->horizontalHeader()->setStretchLastSection(true);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setAlternatingRowColors(true);
        table->verticalHeader()->setVisible(false);
        table->setStyleSheet(QStringLiteral(
            "QTableWidget { border: 1px solid #e4e7ed; border-radius: 4px; background: #fff; "
            "gridline-color: #ebeef5; font-size: 11px; color: #333; }"
            "QHeaderView::section { background: #f5f7fa; border: none; border-bottom: 1px solid #e4e7ed; "
            "padding: 3px 6px; font-size: 11px; color: #606266; }")
            + QLatin1String(kScrollBarQss));
    
        if (!angleFilteredData.isEmpty()) {
            table->setRowCount(angleFilteredData.size());
            for (int i = 0; i < angleFilteredData.size(); ++i) {
                auto item = angleFilteredData[i].toObject();
                auto setText = [table, i](int col, const QString &text) {
                    auto *cell = new QTableWidgetItem(text);
                    cell->setTextAlignment(Qt::AlignCenter);
                    table->setItem(i, col, cell);
                };
                auto fmtNum = [](const QJsonValue &v) -> QString {
                    double n = v.toDouble();
                    return std::isfinite(n) ? QString::number(n, 'f', 2) : QStringLiteral("—");
                };
                auto pickField = [&item](std::initializer_list<const char*> keys) -> QJsonValue {
                    for (const char *k : keys) {
                        auto v = item[QString::fromLatin1(k)];
                        if (!v.isNull() && !v.isUndefined()) return v;
                    }
                    return QJsonValue();
                };
    
                const QString sourceKey = item[QStringLiteral("sourceKey")].toString(QStringLiteral("target"));
                setText(0, scSourceLabel(sourceKey, currentJobTitle()));
                const QString geo = scRowGeometry(item);
                setText(1, geo.isEmpty() ? QStringLiteral("—") : geo);
                setText(2, fmtNum(pickField({"L", "l"})));
                setText(3, fmtNum(pickField({"a"})));
                setText(4, fmtNum(pickField({"b"})));
                setText(5, fmtNum(pickField({"C", "c"})));
                setText(6, fmtNum(pickField({"h"})));
                setText(7, fmtNum(pickField({"dE", "deltaE", "deltaE00", "deltaE76", "deltaE94"})));
                setText(8, fmtNum(pickField({"dL", "deltaL"})));
                setText(9, fmtNum(pickField({"da", "deltaA"})));
                setText(10, fmtNum(pickField({"db", "deltaB"})));
            }
            table->setFixedHeight(40 + angleFilteredData.size() * 28);
        } else {
            table->setRowCount(0);
        }
        outerLayout->addWidget(table);
    
        // 光源 + 角度切换按钮（同一行：左侧光源，右侧角度）
        if (!angleFilteredData.isEmpty()) {
            auto *footer = new QWidget;
            auto *footerLayout = new QHBoxLayout(footer);
            footerLayout->setContentsMargins(0, 2, 0, 0);
            footerLayout->setSpacing(6);

            // 左侧：光源切换按钮
            static const QStringList illumOptions = {
                QStringLiteral("D65-10"), QStringLiteral("A-10"), QStringLiteral("FL12-10")
            };
            for (const auto &opt : illumOptions) {
                auto *btn = new QToolButton;
                btn->setText(opt);
                btn->setCheckable(true);
                btn->setChecked(opt == card.illuminant);
                btn->setCursor(Qt::PointingHandCursor);
                btn->setStyleSheet(QStringLiteral(
                    "QToolButton { border: 1px solid #dcdfe6; border-radius: 4px; padding: 3px 10px; "
                    "font-size: 11px; color: #606266; background: #fff; }"
                    "QToolButton:hover { border-color: #409eff; color: #409eff; }"
                    "QToolButton:checked { background: #409eff; color: #fff; border-color: #409eff; }"));
                const int cidx = [&]() {
                    for (int k = 0; k < m_vizCards.size(); ++k)
                        if (&m_vizCards[k] == &card) return k;
                    return -1;
                }();
                connect(btn, &QToolButton::clicked, this, [this, cidx, opt]() {
                    if (cidx >= 0 && cidx < m_vizCards.size()) {
                        m_vizCards[cidx].illuminant = opt;
                        int scrollPos = m_mainScroll ? m_mainScroll->verticalScrollBar()->value() : 0;
                        updateVizCardContent(m_vizCards[cidx]);
                        if (m_mainScroll) {
                            QTimer::singleShot(0, [this, scrollPos]() {
                                m_mainScroll->verticalScrollBar()->setValue(scrollPos);
                            });
                        }
                    }
                });
                footerLayout->addWidget(btn);
            }

            // 中间弹性空间
            footerLayout->addStretch();

            // 右侧：角度切换按钮
            addAngleToggleButtons(footerLayout, card, availableAngles);

            outerLayout->addWidget(footer);
        }
    
        newContent = container;
    }
    else if (card.type == QStringLiteral("color_block")) {
        // 色块显示：对标前端 ScColorBlockPanel，按角度显示各色源的颜色
        auto *blockWidget = new QWidget;
        auto *blockLayout = new QVBoxLayout(blockWidget);
        blockLayout->setContentsMargins(8, 8, 8, 8);
        blockLayout->setSpacing(0);

        // 辅助 lambda：rgbByGeometry 对象 -> angle -> hex
        auto buildAngleHexMap = [](const QJsonObject &rgbByGeo) -> QMap<QString, QString> {
            QMap<QString, QString> result;
            if (rgbByGeo.isEmpty()) return result;
            static const QStringList allAngles = {
                QStringLiteral("45as15"), QStringLiteral("45as25"),
                QStringLiteral("45as45"), QStringLiteral("45as75"), QStringLiteral("45as110")
            };
            for (const auto &angle : allAngles) {
                if (rgbByGeo.contains(angle)) {
                    QString hex = hexFromRgb(rgbByGeo[angle].toObject());
                    if (hex.isEmpty()) {
                        auto v = rgbByGeo[angle];
                        if (v.isString()) {
                            hex = v.toString().trimmed();
                            if (!hex.isEmpty() && !hex.startsWith(QStringLiteral("#")))
                                hex.prepend(QStringLiteral("#"));
                        }
                    }
                    if (!hex.isEmpty()) result[angle] = hex;
                }
            }
            // 兼容带 r 前缀的角度码
            for (auto it = rgbByGeo.begin(); it != rgbByGeo.end(); ++it) {
                QString geo = it.key().trimmed();
                if (geo.startsWith(QStringLiteral("r"), Qt::CaseInsensitive))
                    geo = geo.mid(1);
                if (result.contains(geo)) continue;
                QString hex = hexFromRgb(it.value().toObject());
                if (hex.isEmpty() && it.value().isString()) {
                    hex = it.value().toString().trimmed();
                    if (!hex.isEmpty() && !hex.startsWith(QStringLiteral("#")))
                        hex.prepend(QStringLiteral("#"));
                }
                if (!hex.isEmpty()) result[geo] = hex;
            }
            return result;
        };

        // 构建各源的 angle->hex 映射（列名与色度数据等卡片一致，走 scSourceLabel 对标前端 formatScSourceLabel）
        // angleUrlMap 是「图像」模式的数据源，与搜索结果列表「作业 | 配方」用的是同一批预览图。
        struct SourceCol {
            QString sourceKey;
            QString label;
            QMap<QString, QString> angleMap;
            QMap<QString, QString> angleUrlMap;
        };
        QVector<SourceCol> columns;
        const bool imageMode = card.colorBlockMode == QStringLiteral("image");

        // 图像模式的行 = 预览图自带的角度名：不同设备角度数与命名都不同（可能是 4 张、名字也不是 45asNN），
        // 所以不能套 MA-5C 的五角度，行和底部角度按钮都跟着预览图走。
        QStringList imageAngles;
        auto appendPreviewItems = [&imageAngles](SourceCol &col, const QList<PreviewItem> &items) {
            for (const auto &it : items) {
                if (it.url.isEmpty()) continue;
                QString name = it.geometryCode.trimmed();
                if (name.startsWith(QLatin1Char('r'))) name = name.mid(1);
                if (name.isEmpty()) continue;
                col.angleUrlMap.insert(name, it.url);
                if (!imageAngles.contains(name)) imageAngles.append(name);
            }
        };
        // 配方侧预览图字段与结果列表取值顺序保持一致
        auto rowPreviewItems = [](const QJsonObject &obj) -> QList<PreviewItem> {
            auto items = parsePreviewItems(obj[QStringLiteral("previewImages")]);
            if (items.isEmpty()) items = parsePreviewItems(obj[QStringLiteral("previewImageItems")]);
            return items;
        };

        // 可视化的每个角度行都自带 rgb.hex：只选了测量作业（还没搜色、没点配方行）时
        // 也能显示标样色块，与色彩数据卡片的数据可得性一致。
        const QMap<QString, QMap<QString, QString>> vizHex =
            colorimetryHexBySource(extractColorimetry(m_vizData));
        const QMap<QString, QMap<QString, QString>> formulaHex =
            colorimetryHexBySource(extractColorimetry(m_formulaVizData));

        // 目标色（target）——对标前端 targetRgbByGeometry prop
        {
            SourceCol col;
            col.sourceKey = QStringLiteral("target");
            col.label = scSourceLabel(QStringLiteral("target"), currentJobTitle());
            col.angleMap = buildAngleHexMap(m_searchTargetRgbByGeometry);
            mergeMissingAngles(col.angleMap, vizHex.value(QStringLiteral("target")));
            // 作业侧预览图：m_jobPreviewUrls 存时已按角度排好序，角度名沿用后端原始码
            {
                QList<PreviewItem> jobItems;
                for (int i = 0; i < m_jobPreviewUrls.size(); ++i) {
                    PreviewItem it; it.url = m_jobPreviewUrls[i];
                    it.geometryCode = (i < m_jobPreviewGeoCodes.size()) ? m_jobPreviewGeoCodes[i] : QString();
                    it.sortOrder = i;
                    if (!it.url.isEmpty()) jobItems.append(it);
                }
                appendPreviewItems(col, jobItems);
            }
            // 渲染模式始终保留标样列（无数据也占位）；图像模式只列真的有图的来源
            if (!imageMode || !col.angleUrlMap.isEmpty()) columns.append(col);
        }

        // 初始配方（initial）——对标前端 initialRgbByGeometry = selectedRow.rgbByGeometry
        {
            SourceCol col;
            col.sourceKey = QStringLiteral("initial");
            col.label = scSourceLabel(QStringLiteral("initial"), currentJobTitle());
            col.angleMap = buildAngleHexMap(m_rowRgbByGeometry);
            mergeMissingAngles(col.angleMap, formulaHex.value(QStringLiteral("initial")));
            appendPreviewItems(col, rowPreviewItems(m_currentRowObj));
            if (col.angleUrlMap.isEmpty())
                appendPreviewItems(col, rowPreviewItems(m_currentRowObj[QStringLiteral("formula")].toObject()));
            if (!col.angleMap.isEmpty() || (imageMode && !col.angleUrlMap.isEmpty())) columns.append(col);
        }

        // AC 修色（ac）——对标前端 acRgbByGeometry = correctDetail.rgbByGeometry
        if (m_currentRowObj[QStringLiteral("corrected")].toBool()) {
            QJsonObject cd = m_currentRowObj[QStringLiteral("correctDetail")].toObject();
            QJsonObject acRgb = cd[QStringLiteral("rgbByGeometry")].toObject();
            if (acRgb.isEmpty()) acRgb = cd[QStringLiteral("targetRgbByGeometry")].toObject();
            SourceCol col;
            col.sourceKey = QStringLiteral("ac");
            col.label = scSourceLabel(QStringLiteral("ac"), currentJobTitle());
            col.angleMap = buildAngleHexMap(acRgb);
            mergeMissingAngles(col.angleMap, formulaHex.value(QStringLiteral("ac")));
            appendPreviewItems(col, rowPreviewItems(cd));
            if (!col.angleMap.isEmpty() || (imageMode && !col.angleUrlMap.isEmpty())) columns.append(col);
        }

        // AC 编辑（acEdited）——对标前端 acEditedRgbByGeometry = predictDetail.rgbByGeometry
        {
            QJsonObject pd = m_currentRowObj[QStringLiteral("predictDetail")].toObject();
            QJsonObject editedRgb = pd[QStringLiteral("rgbByGeometry")].toObject();
            if (!editedRgb.isEmpty()) {
                SourceCol col;
                col.sourceKey = QStringLiteral("acEdited");
                col.label = scSourceLabel(QStringLiteral("acEdited"), currentJobTitle());
                col.angleMap = buildAngleHexMap(editedRgb);
                appendPreviewItems(col, rowPreviewItems(pd));
                if (!col.angleMap.isEmpty() || (imageMode && !col.angleUrlMap.isEmpty())) columns.append(col);
            }
        }

        // 角度行：渲染模式按 MA-5C 标准顺序取有颜色的角度；图像模式直接按预览图自带的角度名，
        // 两者共用 card.angles，切换模式时不在新集合里的选择会被自动丢弃并全选。
        QStringList availableAngles;
        if (imageMode) {
            availableAngles = imageAngles;
        } else {
            static const QStringList allAngles = {
                QStringLiteral("45as15"), QStringLiteral("45as25"),
                QStringLiteral("45as45"), QStringLiteral("45as75"), QStringLiteral("45as110")
            };
            for (const auto &angle : allAngles) {
                for (const auto &col : columns) {
                    if (col.angleMap.contains(angle)) {
                        availableAngles.append(angle);
                        break;
                    }
                }
            }
        }

        // 角度筛选：与色彩数据卡片共用 card.angles，首次或角度变化时默认全选
        {
            QStringList preserved;
            for (const auto &a : card.angles) {
                if (availableAngles.contains(a)) preserved.append(a);
            }
            card.angles = preserved.isEmpty() ? availableAngles : preserved;
        }
        QStringList displayAngles;
        for (const auto &a : availableAngles) {
            if (card.angles.contains(a)) displayAngles.append(a);
        }

        if (availableAngles.isEmpty()) {
            auto *label = new QLabel(imageMode ? T("autosc_no_preview_image")
                                               : T("autosc_no_color_block"));
            label->setAlignment(Qt::AlignCenter);
            label->setMinimumHeight(160);
            label->setStyleSheet(QStringLiteral(
                "QLabel { background: #f5f7fa; border: 1px dashed #dcdfe6; border-radius: 8px; "
                "color: #a8abb2; font-size: 13px; padding: 20px; }"));
            blockLayout->addWidget(label);
        } else {
            // 角度标签列 72px、与色条留 10px 间隙、行间距 8px（对标前端 .sc-color-block__row）
            constexpr int kAngleLabelW = 72;
            constexpr int kColGap = 10;
            constexpr int kRowGap = 8;

            // ── 图例行（对标前端 sc-color-block__row--legend）──
            auto *legendRow = new QHBoxLayout;
            legendRow->setContentsMargins(0, 0, 0, 0);
            legendRow->setSpacing(kColGap);
            auto *legendSpacer = new QLabel;
            legendSpacer->setFixedWidth(kAngleLabelW);
            legendRow->addWidget(legendSpacer);
            for (const auto &col : columns) {
                auto *lbl = new QLabel(col.label);
                lbl->setAlignment(Qt::AlignCenter);
                lbl->setStyleSheet(QStringLiteral(
                    "font-size: 12px; color: #909399; background: transparent;"));
                legendRow->addWidget(lbl, 1);
            }
            blockLayout->addLayout(legendRow);
            blockLayout->addSpacing(kRowGap);

            // ── 每个角度一行（对标前端 sc-color-block__row）──
            // 图像模式：与「作业 | 配方」共用同一份缓存和下载器，图晚到只需重绘。
            ScColorBlockCell::Resolver cellResolver;
            if (imageMode) {
                cellResolver = [this](const QString &url) -> QPixmap {
                    requestPreviewImage(url);
                    return m_previewImgCache.value(url);
                };
                for (int i = m_colorBlockCells.size() - 1; i >= 0; --i)
                    if (m_colorBlockCells[i].isNull()) m_colorBlockCells.removeAt(i);
            }

            for (int ai = 0; ai < displayAngles.size(); ++ai) {
                const QString &angle = displayAngles.at(ai);

                auto *rowLayout = new QHBoxLayout;
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(kColGap);

                auto *angleLbl = new QLabel(angle);
                angleLbl->setFixedWidth(kAngleLabelW);
                angleLbl->setStyleSheet(QStringLiteral(
                    "font-size: 12px; color: #909399; background: transparent;"));
                rowLayout->addWidget(angleLbl);

                // 色块条（对标前端 sc-color-block__strip：height 36、圆角 4、1px 边框）
                // 选择器必须带 objectName：无选择器的样式表会级联到子控件，给每个格子
                // 凭空加上 1px 边框和内容边距，色条里就会出现细缝。
                auto *stripWidget = new QWidget;
                stripWidget->setObjectName(QStringLiteral("scBlockStrip"));
                stripWidget->setFixedHeight(36);
                stripWidget->setStyleSheet(QStringLiteral(
                    "QWidget#scBlockStrip { background: #fff; border: 1px solid #ebeef5; "
                    "border-radius: 4px; }"));
                auto *stripLayout = new QHBoxLayout(stripWidget);
                // 显式设 0 边距会盖掉样式表边框自带的 1px 内缩，格子会压住色条边框，
                // 所以这里手动留出边框宽度，和前端 border + overflow:hidden 一致。
                stripLayout->setContentsMargins(1, 1, 1, 1);
                stripLayout->setSpacing(0);

                for (int ci = 0; ci < columns.size(); ++ci) {
                    const auto &col = columns.at(ci);
                    const QString hex = col.angleMap.value(angle);
                    const QString url = imageMode ? col.angleUrlMap.value(angle) : QString();
                    auto *cell = new ScColorBlockCell(
                        imageMode ? QString() : hex, url, cellResolver,
                        ci == 0, ci == columns.size() - 1, ci < columns.size() - 1);
                    QString tip;
                    if (!url.isEmpty()) tip = QStringLiteral("%1: %2").arg(col.label, angle);
                    else if (!hex.isEmpty()) tip = QStringLiteral("%1: %2").arg(col.label, hex);
                    else tip = T("autosc_col_no_data").arg(col.label);
                    cell->setToolTip(tip);
                    if (imageMode) m_colorBlockCells.append(cell);
                    stripLayout->addWidget(cell, 1);
                }
                rowLayout->addWidget(stripWidget, 1);
                blockLayout->addLayout(rowLayout);
                if (ai < displayAngles.size() - 1) blockLayout->addSpacing(kRowGap);
            }

            // ── 底部角度按钮（与色彩数据卡片同一套交互）──
            if (availableAngles.size() > 1) {
                auto *footer = new QWidget;
                auto *footerLayout = new QHBoxLayout(footer);
                footerLayout->setContentsMargins(0, 4, 0, 0);
                footerLayout->setSpacing(6);
                footerLayout->addStretch();
                addAngleToggleButtons(footerLayout, card, availableAngles);
                blockLayout->addWidget(footer);
            }
        }

        newContent = blockWidget;
    }
    else {
        // 未知类型占位
        auto *label = new QLabel(T("autosc_content_placeholder"));
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumHeight(80);
        label->setStyleSheet(QStringLiteral(
            "QLabel { background: #f5f7fa; border: 1px dashed #dcdfe6; border-radius: 4px; "
            "color: #909399; font-size: 12px; }"));
        newContent = label;
    }

    if (newContent) {
        cardLayout->addWidget(newContent);
        card.contentWidget = newContent;
    }

    // 内容变化后自适应面板高度
    adjustVizPanelHeight();
}

void AutoScPage::adjustVizPanelHeight()
{
    if (!m_vizPanel || !m_vizLayout) return;
    auto *vp = static_cast<ResizablePanel*>(m_vizPanel);
    QTimer::singleShot(0, this, [this, vp]() {
        auto *vLayout = vp->contentLayout();
        if (!vLayout) return;
        vLayout->invalidate();
        // 关键：用 sizeHint().height() 计算所需高度，不受当前面板 fixed height 压缩影响
        int contentH = vLayout->sizeHint().height();
        // ResizablePanel 固定开销：
        //   collapsedH(46=outer2+content10+header~22+content10+outer2)
        // + bodyLayout addSpacing(8)
        // + dragBar(10)
        // = 64
        const int panelOverhead = 64;
        int panelH = contentH + panelOverhead;
        int minH = vp->property("logicalMinH").toInt();
        if (minH <= 0) minH = 80;
        vp->setFixedHeight(qMax(minH, panelH));
        updateScrollContainerHeight();
    });
}

void AutoScPage::updateDeleteButtonState()
{
    // 遍历所有卡片，只有一个时禁用删除按钮
    bool canDelete = m_vizCards.size() > 1;
    for (auto &card : m_vizCards) {
        if (card.widget) {
            auto *delBtn = card.widget->findChild<QPushButton*>(QString(), Qt::FindDirectChildrenOnly);
            // 找到删除按钮（第二个 QPushButton，第一个是添加）
            auto btns = card.widget->findChildren<QPushButton*>();
            for (auto *btn : btns) {
                if (btn->text() == QStringLiteral("×")) {
                    btn->setEnabled(canDelete);
                    break;
                }
            }
        }
    }
}

#include "AutoScPage.moc"
