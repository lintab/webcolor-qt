/* ============================================================
   FormulaDetailWidget.cpp —— 配方详情面板实现
   ============================================================ */

#include "FormulaDetailWidget.h"
#include "api/FormulaApi.h"
#include "api/HttpClient.h"
#include "models/FormulaModel.h"
#include "utils/FormulaExport.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDateTime>

static const QStringList PIGMENT_SYSTEMS = {
    QStringLiteral("internal"),
    QStringLiteral("mengying"),
    QStringLiteral("xiongtu"),
    QStringLiteral("kaiqi"),
    QStringLiteral("shikasi")
};
static const QStringList PIGMENT_LABELS = {
    QStringLiteral("内部编号"),
    QStringLiteral("猛鹰"),
    QStringLiteral("雄途"),
    QStringLiteral("恺祁"),
    QStringLiteral("施卡斯")
};

static QString coatTypeLabel(const QString &v)
{
    if (v.isEmpty()) return {};
    QString upper = v.toUpper();
    static const QMap<QString, QString> map = {
        {QStringLiteral("METALLIC"), QStringLiteral("金属漆")},
        {QStringLiteral("SOLID"),    QStringLiteral("素色漆")},
        {QStringLiteral("PEARL"),    QStringLiteral("珍珠漆")},
        {QStringLiteral("MICA"),     QStringLiteral("云母漆")}
    };
    return map.value(upper, v);
}

static QString layerLabel(int n)
{
    if (n == 1) return QStringLiteral("底层");
    if (n == 2) return QStringLiteral("面层");
    return QStringLiteral("第%1层").arg(n);
}

static QString formatWeight(double n)
{
    return QString::number(n, 'f', 2);
}

// ---------- 卡片样式辅助 ----------
static QFrame *makeCard(const QString &title)
{
    auto *card = new QFrame;
    card->setStyleSheet(QStringLiteral(
        "background: #ffffff; border: 1px solid #e0e3e8; border-radius: 10px;"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    if (!title.isEmpty()) {
        auto *titleLabel = new QLabel(title);
        titleLabel->setStyleSheet(QStringLiteral(
            "font-size: 15px; font-weight: 600; color: #1a2332; "
            "padding-bottom: 8px; border-bottom: 1px solid #f0f2f5; border: none;"));
        layout->addWidget(titleLabel);
    }
    return card;
}

FormulaDetailWidget::FormulaDetailWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("detailPanel"));
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet(QStringLiteral(
        "QScrollBar:vertical { background: #f0f2f5; width: 10px; margin: 0; border-radius: 5px; }"
        "QScrollBar::handle:vertical { background: #606266; min-height: 30px; border-radius: 5px; }"
        "QScrollBar::handle:vertical:hover { background: #4a4c50; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"));

    // 空状态
    m_emptyWidget = new QWidget;
    auto *emptyLayout = new QVBoxLayout(m_emptyWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    auto *emptyLabel = new QLabel(QStringLiteral("点击左侧配方查看详情"));
    emptyLabel->setObjectName(QStringLiteral("detailEmpty"));
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet(QStringLiteral("color: #909399; font-size: 14px;"));
    emptyLayout->addWidget(emptyLabel);

    // 内容容器
    m_contentWidget = new QWidget;
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(16, 16, 16, 16);
    m_contentLayout->setSpacing(12);
    m_contentWidget->hide();

    auto *container = new QWidget;
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(m_emptyWidget);
    containerLayout->addWidget(m_contentWidget);
    m_scrollArea->setWidget(container);
    mainLayout->addWidget(m_scrollArea);
}

void FormulaDetailWidget::clear()
{
    m_contentWidget->hide();
    m_emptyWidget->show();
    m_currentId = 0;
    m_currentFormula = QJsonObject();
}

void FormulaDetailWidget::loadDetail(int id)
{
    m_currentId = id;
    FormulaApi::instance().getFormulaDetail(id, m_currentPigmentSystem,
        [this](const QJsonObject &res) {
            auto data = res.value(QStringLiteral("data")).toObject();
            auto detail = FormulaModel::parseFormulaDetail(data);
            m_currentFormula = detail.formula;
            renderDetail(detail);
        },
        [this](const QString &errMsg, int) {
            QMessageBox::warning(this, QStringLiteral("错误"), errMsg);
        }
    );
}

void FormulaDetailWidget::renderDetail(const FormulaDetail &detail)
{
    // 清空旧内容
    QLayoutItem *child;
    while ((child = m_contentLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    auto f = detail.formula;
    m_emptyWidget->hide();
    m_contentWidget->show();

    // ===== 1. 头部卡片 =====
    auto *headCard = makeCard({});
    auto *headLayout = qobject_cast<QVBoxLayout *>(headCard->layout());
    auto *headRow = new QWidget;
    auto *headRowLayout = new QHBoxLayout(headRow);
    headRowLayout->setSpacing(12);

    // 缩略图
    auto *thumbLabel = new QLabel;
    thumbLabel->setFixedSize(64, 64);
    thumbLabel->setAlignment(Qt::AlignCenter);
    thumbLabel->setStyleSheet(QStringLiteral(
        "background: #f0f2f5; border-radius: 12px; font-size: 24px; color: #0d4f8b;"));
    QString imgUrl = f.value(QStringLiteral("imageUrl")).toString();
    if (!imgUrl.isEmpty()) {
        auto *nam = new QNetworkAccessManager(thumbLabel);
        connect(nam, &QNetworkAccessManager::finished, thumbLabel, [thumbLabel, nam](QNetworkReply *reply) {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pix;
                pix.loadFromData(reply->readAll());
                if (!pix.isNull()) {
                    thumbLabel->setPixmap(pix.scaled(64, 64, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                }
            }
            nam->deleteLater();
        });
        nam->get(QNetworkRequest(QUrl(FormulaModel::getResourceUrl(imgUrl))));
    } else {
        QString ch = f.value(QStringLiteral("colorCode")).toString();
        thumbLabel->setText(ch.isEmpty() ? QStringLiteral("?") : ch.left(1).toUpper());
    }
    headRowLayout->addWidget(thumbLabel);

    // 名称 + 色号 + 标签
    auto *infoWidget = new QWidget;
    auto *infoLayout = new QVBoxLayout(infoWidget);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(4);

    auto *titleLine = new QWidget;
    auto *titleLayout = new QHBoxLayout(titleLine);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(8);
    QString colorName = f.value(QStringLiteral("colorName")).toString(
                        f.value(QStringLiteral("colorNameEn")).toString());
    if (!colorName.isEmpty()) {
        auto *nameLabel = new QLabel(
            QStringLiteral("<span style='color:#909399;font-size:12px;'>名称：</span>"
                           "<span style='font-size:18px;font-weight:700;color:#1a2332;'>%1</span>")
                .arg(colorName.toHtmlEscaped()));
        nameLabel->setTextFormat(Qt::RichText);
        titleLayout->addWidget(nameLabel);
    }
    QString colorCode = f.value(QStringLiteral("colorCode")).toString();
    if (!colorCode.isEmpty()) {
        auto *codeLabel = new QLabel(
            QStringLiteral("<span style='color:#909399;font-size:12px;'>色号：</span>"
                           "<span style='font-size:13px;color:#606266;'>%1</span>")
                .arg(colorCode.toHtmlEscaped()));
        codeLabel->setTextFormat(Qt::RichText);
        titleLayout->addWidget(codeLabel);
    }
    titleLayout->addStretch();
    infoLayout->addWidget(titleLine);

    // 属性标签
    auto *tagsWidget = new QWidget;
    auto *tagsLayout = new QHBoxLayout(tagsWidget);
    tagsLayout->setContentsMargins(0, 0, 0, 0);
    tagsLayout->setSpacing(4);
    auto addTag = [&](const QString &label, const QString &text) {
        auto *tag = new QLabel(
            QStringLiteral("<span style='color:#909399;font-size:11px;'>%1</span>"
                           "<span style='color:#606266;font-size:11px;'>%2</span>")
                .arg(label.toHtmlEscaped(), text.toHtmlEscaped()));
        tag->setTextFormat(Qt::RichText);
        tag->setStyleSheet(QStringLiteral(
            "background: #f0f2f5; border-radius: 4px; padding: 2px 8px;"));
        tagsLayout->addWidget(tag);
    };
    if (f.value(QStringLiteral("threeStep")).toBool())
        addTag(QStringLiteral("工艺："), QStringLiteral("三工序"));
    QString coatType = f.value(QStringLiteral("coatType")).toString();
    if (!coatType.isEmpty()) addTag(QStringLiteral("类型："), coatTypeLabel(coatType));
    QString year = f.value(QStringLiteral("year")).toString();
    if (!year.isEmpty()) addTag(QStringLiteral("年份："), year + QStringLiteral(" 款"));
    QString region = f.value(QStringLiteral("applicableRegion")).toString();
    if (!region.isEmpty()) addTag(QStringLiteral("适应地区："), region);
    headLayout->addWidget(headRow);
    if (tagsLayout->count() > 0) infoLayout->addWidget(tagsWidget);
    headRowLayout->addWidget(infoWidget, 1);
    m_contentLayout->addWidget(headCard);

    // ===== 2. 配方信息卡片 =====
    auto *infoCard = makeCard(QStringLiteral("配方信息"));
    auto *infoCardLayout = qobject_cast<QVBoxLayout *>(infoCard->layout());
    auto *gridWidget = new QWidget;
    auto *gridLayout = new QGridLayout(gridWidget);
    gridLayout->setHorizontalSpacing(24);
    gridLayout->setVerticalSpacing(8);

    struct GridPair { QString label; QString value; };
    QList<GridPair> pairs = {
        {QStringLiteral("品牌"), f.value(QStringLiteral("brandName")).toString()},
        {QStringLiteral("车系"), f.value(QStringLiteral("seriesName")).toString()},
        {QStringLiteral("车型"), f.value(QStringLiteral("modelName")).toString()},
        {QStringLiteral("色号"), colorCode},
        {QStringLiteral("配方名称"), f.value(QStringLiteral("formulaName")).toString()},
        {QStringLiteral("颜色属性"), f.value(QStringLiteral("coatType")).toString()},
        {QStringLiteral("颜色偏向"), f.value(QStringLiteral("colorBias")).toString()},
        {QStringLiteral("配方类型"), f.value(QStringLiteral("formulaType")).toInt() == 1
            ? QStringLiteral("客户配方") : f.value(QStringLiteral("formulaType")).toInt() == 0
            ? QStringLiteral("公司配方") : QString()},
        {QStringLiteral("产地"), f.value(QStringLiteral("origin")).toString()},
        {QStringLiteral("年份"), f.value(QStringLiteral("year")).toString()},
        {QStringLiteral("上传时间"), f.value(QStringLiteral("createdAt")).toString()},
        {QStringLiteral("更新时间"), f.value(QStringLiteral("updatedAt")).toString()},
        {QStringLiteral("作者"), f.value(QStringLiteral("submitterNickName")).toString(
                                   f.value(QStringLiteral("userName")).toString())},
    };
    // 每行显示两个字段
    int col = 0, row = 0;
    for (const auto &p : pairs) {
        if (p.value.isEmpty()) continue;
        auto *fieldLabel = new QLabel(
            QStringLiteral("<span style='color:#909399;font-size:12px;'>%1：</span>"
                           "<span style='color:#1a2332;font-size:13px;font-weight:500;'>%2</span>")
                .arg(p.label, p.value.toHtmlEscaped()));
        fieldLabel->setTextFormat(Qt::RichText);
        fieldLabel->setWordWrap(true);
        gridLayout->addWidget(fieldLabel, row, col);
        col++;
        if (col >= 2) { col = 0; row++; }
    }
    infoCardLayout->addWidget(gridWidget);
    m_contentLayout->addWidget(infoCard);

    // ===== 3. 色母明细卡片 =====
    auto *pigmentCard = makeCard({});
    auto *pigmentCardLayout = qobject_cast<QVBoxLayout *>(pigmentCard->layout());

    // 标题行 + 编码体系下拉
    auto *pigmentTitleRow = new QWidget;
    auto *pigmentTitleLayout = new QHBoxLayout(pigmentTitleRow);
    pigmentTitleLayout->setContentsMargins(0, 0, 0, 0);
    auto *pigmentTitleLabel = new QLabel(
        QStringLiteral("配方详情"));
    pigmentTitleLabel->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 600; color: #1a2332;"));
    pigmentTitleLayout->addWidget(pigmentTitleLabel);
    pigmentTitleLayout->addStretch();

    auto *systemLabel = new QLabel(QStringLiteral("品牌："));
    systemLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #606266;"));
    pigmentTitleLayout->addWidget(systemLabel);

    auto *systemCombo = new QComboBox;
    systemCombo->setFixedSize(120, 32);
    systemCombo->setStyleSheet(QStringLiteral(
        "QComboBox { background: #e8f4fd; border: 1px solid #b8ddf5; border-radius: 8px; "
        "padding: 4px 28px 4px 10px; font-size: 13px; color: #0d4f8b; min-height: 20px; }"
        "QComboBox:hover { background: #d6ecfa; border-color: #4a9ede; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox::down-arrow { image: none; border: none; "
        "border-left: 4px solid transparent; border-right: 4px solid transparent; "
        "border-top: 5px solid #0d4f8b; margin-right: 6px; }"
        "QComboBox QAbstractItemView { background: #ffffff; border: 1px solid #b8ddf5; "
        "border-radius: 6px; selection-background-color: #e8f4fd; selection-color: #0d4f8b; "
        "padding: 4px; }"
        "QComboBox QAbstractItemView::item { padding: 6px 8px; }"));
    for (int i = 0; i < PIGMENT_SYSTEMS.size(); ++i) {
        systemCombo->addItem(PIGMENT_LABELS[i], PIGMENT_SYSTEMS[i]);
        if (PIGMENT_SYSTEMS[i] == m_currentPigmentSystem)
            systemCombo->setCurrentIndex(i);
    }
    connect(systemCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, systemCombo](int) {
        m_currentPigmentSystem = systemCombo->currentData().toString();
        if (m_currentId > 0) loadDetail(m_currentId);
    });
    pigmentTitleLayout->addWidget(systemCombo);
    pigmentCardLayout->addWidget(pigmentTitleRow);

    // 按层分组渲染表格
    QMap<int, QList<PigmentDetailRow>> layerGroups;
    for (const auto &row : detail.detailRows) {
        layerGroups[row.layer].append(row);
    }
    auto layerKeys = layerGroups.keys();
    std::sort(layerKeys.begin(), layerKeys.end());

    for (int layer : layerKeys) {
        renderPigmentTable(layerGroups[layer], layer, pigmentCardLayout);
    }
    m_contentLayout->addWidget(pigmentCard);

    // ===== 4. 备注卡片 =====
    QString remark = f.value(QStringLiteral("remark")).toString();
    if (!remark.isEmpty()) {
        auto *remarkCard = makeCard(QStringLiteral("备注"));
        auto *remarkLayout = qobject_cast<QVBoxLayout *>(remarkCard->layout());
        auto *remarkLabel = new QLabel(remark);
        remarkLabel->setWordWrap(true);
        remarkLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #1a2332;"));
        remarkLayout->addWidget(remarkLabel);
        m_contentLayout->addWidget(remarkCard);
    }

    m_contentLayout->addStretch();
}

void FormulaDetailWidget::renderPigmentTable(const QList<PigmentDetailRow> &rows,
                                              int layer, QVBoxLayout *container)
{
    // 层标题 + 下载按钮
    auto *layerTitleRow = new QWidget;
    auto *layerTitleLayout = new QHBoxLayout(layerTitleRow);
    layerTitleLayout->setContentsMargins(0, 4, 0, 4);

    auto *layerLabelW = new QLabel(layerLabel(layer));
    layerLabelW->setProperty("class", QStringLiteral("layer-title"));
    layerLabelW->setStyleSheet(QStringLiteral("font-size: 14px; font-weight: 600; color: #1a2332;"));
    layerTitleLayout->addWidget(layerLabelW);
    layerTitleLayout->addStretch();

    auto *downloadBtn = new QPushButton(QStringLiteral("下载配方"));
    downloadBtn->setCursor(Qt::PointingHandCursor);
    downloadBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #e8f4fd; color: #0d4f8b; font-size: 13px; font-weight: 600; "
        "border: 1px solid #b8ddf5; border-radius: 16px; padding: 6px 18px; }"
        "QPushButton:hover { background: #d6ecfa; border-color: #4a9ede; }"
        "QPushButton:pressed { background: #c4e2f7; }"));
    connect(downloadBtn, &QPushButton::clicked, this, [this, layer]() {
        downloadLayerFormula(layer);
    });
    layerTitleLayout->addWidget(downloadBtn);
    container->addWidget(layerTitleRow);

    // 表格
    auto *table = new QTableWidget;
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({QStringLiteral("序号"),
                                       QStringLiteral("色母编号"),
                                       QStringLiteral("色母名称"),
                                       QStringLiteral("用量(克)"),
                                       QStringLiteral("累计(克)")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);

    double cumulative = 0;
    double sum = 0;
    for (const auto &r : rows) sum += r.weight;

    table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const auto &r = rows[i];
        cumulative += r.weight;

        // 序号
        auto *seqItem = new QTableWidgetItem(QString::number(i + 1));
        seqItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 0, seqItem);

        table->setItem(i, 1, new QTableWidgetItem(r.pigmentCode));
        table->setItem(i, 2, new QTableWidgetItem(r.pigmentName));

        auto *weightItem = new QTableWidgetItem(formatWeight(r.weight));
        weightItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        weightItem->setData(Qt::UserRole, r.weight);
        table->setItem(i, 3, weightItem);

        auto *cumItem = new QTableWidgetItem(formatWeight(cumulative));
        cumItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(i, 4, cumItem);
    }
    table->setFixedHeight(40 + rows.size() * 36 + 40);
    container->addWidget(table);

    // 目标总重输入 + 合计
    auto *footerRow = new QWidget;
    auto *footerLayout = new QHBoxLayout(footerRow);
    footerLayout->setContentsMargins(0, 4, 0, 4);

    auto *targetLabel = new QLabel(QStringLiteral("目标总重："));
    footerLayout->addWidget(targetLabel);

    auto *targetInput = new QLineEdit;
    targetInput->setPlaceholderText(QStringLiteral("按比例换算"));
    targetInput->setMaximumWidth(120);
    connect(targetInput, &QLineEdit::textChanged, this, [this, table](const QString &text) {
        recalcLayerWeights(table, text);
    });
    footerLayout->addWidget(targetInput);

    auto *unitLabel = new QLabel(QStringLiteral("g"));
    footerLayout->addWidget(unitLabel);
    footerLayout->addStretch();

    auto *sumLabel = new QLabel(QStringLiteral("合计：%1 g").arg(formatWeight(sum)));
    sumLabel->setStyleSheet(QStringLiteral("font-weight: 600;"));
    footerLayout->addWidget(sumLabel);

    container->addWidget(footerRow);
}

void FormulaDetailWidget::recalcLayerWeights(QTableWidget *table, const QString &targetVal)
{
    double target = targetVal.toDouble();
    int rowCount = table->rowCount();

    // 计算原始总和
    double sum = 0;
    for (int i = 0; i < rowCount; ++i) {
        auto *item = table->item(i, 3);
        sum += item ? item->data(Qt::UserRole).toDouble() : 0;
    }

    if (target <= 0 || sum <= 0) {
        // 恢复原始克重
        double cum = 0;
        for (int i = 0; i < rowCount; ++i) {
            double w = table->item(i, 3)->data(Qt::UserRole).toDouble();
            cum += w;
            table->item(i, 3)->setText(formatWeight(w));
            table->item(i, 4)->setText(formatWeight(cum));
        }
        return;
    }

    // 按比例换算
    double cumCalc = 0;
    for (int i = 0; i < rowCount; ++i) {
        double w = table->item(i, 3)->data(Qt::UserRole).toDouble();
        double nw = (w / sum) * target;
        cumCalc += nw;
        table->item(i, 3)->setText(formatWeight(nw));
        table->item(i, 4)->setText(formatWeight(cumCalc));
    }
}

void FormulaDetailWidget::downloadLayerFormula(int layer)
{
    // 收集该层色母数据
    // 找到对应的 table widget
    auto tables = m_contentWidget->findChildren<QTableWidget *>();
    QTableWidget *targetTable = nullptr;
    int layerIdx = 0;
    for (auto *t : tables) {
        layerIdx++;
        if (layerIdx == layer) {
            targetTable = t;
            break;
        }
    }
    if (!targetTable) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("未找到该层色母数据"));
        return;
    }

    QList<ExportItem> items;
    for (int i = 0; i < targetTable->rowCount(); ++i) {
        QString code = targetTable->item(i, 1)->text();
        double weight = targetTable->item(i, 3)->text().toDouble();
        if (!code.isEmpty() && weight > 0)
            items.append({code, weight});
    }
    if (items.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("该层没有可下载的色母"));
        return;
    }

    // 构建 meta
    QJsonObject meta;
    meta[QStringLiteral("brand")]         = m_currentFormula.value(QStringLiteral("brandName")).toString();
    meta[QStringLiteral("product")]       = m_currentFormula.value(QStringLiteral("seriesName")).toString();
    meta[QStringLiteral("manufacturer")]  = m_currentFormula.value(QStringLiteral("brandName")).toString();
    meta[QStringLiteral("model")]         = m_currentFormula.value(QStringLiteral("modelName")).toString();
    meta[QStringLiteral("description")]   = m_currentFormula.value(QStringLiteral("formulaName")).toString(
                                            m_currentFormula.value(QStringLiteral("colorName")).toString());
    meta[QStringLiteral("innerColorCode")] = m_currentFormula.value(QStringLiteral("sampleId")).toString();
    meta[QStringLiteral("colorGroup")]    = m_currentFormula.value(QStringLiteral("coatType")).toString();
    meta[QStringLiteral("standardColor")] = m_currentFormula.value(QStringLiteral("colorCode")).toString();
    meta[QStringLiteral("variantCode")]   = m_currentFormula.value(QStringLiteral("colorBias")).toString();

    QString sampleId = m_currentFormula.value(QStringLiteral("sampleId")).toString(
                       m_currentFormula.value(QStringLiteral("colorCode")).toString(
                       QStringLiteral("recipe")));
    double quantity = 0;
    for (const auto &it : items) quantity += it.amount;

    QString layerTag = (layer == 2) ? QStringLiteral("面层") : QStringLiteral("底层");
    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    QString colorCode = m_currentFormula.value(QStringLiteral("colorCode")).toString(sampleId);

    // 选择保存路径
    QString defaultName = QStringLiteral("%1_%2_%3").arg(colorCode, layerTag, timestamp);
    QString filter = QStringLiteral("XML 文件 (*.xml);;DAT 文件 (*.dat);;REZ 文件 (*.rez);;TXT 文件 (*.txt)");
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("保存配方"), defaultName, filter);
    if (filePath.isEmpty()) return;

    // 根据扩展名确定格式
    FormulaExport::Format fmt = FormulaExport::Format::Xml;
    if (filePath.endsWith(QStringLiteral(".dat"), Qt::CaseInsensitive))
        fmt = FormulaExport::Format::Dat;
    else if (filePath.endsWith(QStringLiteral(".rez"), Qt::CaseInsensitive))
        fmt = FormulaExport::Format::Rez;
    else if (filePath.endsWith(QStringLiteral(".txt"), Qt::CaseInsensitive))
        fmt = FormulaExport::Format::Txt;

    ExportOptions opts;
    opts.layerNo = (layer == 2) ? 2 : 1;
    opts.meta = meta;
    opts.items = items;
    opts.quantity = quantity;
    opts.assortmentId = m_currentFormula.value(QStringLiteral("seriesName")).toString(
                        m_currentFormula.value(QStringLiteral("brandName")).toString());
    opts.sampleId = sampleId;

    QString content = FormulaExport::buildContent(fmt, opts);
    if (FormulaExport::saveToFile(filePath, content)) {
        QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("配方已保存"));
    } else {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("文件保存失败"));
    }
}
