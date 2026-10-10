/* ============================================================
   UpdateWidget.h —— 软件更新页面（独立窗口）
   检查后端 Windows 更新推送，下载安装包后提示运行
   ============================================================ */

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;
class QFile;

class UpdateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UpdateWidget(QWidget *parent = nullptr);
    ~UpdateWidget() override;

private:
    void setupUi();
    void checkForUpdates();
    void startDownload();
    void launchInstaller();
    void abortDownload();
    void setStatus(const QString &text, const QString &color);
    void setBusy(bool busy);

    QLabel *m_iconLabel;
    QLabel *m_titleLabel;
    QLabel *m_versionLabel;
    QLabel *m_statusLabel;
    QLabel *m_noteLabel;
    QProgressBar *m_progressBar;
    QPushButton *m_checkBtn;
    QPushButton *m_updateBtn;
    QPushButton *m_closeBtn;

    QString m_latestVersion;
    QString m_downloadUrl;
    QString m_releaseNote;

    QNetworkAccessManager *m_dlMgr = nullptr;
    QNetworkReply *m_dlReply = nullptr;
    QFile *m_dlFile = nullptr;
    QString m_installerPath;
};
