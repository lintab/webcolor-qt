/* ============================================================
   ColorSearchSettingsPage.h —— 搜色用户配置页面
   配置 S&C 搜色与修色阈值：
   - 启用用户配置
   - 搜色参数（dE2000、纹理容差、Score上限等）
   - 修色阈值
   - 前端显示设置（评分小数位数、颜色阈值）
   ============================================================ */

#pragma once

#include <QWidget>

class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QLabel;

class ColorSearchSettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit ColorSearchSettingsPage(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void loadSettings();
    void saveSettings();

private:
    void setupUi();

    // 启用开关
    QCheckBox *m_enabledCheck;

    // 搜色参数
    QDoubleSpinBox *m_maxDe2000SolidSpin;
    QDoubleSpinBox *m_maxDe2000EffectSpin;
    QDoubleSpinBox *m_maxCoarsenessToleranceSpin;
    QDoubleSpinBox *m_maxMeritScoreSpin;
    QSpinBox *m_maxResultsSpin;
    QDoubleSpinBox *m_correctionThresholdSolidSpin;
    QDoubleSpinBox *m_correctionThresholdEffectSpin;

    // 前端显示设置
    QSpinBox *m_scoreDecimalPlacesSpin;

    // 素色评分颜色阈值
    QDoubleSpinBox *m_solidInitialGreenSpin;
    QDoubleSpinBox *m_solidInitialYellowSpin;
    QDoubleSpinBox *m_solidInitialRedSpin;
    QDoubleSpinBox *m_solidCorrectedGreenSpin;
    QDoubleSpinBox *m_solidCorrectedYellowSpin;
    QDoubleSpinBox *m_solidCorrectedRedSpin;

    // 金属色评分颜色阈值
    QDoubleSpinBox *m_effectInitialGreenSpin;
    QDoubleSpinBox *m_effectInitialYellowSpin;
    QDoubleSpinBox *m_effectInitialRedSpin;
    QDoubleSpinBox *m_effectCorrectedGreenSpin;
    QDoubleSpinBox *m_effectCorrectedYellowSpin;
    QDoubleSpinBox *m_effectCorrectedRedSpin;

    // 按钮
    QPushButton *m_saveBtn;
    QPushButton *m_reloadBtn;

    // 更新时间标签
    QLabel *m_updatedAtLabel;

    bool m_loaded = false;
};
