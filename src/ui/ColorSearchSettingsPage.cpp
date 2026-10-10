/* ============================================================
   ColorSearchSettingsPage.cpp —— 搜色用户配置页面实现
   ============================================================ */

#include "ColorSearchSettingsPage.h"
#include "api/HttpClient.h"
#include "utils/Auth.h"
#include "utils/I18nManager.h"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QShowEvent>
#include <QDebug>

ColorSearchSettingsPage::ColorSearchSettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void ColorSearchSettingsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_loaded) {
        QString token = Auth::instance().getToken();
        if (token.isEmpty()) return;
        loadSettings();
    }
}

// ========== UI 搭建 ==========

void ColorSearchSettingsPage::setupUi()
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(QStringLiteral("QScrollArea { border: none; background: transparent; }"));
    outerLayout->addWidget(scrollArea);

    auto *contentWidget = new QWidget;
    auto *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(12);

    scrollArea->setWidget(contentWidget);

    // 标题和更新时间
    auto *headerLayout = new QHBoxLayout;
    auto *titleLabel = new QLabel(T("csset_title"));
    titleLabel->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold; color: #333;"));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    m_updatedAtLabel = new QLabel;
    m_updatedAtLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #999;"));
    headerLayout->addWidget(m_updatedAtLabel);
    mainLayout->addLayout(headerLayout);

    // 提示信息
    auto *hintLabel = new QLabel(T("csset_hint"));
    hintLabel->setStyleSheet(QStringLiteral(
        "background: #e6f7ff; border: 1px solid #91d5ff; border-radius: 4px; "
        "padding: 10px 12px; font-size: 12px; color: #666; margin-bottom: 8px;"));
    hintLabel->setWordWrap(true);
    mainLayout->addWidget(hintLabel);

    // ===== 启用开关 =====
    auto *enableGroup = new QGroupBox;
    enableGroup->setStyleSheet(QStringLiteral(
        "QGroupBox { border: 1px solid #e8e8e8; border-radius: 6px; "
        "margin-top: 8px; padding: 12px 16px; }"));
    auto *enableLayout = new QHBoxLayout(enableGroup);
    enableLayout->setContentsMargins(0, 0, 0, 0);
    m_enabledCheck = new QCheckBox(T("csset_enable"));
    m_enabledCheck->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 500;"));
    enableLayout->addWidget(m_enabledCheck);
    enableLayout->addStretch();
    auto *enableTip = new QLabel(T("csset_enable_tip"));
    enableTip->setStyleSheet(QStringLiteral("font-size: 11px; color: #999;"));
    enableLayout->addWidget(enableTip);
    mainLayout->addWidget(enableGroup);

    // ===== 搜色参数 =====
    auto *searchGroup = new QGroupBox(T("csset_search_params"));
    searchGroup->setStyleSheet(QStringLiteral(
        "QGroupBox { font-weight: bold; font-size: 13px; color: #333; "
        "border: 1px solid #e8e8e8; border-radius: 6px; "
        "margin-top: 12px; padding: 16px 16px 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }"));
    auto *searchForm = new QFormLayout(searchGroup);
    searchForm->setHorizontalSpacing(20);
    searchForm->setVerticalSpacing(10);
    searchForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto createDoubleSpin = [](double min, double max, int decimals, double step) {
        auto *spin = new QDoubleSpinBox;
        spin->setRange(min, max);
        spin->setDecimals(decimals);
        spin->setSingleStep(step);
        spin->setStyleSheet(QStringLiteral(
            "QDoubleSpinBox { border: 1px solid #d9d9d9; border-radius: 4px; "
            "padding: 4px 8px; font-size: 13px; min-width: 120px; }"
            "QDoubleSpinBox:focus { border-color: #0d4f8b; }"));
        return spin;
    };

    auto createIntSpin = [](int min, int max) {
        auto *spin = new QSpinBox;
        spin->setRange(min, max);
        spin->setStyleSheet(QStringLiteral(
            "QSpinBox { border: 1px solid #d9d9d9; border-radius: 4px; "
            "padding: 4px 8px; font-size: 13px; min-width: 120px; }"
            "QSpinBox:focus { border-color: #0d4f8b; }"));
        return spin;
    };

    m_maxDe2000SolidSpin = createDoubleSpin(0, 100, 4, 1);
    searchForm->addRow(T("csset_max_de_solid"), m_maxDe2000SolidSpin);
    auto *tip1 = new QLabel(T("csset_max_de_solid_tip"));
    tip1->setStyleSheet(QStringLiteral("font-size: 11px; color: #999;"));
    searchForm->addRow(QString(), tip1);

    m_maxDe2000EffectSpin = createDoubleSpin(0, 100, 4, 1);
    searchForm->addRow(T("csset_max_de_effect"), m_maxDe2000EffectSpin);
    auto *tip2 = new QLabel(T("csset_max_de_effect_tip"));
    tip2->setStyleSheet(QStringLiteral("font-size: 11px; color: #999;"));
    searchForm->addRow(QString(), tip2);

    m_maxCoarsenessToleranceSpin = createDoubleSpin(0, 100, 4, 1);
    searchForm->addRow(T("csset_coarseness_tol"), m_maxCoarsenessToleranceSpin);
    auto *tip3 = new QLabel(T("csset_coarseness_tol_tip"));
    tip3->setStyleSheet(QStringLiteral("font-size: 11px; color: #999;"));
    searchForm->addRow(QString(), tip3);

    m_maxMeritScoreSpin = createDoubleSpin(0, 1000, 4, 1);
    searchForm->addRow(T("csset_max_score"), m_maxMeritScoreSpin);
    auto *tip4 = new QLabel(T("csset_max_score_tip"));
    tip4->setStyleSheet(QStringLiteral("font-size: 11px; color: #999;"));
    searchForm->addRow(QString(), tip4);

    m_maxResultsSpin = createIntSpin(1, 1000);
    searchForm->addRow(T("csset_max_results"), m_maxResultsSpin);
    auto *tip5 = new QLabel(T("csset_max_results_tip"));
    tip5->setStyleSheet(QStringLiteral("font-size: 11px; color: #999;"));
    searchForm->addRow(QString(), tip5);

    m_correctionThresholdSolidSpin = createDoubleSpin(0, 100, 4, 1);
    searchForm->addRow(T("csset_corr_threshold_solid"), m_correctionThresholdSolidSpin);
    auto *tip6 = new QLabel(T("csset_corr_threshold_solid_tip"));
    tip6->setStyleSheet(QStringLiteral("font-size: 11px; color: #999;"));
    searchForm->addRow(QString(), tip6);

    m_correctionThresholdEffectSpin = createDoubleSpin(0, 100, 4, 1);
    searchForm->addRow(T("csset_corr_threshold_effect"), m_correctionThresholdEffectSpin);
    auto *tip7 = new QLabel(T("csset_corr_threshold_effect_tip"));
    tip7->setStyleSheet(QStringLiteral("font-size: 11px; color: #999;"));
    searchForm->addRow(QString(), tip7);

    mainLayout->addWidget(searchGroup);

    // ===== 前端显示设置 =====
    auto *displayGroup = new QGroupBox(T("csset_display"));
    displayGroup->setStyleSheet(searchGroup->styleSheet());
    auto *displayForm = new QFormLayout(displayGroup);
    displayForm->setHorizontalSpacing(20);
    displayForm->setVerticalSpacing(10);
    displayForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_scoreDecimalPlacesSpin = createIntSpin(0, 6);
    displayForm->addRow(T("csset_score_decimals"), m_scoreDecimalPlacesSpin);
    auto *tip8 = new QLabel(T("csset_score_decimals_tip"));
    tip8->setStyleSheet(QStringLiteral("font-size: 11px; color: #999;"));
    displayForm->addRow(QString(), tip8);

    mainLayout->addWidget(displayGroup);

    // ===== 素色评分颜色阈值 =====
    auto *solidGroup = new QGroupBox(T("csset_solid_score_color"));
    solidGroup->setStyleSheet(searchGroup->styleSheet());
    auto *solidLayout = new QVBoxLayout(solidGroup);

    auto *solidInitialLabel = new QLabel(T("csset_initial_score"));
    solidInitialLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #666; margin-top: 4px;"));
    solidLayout->addWidget(solidInitialLabel);

    auto *solidInitialLayout = new QHBoxLayout;
    solidInitialLayout->setSpacing(12);
    auto *solidInitialGreenLabel = new QLabel(T("csset_green_max"));
    solidInitialGreenLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
    solidInitialLayout->addWidget(solidInitialGreenLabel);
    m_solidInitialGreenSpin = createDoubleSpin(0, 100, 4, 0.1);
    solidInitialLayout->addWidget(m_solidInitialGreenSpin);
    solidInitialLayout->addSpacing(16);
    auto *solidInitialYellowLabel = new QLabel(T("csset_yellow_max"));
    solidInitialYellowLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
    solidInitialLayout->addWidget(solidInitialYellowLabel);
    m_solidInitialYellowSpin = createDoubleSpin(0, 100, 4, 0.1);
    solidInitialLayout->addWidget(m_solidInitialYellowSpin);
    solidInitialLayout->addSpacing(16);
    auto *solidInitialRedLabel = new QLabel(T("csset_red_max"));
    solidInitialRedLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
    solidInitialLayout->addWidget(solidInitialRedLabel);
    m_solidInitialRedSpin = createDoubleSpin(0, 100, 4, 0.1);
    solidInitialLayout->addWidget(m_solidInitialRedSpin);
    solidInitialLayout->addStretch();
    solidLayout->addLayout(solidInitialLayout);

    auto *solidCorrectedLabel = new QLabel(T("csset_corrected_score"));
    solidCorrectedLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #666; margin-top: 8px;"));
    solidLayout->addWidget(solidCorrectedLabel);

    auto *solidCorrectedLayout = new QHBoxLayout;
    solidCorrectedLayout->setSpacing(12);
    auto *solidCorrectedGreenLabel = new QLabel(T("csset_green_max"));
    solidCorrectedGreenLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
    solidCorrectedLayout->addWidget(solidCorrectedGreenLabel);
    m_solidCorrectedGreenSpin = createDoubleSpin(0, 100, 4, 0.1);
    solidCorrectedLayout->addWidget(m_solidCorrectedGreenSpin);
    solidCorrectedLayout->addSpacing(16);
    auto *solidCorrectedYellowLabel = new QLabel(T("csset_yellow_max"));
    solidCorrectedYellowLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
    solidCorrectedLayout->addWidget(solidCorrectedYellowLabel);
    m_solidCorrectedYellowSpin = createDoubleSpin(0, 100, 4, 0.1);
    solidCorrectedLayout->addWidget(m_solidCorrectedYellowSpin);
    solidCorrectedLayout->addSpacing(16);
    auto *solidCorrectedRedLabel = new QLabel(T("csset_red_max"));
    solidCorrectedRedLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
    solidCorrectedLayout->addWidget(solidCorrectedRedLabel);
    m_solidCorrectedRedSpin = createDoubleSpin(0, 100, 4, 0.1);
    solidCorrectedLayout->addWidget(m_solidCorrectedRedSpin);
    solidCorrectedLayout->addStretch();
    solidLayout->addLayout(solidCorrectedLayout);

    mainLayout->addWidget(solidGroup);

    // ===== 金属色评分颜色阈值 =====
    auto *effectGroup = new QGroupBox(T("csset_effect_score_color"));
    effectGroup->setStyleSheet(searchGroup->styleSheet());
    auto *effectLayout = new QVBoxLayout(effectGroup);

    auto *effectInitialLabel = new QLabel(T("csset_initial_score"));
    effectInitialLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #666; margin-top: 4px;"));
    effectLayout->addWidget(effectInitialLabel);

    auto *effectInitialLayout = new QHBoxLayout;
    effectInitialLayout->setSpacing(12);
    auto *effectInitialGreenLabel = new QLabel(T("csset_green_max"));
    effectInitialGreenLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
    effectInitialLayout->addWidget(effectInitialGreenLabel);
    m_effectInitialGreenSpin = createDoubleSpin(0, 100, 4, 0.1);
    effectInitialLayout->addWidget(m_effectInitialGreenSpin);
    effectInitialLayout->addSpacing(16);
    auto *effectInitialYellowLabel = new QLabel(T("csset_yellow_max"));
    effectInitialYellowLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
    effectInitialLayout->addWidget(effectInitialYellowLabel);
    m_effectInitialYellowSpin = createDoubleSpin(0, 100, 4, 0.1);
    effectInitialLayout->addWidget(m_effectInitialYellowSpin);
    effectInitialLayout->addSpacing(16);
    auto *effectInitialRedLabel = new QLabel(T("csset_red_max"));
    effectInitialRedLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
    effectInitialLayout->addWidget(effectInitialRedLabel);
    m_effectInitialRedSpin = createDoubleSpin(0, 100, 4, 0.1);
    effectInitialLayout->addWidget(m_effectInitialRedSpin);
    effectInitialLayout->addStretch();
    effectLayout->addLayout(effectInitialLayout);

    auto *effectCorrectedLabel = new QLabel(T("csset_corrected_score"));
    effectCorrectedLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #666; margin-top: 8px;"));
    effectLayout->addWidget(effectCorrectedLabel);

    auto *effectCorrectedLayout = new QHBoxLayout;
    effectCorrectedLayout->setSpacing(12);
    auto *effectCorrectedGreenLabel = new QLabel(T("csset_green_max"));
    effectCorrectedGreenLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
    effectCorrectedLayout->addWidget(effectCorrectedGreenLabel);
    m_effectCorrectedGreenSpin = createDoubleSpin(0, 100, 4, 0.1);
    effectCorrectedLayout->addWidget(m_effectCorrectedGreenSpin);
    effectCorrectedLayout->addSpacing(16);
    auto *effectCorrectedYellowLabel = new QLabel(T("csset_yellow_max"));
    effectCorrectedYellowLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
    effectCorrectedLayout->addWidget(effectCorrectedYellowLabel);
    m_effectCorrectedYellowSpin = createDoubleSpin(0, 100, 4, 0.1);
    effectCorrectedLayout->addWidget(m_effectCorrectedYellowSpin);
    effectCorrectedLayout->addSpacing(16);
    auto *effectCorrectedRedLabel = new QLabel(T("csset_red_max"));
    effectCorrectedRedLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
    effectCorrectedLayout->addWidget(effectCorrectedRedLabel);
    m_effectCorrectedRedSpin = createDoubleSpin(0, 100, 4, 0.1);
    effectCorrectedLayout->addWidget(m_effectCorrectedRedSpin);
    effectCorrectedLayout->addStretch();
    effectLayout->addLayout(effectCorrectedLayout);

    auto *effectTip = new QLabel(T("csset_score_color_tip"));
    effectTip->setStyleSheet(QStringLiteral("font-size: 11px; color: #999; margin-top: 4px;"));
    effectLayout->addWidget(effectTip);

    mainLayout->addWidget(effectGroup);

    // ===== 按钮 =====
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    m_reloadBtn = new QPushButton(T("csset_reload"));
    m_reloadBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border: 1px solid #d9d9d9; border-radius: 4px; padding: 8px 20px; "
        "font-size: 13px; background: #fff; color: #333; }"
        "QPushButton:hover { border-color: #0d4f8b; color: #0d4f8b; }"));
    btnLayout->addWidget(m_reloadBtn);
    m_saveBtn = new QPushButton(T("csset_save_config"));
    m_saveBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border: none; border-radius: 4px; padding: 8px 24px; "
        "font-size: 13px; background: #0d4f8b; color: #fff; font-weight: 500; }"
        "QPushButton:hover { background: #1a6ec9; }"));
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);

    mainLayout->addStretch();

    // 信号连接
    connect(m_saveBtn, &QPushButton::clicked, this, &ColorSearchSettingsPage::saveSettings);
    connect(m_reloadBtn, &QPushButton::clicked, this, &ColorSearchSettingsPage::loadSettings);
}

// ========== 加载设置 ==========

void ColorSearchSettingsPage::loadSettings()
{
    HttpClient::instance().get(QStringLiteral("/colorcr/sc/settings/mine"), QJsonObject{},
        [this](const QJsonObject &res) {
            QJsonObject data = res.value(QStringLiteral("data")).toObject();
            if (data.isEmpty()) {
                m_loaded = true;
                return;
            }

            m_enabledCheck->setChecked(data.value(QStringLiteral("enabled")).toBool());

            auto getDouble = [&data](const QString &key, QDoubleSpinBox *spin) {
                if (data.contains(key)) {
                    spin->setValue(data.value(key).toDouble());
                }
            };
            auto getInt = [&data](const QString &key, QSpinBox *spin) {
                if (data.contains(key)) {
                    spin->setValue(data.value(key).toInt());
                }
            };

            getDouble(QStringLiteral("maxDe2000Solid"), m_maxDe2000SolidSpin);
            getDouble(QStringLiteral("maxDe2000Effect"), m_maxDe2000EffectSpin);
            getDouble(QStringLiteral("maxCoarsenessTolerance"), m_maxCoarsenessToleranceSpin);
            getDouble(QStringLiteral("maxMeritScore"), m_maxMeritScoreSpin);
            getInt(QStringLiteral("maxResults"), m_maxResultsSpin);
            getDouble(QStringLiteral("correctionThresholdSolid"), m_correctionThresholdSolidSpin);
            getDouble(QStringLiteral("correctionThresholdEffect"), m_correctionThresholdEffectSpin);
            getInt(QStringLiteral("scoreDecimalPlaces"), m_scoreDecimalPlacesSpin);

            getDouble(QStringLiteral("solidInitialGreen"), m_solidInitialGreenSpin);
            getDouble(QStringLiteral("solidInitialYellow"), m_solidInitialYellowSpin);
            getDouble(QStringLiteral("solidInitialRed"), m_solidInitialRedSpin);
            getDouble(QStringLiteral("solidCorrectedGreen"), m_solidCorrectedGreenSpin);
            getDouble(QStringLiteral("solidCorrectedYellow"), m_solidCorrectedYellowSpin);
            getDouble(QStringLiteral("solidCorrectedRed"), m_solidCorrectedRedSpin);

            getDouble(QStringLiteral("effectInitialGreen"), m_effectInitialGreenSpin);
            getDouble(QStringLiteral("effectInitialYellow"), m_effectInitialYellowSpin);
            getDouble(QStringLiteral("effectInitialRed"), m_effectInitialRedSpin);
            getDouble(QStringLiteral("effectCorrectedGreen"), m_effectCorrectedGreenSpin);
            getDouble(QStringLiteral("effectCorrectedYellow"), m_effectCorrectedYellowSpin);
            getDouble(QStringLiteral("effectCorrectedRed"), m_effectCorrectedRedSpin);

            QString updatedAt = data.value(QStringLiteral("updatedAt")).toString();
            if (!updatedAt.isEmpty()) {
                m_updatedAtLabel->setText(T("csset_updated_at").arg(updatedAt));
            }

            m_loaded = true;
        },
        [](const QString &errMsg, int) {
            QMessageBox::warning(nullptr, T("error"),
                                 T("csset_load_failed").arg(errMsg));
        }
    );
}

// ========== 保存设置 ==========

void ColorSearchSettingsPage::saveSettings()
{
    QJsonObject payload;
    payload[QStringLiteral("enabled")] = m_enabledCheck->isChecked();
    payload[QStringLiteral("maxDe2000Solid")] = m_maxDe2000SolidSpin->value();
    payload[QStringLiteral("maxDe2000Effect")] = m_maxDe2000EffectSpin->value();
    payload[QStringLiteral("maxCoarsenessTolerance")] = m_maxCoarsenessToleranceSpin->value();
    payload[QStringLiteral("maxMeritScore")] = m_maxMeritScoreSpin->value();
    payload[QStringLiteral("maxResults")] = m_maxResultsSpin->value();
    payload[QStringLiteral("correctionThresholdSolid")] = m_correctionThresholdSolidSpin->value();
    payload[QStringLiteral("correctionThresholdEffect")] = m_correctionThresholdEffectSpin->value();
    payload[QStringLiteral("scoreDecimalPlaces")] = m_scoreDecimalPlacesSpin->value();

    payload[QStringLiteral("solidInitialGreen")] = m_solidInitialGreenSpin->value();
    payload[QStringLiteral("solidInitialYellow")] = m_solidInitialYellowSpin->value();
    payload[QStringLiteral("solidInitialRed")] = m_solidInitialRedSpin->value();
    payload[QStringLiteral("solidCorrectedGreen")] = m_solidCorrectedGreenSpin->value();
    payload[QStringLiteral("solidCorrectedYellow")] = m_solidCorrectedYellowSpin->value();
    payload[QStringLiteral("solidCorrectedRed")] = m_solidCorrectedRedSpin->value();

    payload[QStringLiteral("effectInitialGreen")] = m_effectInitialGreenSpin->value();
    payload[QStringLiteral("effectInitialYellow")] = m_effectInitialYellowSpin->value();
    payload[QStringLiteral("effectInitialRed")] = m_effectInitialRedSpin->value();
    payload[QStringLiteral("effectCorrectedGreen")] = m_effectCorrectedGreenSpin->value();
    payload[QStringLiteral("effectCorrectedYellow")] = m_effectCorrectedYellowSpin->value();
    payload[QStringLiteral("effectCorrectedRed")] = m_effectCorrectedRedSpin->value();

    HttpClient::instance().put(QStringLiteral("/colorcr/sc/settings/mine"), payload,
        [this](const QJsonObject &res) {
            QMessageBox::information(nullptr, T("success"), T("csset_save_success"));
            QJsonObject data = res.value(QStringLiteral("data")).toObject();
            if (!data.isEmpty()) {
                QString updatedAt = data.value(QStringLiteral("updatedAt")).toString();
                if (!updatedAt.isEmpty()) {
                    m_updatedAtLabel->setText(T("csset_updated_at").arg(updatedAt));
                }
            }
        },
        [](const QString &errMsg, int) {
            QMessageBox::warning(nullptr, T("error"),
                                 T("csset_save_failed").arg(errMsg));
        }
    );
}
