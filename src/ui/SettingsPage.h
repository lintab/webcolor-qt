/* ============================================================
   SettingsPage.h —— 设置页面（嵌入主窗口内容区）
   ============================================================ */

#pragma once

#include <QWidget>

class QLineEdit;
class QLabel;

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr);

    /** 获取保存的配方下载路径 */
    static QString formulaSavePath();

    /** 设置配方下载路径 */
    static void setFormulaSavePath(const QString &path);

private:
    void setupUi();
    void browsePath();
    void saveSettings();

    QLineEdit *m_pathEdit;
    QLabel *m_statusLabel;
};
