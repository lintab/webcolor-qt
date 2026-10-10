/* ============================================================
   SettingsPage.cpp —— 设置页面实现
   ============================================================ */

#include "SettingsPage.h"
#include "utils/I18nManager.h"
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QSettings>
#include <QDir>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QDebug>

static const QString SETTINGS_KEY_SAVE_PATH = QStringLiteral("FormulaSavePath");

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

QString SettingsPage::formulaSavePath()
{
    QSettings settings;
    QString path = settings.value(SETTINGS_KEY_SAVE_PATH).toString();
    if (path.isEmpty()) {
        // 默认使用桌面
        path = QDir::homePath() + QStringLiteral("/Desktop");
    }
    return path;
}

void SettingsPage::setFormulaSavePath(const QString &path)
{
    QSettings settings;
    settings.setValue(SETTINGS_KEY_SAVE_PATH, path);
}

void SettingsPage::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // 页面标题
    auto *titleLabel = new QLabel(T("formula_download_settings"));
    titleLabel->setStyleSheet(QStringLiteral(
        "font-size: 20px; font-weight: bold; color: #1a2332;"));
    mainLayout->addWidget(titleLabel);

    // 配方下载路径设置
    auto *pathGroup = new QGroupBox(T("formula_save_path_title"));
    pathGroup->setStyleSheet(QStringLiteral(
        "QGroupBox { font-weight: bold; font-size: 14px; color: #1a2332; "
        "border: 1px solid #e0e3e8; border-radius: 6px; margin-top: 12px; padding: 20px 16px 16px 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }"));
    auto *groupLayout = new QVBoxLayout(pathGroup);
    groupLayout->setSpacing(12);

    auto *descLabel = new QLabel(T("save_path_desc"));
    descLabel->setStyleSheet(QStringLiteral("color: #666; font-size: 13px; font-weight: normal;"));
    descLabel->setWordWrap(true);
    groupLayout->addWidget(descLabel);

    auto *pathLayout = new QHBoxLayout;
    pathLayout->setSpacing(10);

    m_pathEdit = new QLineEdit;
    m_pathEdit->setPlaceholderText(T("select_path_placeholder"));
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { padding: 10px 14px; border: 1px solid #d0d5dd; border-radius: 6px; "
        "background: #f9fafb; font-size: 13px; color: #333; }"));
    m_pathEdit->setText(formulaSavePath());
    pathLayout->addWidget(m_pathEdit);

    auto *browseBtn = new QPushButton(T("browse_dots"));
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setStyleSheet(QStringLiteral(
        "QPushButton { padding: 10px 20px; background: #0d4f8b; color: #fff; "
        "border: none; border-radius: 6px; font-size: 13px; font-weight: 500; }"
        "QPushButton:hover { background: #0a3f70; }"));
    connect(browseBtn, &QPushButton::clicked, this, &SettingsPage::browsePath);
    pathLayout->addWidget(browseBtn);

    groupLayout->addLayout(pathLayout);

    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet(QStringLiteral("color: #28a745; font-size: 12px; font-weight: normal;"));
    m_statusLabel->hide();
    groupLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(pathGroup);

    // 保存按钮
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();

    auto *saveBtn = new QPushButton(T("save_settings_btn"));
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setStyleSheet(QStringLiteral(
        "QPushButton { padding: 10px 28px; background: #0d4f8b; color: #fff; "
        "border: none; border-radius: 6px; font-size: 14px; font-weight: 500; }"
        "QPushButton:hover { background: #0a3f70; }"));
    connect(saveBtn, &QPushButton::clicked, this, &SettingsPage::saveSettings);
    btnLayout->addWidget(saveBtn);

    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch();
}

void SettingsPage::browsePath()
{
    qDebug() << "browsePath called!";
    QString current = m_pathEdit->text();
    if (current.isEmpty()) {
        current = QDir::homePath();
    }
    qDebug() << "Current path:" << current;
    
    // 使用延时确保菜单完全关闭后再打开对话框
    QTimer::singleShot(300, this, [this, current]() {
        qDebug() << "Timer fired, showing dialog...";
        // 使用非原生对话框避免 macOS 菜单冲突
        QString dir = QFileDialog::getExistingDirectory(
            this, 
            T("select_path_dialog"),
            current,
            QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog
        );
        if (!dir.isEmpty()) {
            m_pathEdit->setText(dir);
            m_statusLabel->hide();
            qDebug() << "Selected:" << dir;
        } else {
            qDebug() << "Dialog cancelled or empty";
        }
    });
}

void SettingsPage::saveSettings()
{
    QString path = m_pathEdit->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, T("info"), T("select_path_warn"));
        return;
    }

    // 检查路径是否存在
    QDir dir(path);
    if (!dir.exists()) {
        QMessageBox::warning(this, T("info"), T("path_not_exist"));
        return;
    }

    setFormulaSavePath(path);

    m_statusLabel->setText(T("settings_saved_check"));
    m_statusLabel->show();

    // 3秒后隐藏提示
    QTimer::singleShot(3000, m_statusLabel, [this]() {
        m_statusLabel->hide();
    });
}
