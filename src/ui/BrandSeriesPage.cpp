/* ============================================================
   BrandSeriesPage.cpp —— 品牌车型搜索页面实现
   左右两栏布局：
   - 左栏（上）：品牌 + 车系联动下拉菜单（宽度可缩小）
   - 左栏（下）：车系对应的车型列表
   - 右栏（上）：车型对应的配方列表
   - 右栏（下）：配方详情
   ============================================================ */

#include "BrandSeriesPage.h"
#include "FormulaDetailWidget.h"
#include "api/FormulaApi.h"
#include "api/HttpClient.h"
#include "utils/Auth.h"
#include "utils/I18nManager.h"
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QShowEvent>
#include <QDebug>

BrandSeriesPage::BrandSeriesPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void BrandSeriesPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_brandsLoaded) {
        QString token = Auth::instance().getToken();
        if (token.isEmpty()) return;
        loadBrands();
    }
}

static QString groupBoxStyle()
{
    return QStringLiteral(
        "QGroupBox { font-weight: bold; font-size: 13px; color: #333; "
        "border: 1px solid #dcdfe6; border-radius: 6px; "
        "margin-top: 8px; padding: 12px 10px 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; "
        "padding: 0 4px; }");
}

// ========== UI 搭建 ==========

void BrandSeriesPage::setupUi()
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ===== 左右分割器 =====
    auto *splitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(splitter);

    // ========== 左栏：品牌车系 + 车型列表 ==========
    auto *leftWidget = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    // 左栏上部：品牌车系联动下拉
    auto *selectGroup = new QGroupBox(T("brand_series_title"));
    selectGroup->setStyleSheet(groupBoxStyle());
    auto *selectLayout = new QVBoxLayout(selectGroup);
    selectLayout->setSpacing(6);

    auto *brandLabel = new QLabel(T("brand_field_brand"));
    brandLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #606266; font-weight: normal;"));
    selectLayout->addWidget(brandLabel);

    m_brandCombo = new QComboBox;
    m_brandCombo->setStyleSheet(QStringLiteral(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px 8px; "
        "font-size: 12px; background: #fff; min-height: 20px; color: #333; }"
        "QComboBox:hover { border-color: #c0c4cc; }"
        "QComboBox QAbstractItemView { border: 1px solid #dcdfe6; background: #fff; "
        "color: #333; selection-background-color: #e8f0fe; selection-color: #0d4f8b; }"));
    m_brandCombo->addItem(T("brand_select"));
    selectLayout->addWidget(m_brandCombo);

    auto *seriesLabel = new QLabel(T("brand_field_series"));
    seriesLabel->setStyleSheet(brandLabel->styleSheet());
    selectLayout->addWidget(seriesLabel);

    m_seriesCombo = new QComboBox;
    m_seriesCombo->setStyleSheet(m_brandCombo->styleSheet());
    m_seriesCombo->addItem(T("series_select"));
    m_seriesCombo->setEnabled(false);
    selectLayout->addWidget(m_seriesCombo);

    leftLayout->addWidget(selectGroup);

    // 左栏下部：车型列表
    auto *modelGroup = new QGroupBox(T("brand_model_list_title"));
    modelGroup->setStyleSheet(groupBoxStyle());
    auto *modelGroupLayout = new QVBoxLayout(modelGroup);
    modelGroupLayout->setContentsMargins(6, 12, 6, 6);

    m_modelHint = new QLabel(T("brand_select_brand_series_first"));
    m_modelHint->setStyleSheet(QStringLiteral("color: #999; font-size: 12px; padding: 12px;"));
    m_modelHint->setAlignment(Qt::AlignCenter);
    modelGroupLayout->addWidget(m_modelHint);

    m_modelList = new QListWidget;
    m_modelList->setStyleSheet(QStringLiteral(
        "QListWidget { border: 1px solid #e4e7ed; border-radius: 4px; background: #fff; }"
        "QListWidget::item { padding: 7px 10px; border-bottom: 1px solid #f0f2f5; font-size: 12px; }"
        "QListWidget::item:hover { background: #f0f5ff; }"
        "QListWidget::item:selected { background: #e8f0fe; border-left: 3px solid #0d4f8b; "
        "color: #0d4f8b; font-weight: bold; }"));
    m_modelList->hide();
    modelGroupLayout->addWidget(m_modelList);

    leftLayout->addWidget(modelGroup, 1);

    // ========== 右侧：配方列表 + 配方详情（左右并排） ==========
    auto *rightSplitter = new QSplitter(Qt::Horizontal);

    // 配方列表（窄）
    auto *formulaGroup = new QGroupBox(T("brand_formula_list_title"));
    formulaGroup->setStyleSheet(groupBoxStyle());
    auto *formulaGroupLayout = new QVBoxLayout(formulaGroup);
    formulaGroupLayout->setContentsMargins(6, 12, 6, 6);

    m_formulaBreadcrumb = new QLabel(T("brand_select_model_for_formulas"));
    m_formulaBreadcrumb->setStyleSheet(QStringLiteral(
        "font-size: 11px; color: #64748b; padding: 2px 0; font-weight: normal;"));
    m_formulaBreadcrumb->setWordWrap(true);
    formulaGroupLayout->addWidget(m_formulaBreadcrumb);

    m_formulaHint = new QLabel(T("brand_formula_hint"));
    m_formulaHint->setStyleSheet(QStringLiteral("color: #999; font-size: 12px; padding: 12px;"));
    m_formulaHint->setAlignment(Qt::AlignCenter);
    formulaGroupLayout->addWidget(m_formulaHint);

    m_formulaList = new QListWidget;
    m_formulaList->setStyleSheet(QStringLiteral(
        "QListWidget { border: 1px solid #e4e7ed; border-radius: 4px; background: #fff; color: #333; }"
        "QListWidget::item { padding: 8px 10px; border-bottom: 1px solid #f0f2f5; font-size: 12px; color: #333; }"
        "QListWidget::item:hover { background: #f0f5ff; }"
        "QListWidget::item:selected { background: #e8f0fe; color: #0d4f8b; }"));
    m_formulaList->hide();
    formulaGroupLayout->addWidget(m_formulaList);

    rightSplitter->addWidget(formulaGroup);

    // 配方详情（宽）
    auto *detailGroup = new QGroupBox(T("formula_detail"));
    detailGroup->setStyleSheet(groupBoxStyle());
    auto *detailGroupLayout = new QVBoxLayout(detailGroup);
    detailGroupLayout->setContentsMargins(6, 12, 6, 6);

    m_detailWidget = new FormulaDetailWidget;
    detailGroupLayout->addWidget(m_detailWidget);

    rightSplitter->addWidget(detailGroup);
    rightSplitter->setStretchFactor(0, 1);
    rightSplitter->setStretchFactor(1, 3);

    // ===== 添加到主分割器 =====
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightSplitter);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 4);

    // ===== 信号连接 =====
    connect(m_brandCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BrandSeriesPage::onBrandChanged);
    connect(m_seriesCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BrandSeriesPage::onSeriesChanged);
    connect(m_modelList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (!item) return;
        ModelItem model;
        model.id = item->data(Qt::UserRole).toString();
        model.name = item->text();
        selectModel(model);
    });
    connect(m_formulaList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (!item) return;
        int formulaId = item->data(Qt::UserRole).toInt();
        showFormulaDetail(formulaId);
    });
}

// ========== 品牌加载 ==========

void BrandSeriesPage::loadBrands()
{
    m_brands.clear();
    m_brandCombo->blockSignals(true);
    m_brandCombo->clear();
    m_brandCombo->addItem(T("brand_select"), QString());
    m_brandCombo->blockSignals(false);

    FormulaApi::instance().getBrands(1, 500,
        [this](const QJsonObject &res) {
            QJsonArray arr = res.value(QStringLiteral("rows")).toArray();
            for (const auto &v : arr) {
                auto obj = v.toObject();
                BrandItem item;
                item.id = obj.value(QStringLiteral("id")).toVariant().toString();
                item.name = Tn(obj, "brandName", "brandNameEn");
                m_brands.append(item);
            }

            m_brandCombo->blockSignals(true);
            for (const auto &b : m_brands) {
                m_brandCombo->addItem(b.name, b.id);
            }
            m_brandCombo->blockSignals(false);
            m_brandsLoaded = true;
        },
        [](const QString &errMsg, int code) {
            qDebug() << "Brand API error:" << errMsg << "code:" << code;
            QMessageBox::warning(nullptr, T("error"),
                                 T("brand_load_brand_failed").arg(errMsg));
        }
    );
}

// ========== 品牌切换 → 加载车系 ==========

void BrandSeriesPage::onBrandChanged(int index)
{
    if (index <= 0) {
        m_seriesCombo->blockSignals(true);
        m_seriesCombo->clear();
        m_seriesCombo->addItem(T("series_select"));
        m_seriesCombo->setEnabled(false);
        m_seriesCombo->blockSignals(false);

        m_currentBrandId.clear();
        m_currentSeriesId.clear();

        m_modelList->clear();
        m_modelList->hide();
        m_modelHint->show();
        m_modelHint->setText(T("brand_select_brand_series_first"));

        m_formulaList->clear();
        m_formulaList->hide();
        m_formulaHint->show();
        m_formulaHint->setText(T("brand_formula_hint"));
        m_formulaBreadcrumb->setText(T("brand_select_model_for_formulas"));
        return;
    }

    QString brandId = m_brandCombo->itemData(index).toString();
    m_currentBrandId = brandId;

    m_modelList->clear();
    m_modelList->hide();
    m_modelHint->show();
    m_modelHint->setText(T("series_select"));
    m_formulaList->clear();
    m_formulaList->hide();
    m_formulaHint->show();
    m_formulaHint->setText(T("brand_formula_hint"));
    m_formulaBreadcrumb->setText(T("brand_select_model_for_formulas"));

    loadSeries(brandId);
}

void BrandSeriesPage::loadSeries(const QString &brandId)
{
    if (m_seriesCache.contains(brandId)) {
        m_seriesCombo->blockSignals(true);
        m_seriesCombo->clear();
        m_seriesCombo->addItem(T("series_select"), QString());
        for (const auto &s : m_seriesCache[brandId]) {
            m_seriesCombo->addItem(s.name, s.id);
        }
        m_seriesCombo->setEnabled(true);
        m_seriesCombo->blockSignals(false);
        return;
    }

    FormulaApi::instance().getSeriesByBrand(brandId,
        [this, brandId](const QJsonObject &res) {
            QJsonArray arr;
            if (res.contains(QStringLiteral("rows")))
                arr = res.value(QStringLiteral("rows")).toArray();
            else if (res.contains(QStringLiteral("data")))
                arr = res.value(QStringLiteral("data")).toArray();

            QList<SeriesItem> seriesList;
            for (const auto &v : arr) {
                auto obj = v.toObject();
                SeriesItem s;
                s.id = obj.value(QStringLiteral("id")).toVariant().toString();
                s.brandId = brandId;
                s.name = Tn(obj, "seriesName", "seriesNameEn");
                if (s.name.isEmpty())
                    s.name = obj.value(QStringLiteral("name")).toString();
                seriesList.append(s);
            }
            m_seriesCache[brandId] = seriesList;

            if (m_currentBrandId == brandId) {
                m_seriesCombo->blockSignals(true);
                m_seriesCombo->clear();
                m_seriesCombo->addItem(T("series_select"), QString());
                for (const auto &s : seriesList) {
                    m_seriesCombo->addItem(s.name, s.id);
                }
                m_seriesCombo->setEnabled(true);
                m_seriesCombo->blockSignals(false);
            }
        },
        [](const QString &errMsg, int) {
            QMessageBox::warning(nullptr, T("error"),
                                 T("brand_load_series_failed").arg(errMsg));
        }
    );
}

// ========== 车系切换 → 加载车型 ==========

void BrandSeriesPage::onSeriesChanged(int index)
{
    if (index <= 0) {
        m_currentSeriesId.clear();
        m_modelList->clear();
        m_modelList->hide();
        m_modelHint->show();
        m_modelHint->setText(T("series_select"));
        m_formulaList->clear();
        m_formulaList->hide();
        m_formulaHint->show();
        m_formulaHint->setText(T("brand_formula_hint"));
        m_formulaBreadcrumb->setText(T("brand_select_model_for_formulas"));
        return;
    }

    m_currentSeriesId = m_seriesCombo->itemData(index).toString();
    m_modelHint->show();
    m_modelHint->setText(T("loading"));
    m_modelList->hide();

    m_formulaList->clear();
    m_formulaList->hide();
    m_formulaHint->show();
    m_formulaHint->setText(T("brand_select_model"));
    m_formulaBreadcrumb->setText(T("brand_select_model"));

    fetchModels();
}

// ========== 车型加载 ==========

void BrandSeriesPage::fetchModels()
{
    if (m_loadingModels || m_currentSeriesId.isEmpty()) return;
    m_loadingModels = true;

    FormulaApi::instance().getModelsBySeries(m_currentSeriesId, 1, 200,
        [this](const QJsonObject &res) {
            m_loadingModels = false;

            QJsonArray arr;
            if (res.contains(QStringLiteral("rows")))
                arr = res.value(QStringLiteral("rows")).toArray();
            else if (res.contains(QStringLiteral("data")))
                arr = res.value(QStringLiteral("data")).toArray();

            m_modelList->clear();

            if (arr.isEmpty()) {
                m_modelHint->setText(T("brand_no_model_data"));
                m_modelHint->show();
                m_modelList->hide();
                return;
            }

            m_modelHint->hide();
            m_modelList->show();

            for (const auto &v : arr) {
                auto obj = v.toObject();
                ModelItem model;
                model.id = obj.value(QStringLiteral("id")).toVariant().toString();
                model.seriesId = m_currentSeriesId;
                model.name = Tn(obj, "modelName", "modelNameEn");
                if (model.name.isEmpty())
                    model.name = obj.value(QStringLiteral("name")).toString();

                auto *item = new QListWidgetItem(model.name);
                item->setData(Qt::UserRole, model.id);
                m_modelList->addItem(item);
            }
        },
        [this](const QString &errMsg, int) {
            m_loadingModels = false;
            m_modelHint->setText(T("brand_load_failed").arg(errMsg));
        }
    );
}

// ========== 车型选择 → 加载配方 ==========

void BrandSeriesPage::selectModel(const ModelItem &model)
{
    m_currentModelId = model.id;
    m_currentModelName = model.name;

    QString brandName = m_brandCombo->currentText();
    QString seriesName = m_seriesCombo->currentText();
    m_formulaBreadcrumb->setText(QStringLiteral("📍 %1 › %2 › %3")
        .arg(brandName, seriesName, model.name));

    m_formulaList->clear();
    m_formulaHint->show();
    m_formulaHint->setText(T("loading"));
    m_formulaList->hide();

    fetchFormulas();
}

// ========== 配方加载 ==========

void BrandSeriesPage::fetchFormulas()
{
    if (m_loadingFormulas || m_currentModelId.isEmpty()) return;
    m_loadingFormulas = true;

    FormulaApi::instance().getFormulasByModel(m_currentModelId, 1, 200,
        [this](const QJsonObject &res) {
            m_loadingFormulas = false;

            QJsonArray arr;
            if (res.contains(QStringLiteral("rows")))
                arr = res.value(QStringLiteral("rows")).toArray();
            else if (res.contains(QStringLiteral("data")))
                arr = res.value(QStringLiteral("data")).toArray();

            m_formulaList->clear();

            if (arr.isEmpty()) {
                m_formulaHint->setText(T("brand_no_formula_data"));
                m_formulaHint->show();
                m_formulaList->hide();
                return;
            }

            m_formulaHint->hide();
            m_formulaList->show();

            for (const auto &v : arr) {
                auto obj = v.toObject();
                QString colorCode = obj.value(QStringLiteral("colorCode")).toString();
                QString colorName = Tn(obj, "colorName", "colorNameEn");
                QString formulaName = obj.value(QStringLiteral("formulaName")).toString();

                QString displayText = colorCode;
                if (!colorName.isEmpty()) displayText += QStringLiteral("  ") + colorName;
                if (!formulaName.isEmpty()) displayText += QStringLiteral("  [") + formulaName + QStringLiteral("]");

                auto *item = new QListWidgetItem(displayText);
                item->setData(Qt::UserRole, obj.value(QStringLiteral("id")).toInt());
                m_formulaList->addItem(item);
            }
        },
        [this](const QString &errMsg, int) {
            m_loadingFormulas = false;
            m_formulaHint->setText(T("brand_load_formula_failed").arg(errMsg));
        }
    );
}

// ========== 配方详情 ==========

void BrandSeriesPage::showFormulaDetail(int formulaId)
{
    if (formulaId <= 0) return;
    m_detailWidget->loadDetail(formulaId);
}
