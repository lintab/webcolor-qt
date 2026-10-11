/* ============================================================
   UpdateWidget.cpp —— 软件更新页面（独立窗口）
   后端 win 通道（/colorcr/app/version/latest 的 win 字段）发布
   Windows 版本号与安装包地址，本机比对后下载并提示运行。
   ============================================================ */

#include "UpdateWidget.h"
#include "api/HttpClient.h"
#include "utils/I18nManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QPointer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QUrl>

namespace {

const char *const COLOR_IDLE = "#909399";
const char *const COLOR_INFO = "#409eff";
const char *const COLOR_OK = "#67c23a";
const char *const COLOR_WARN = "#e6a23c";
const char *const COLOR_FAIL = "#f56c6c";

/** 取开头的数字段，"1.0.7-beta" 里的 7 参与比较，非数字尾巴忽略 */
int numPrefix(const QString &part)
{
    int end = 0;
    while (end < part.size() && part.at(end).isDigit()) ++end;
    return end == 0 ? 0 : part.left(end).toInt();
}

/** 按点分段逐段比数值，返回 >0 表示 latest 比 current 新 */
int compareVersions(const QString &latest, const QString &current)
{
    const QStringList a = latest.split(QLatin1Char('.'));
    const QStringList b = current.split(QLatin1Char('.'));
    const int n = qMax(a.size(), b.size());
    for (int i = 0; i < n; ++i) {
        const int va = i < a.size() ? numPrefix(a.at(i)) : 0;
        const int vb = i < b.size() ? numPrefix(b.at(i)) : 0;
        if (va != vb) return va > vb ? 1 : -1;
    }
    return 0;
}

QString localVersion()
{
    const QString v = QApplication::applicationVersion();
    return v.isEmpty() ? QStringLiteral("0") : v;
}

} // namespace

UpdateWidget::UpdateWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

UpdateWidget::~UpdateWidget()
{
    abortDownload();
}

void UpdateWidget::setupUi()
{
    setWindowTitle(T("software_update"));
    setFixedSize(500, 400);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    // 图标
    m_iconLabel = new QLabel(QStringLiteral("🔄"));
    m_iconLabel->setStyleSheet(QStringLiteral("font-size: 60px;"));
    m_iconLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_iconLabel);

    // 标题
    m_titleLabel = new QLabel(T("app_title"));
    m_titleLabel->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: bold; color: #1a2332;"));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    // 版本号
    m_versionLabel = new QLabel(T("current_version") + localVersion());
    m_versionLabel->setStyleSheet(QStringLiteral("font-size: 14px; color: #606266;"));
    m_versionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_versionLabel);

    mainLayout->addSpacing(10);

    // 状态标签
    m_statusLabel = new QLabel(T("check_update_status"));
    m_statusLabel->setStyleSheet(QString::fromLatin1("font-size: 13px; color: %1;").arg(COLOR_IDLE));
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    // 更新说明
    m_noteLabel = new QLabel;
    m_noteLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #606266;"));
    m_noteLabel->setAlignment(Qt::AlignCenter);
    m_noteLabel->setWordWrap(true);
    m_noteLabel->setVisible(false);
    mainLayout->addWidget(m_noteLabel);

    // 进度条（初始隐藏）
    m_progressBar = new QProgressBar;
    m_progressBar->setVisible(false);
    m_progressBar->setStyleSheet(QStringLiteral(
        "QProgressBar { border: 1px solid #dcdfe6; border-radius: 4px; text-align: center; }"
        "QProgressBar::chunk { background: #409eff; border-radius: 3px; }"));
    mainLayout->addWidget(m_progressBar);

    mainLayout->addStretch();

    // 按钮区域
    auto *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(12);

    m_checkBtn = new QPushButton(T("check_update_btn"));
    m_checkBtn->setFixedSize(120, 36);
    m_checkBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #409eff; color: white; border: none; border-radius: 4px; font-size: 14px; }"
        "QPushButton:hover { background: #66b1ff; }"
        "QPushButton:pressed { background: #3a8ee6; }"));
    btnLayout->addWidget(m_checkBtn);

    m_updateBtn = new QPushButton(T("update_now_btn"));
    m_updateBtn->setFixedSize(120, 36);
    m_updateBtn->setEnabled(false);
    m_updateBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #67c23a; color: white; border: none; border-radius: 4px; font-size: 14px; }"
        "QPushButton:hover { background: #85ce61; }"
        "QPushButton:pressed { background: #5daf34; }"
        "QPushButton:disabled { background: #c0c4cc; }"));
    btnLayout->addWidget(m_updateBtn);

    m_closeBtn = new QPushButton(T("close_btn"));
    m_closeBtn->setFixedSize(120, 36);
    m_closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #f0f2f5; color: #606266; border: 1px solid #dcdfe6; border-radius: 4px; font-size: 14px; }"
        "QPushButton:hover { background: #e4e7ed; }"
        "QPushButton:pressed { background: #dcdfe6; }"));
    btnLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(btnLayout);

    m_dlMgr = new QNetworkAccessManager(this);

    connect(m_checkBtn, &QPushButton::clicked, this, &UpdateWidget::checkForUpdates);
    connect(m_updateBtn, &QPushButton::clicked, this, [this]() {
        if (m_installerPath.isEmpty()) {
            startDownload();
        } else {
            launchInstaller();
        }
    });
    connect(m_closeBtn, &QPushButton::clicked, this, &QWidget::close);
}

void UpdateWidget::setStatus(const QString &text, const QString &color)
{
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(QString::fromLatin1("font-size: 13px; color: %1;").arg(color));
}

void UpdateWidget::setBusy(bool busy)
{
    m_checkBtn->setEnabled(!busy);
    m_updateBtn->setEnabled(!busy && !m_latestVersion.isEmpty());
}

void UpdateWidget::checkForUpdates()
{
    if (m_dlReply) return;

    setBusy(true);
    m_noteLabel->clear();
    m_noteLabel->setVisible(false);
    setStatus(T("checking_update"), COLOR_INFO);

    QPointer<UpdateWidget> self(this);
    HttpClient::instance().get(QStringLiteral("/colorcr/app/version/latest"), QJsonObject{},
        [self](const QJsonObject &body) {
            if (!self) return;
            const QJsonObject win = body.value(QStringLiteral("data")).toObject()
                                        .value(QStringLiteral("win")).toObject();
            self->m_latestVersion = win.value(QStringLiteral("version")).toString().trimmed();
            self->m_downloadUrl = win.value(QStringLiteral("storeUrl")).toString().trimmed();
            self->m_releaseNote = win.value(QStringLiteral("remark")).toString().trimmed();

            if (self->m_latestVersion.isEmpty()) {
                self->m_updateBtn->setEnabled(false);
                self->setStatus(T("upd_no_release"), COLOR_IDLE);
                return;
            }
            const QString current = localVersion();
            if (compareVersions(self->m_latestVersion, current) <= 0) {
                self->m_updateBtn->setEnabled(false);
                self->setStatus(T("latest_version_msg") + QStringLiteral(" (v") + current + QLatin1Char(')'),
                                COLOR_OK);
                return;
            }
            self->setStatus(T("upd_found") + self->m_latestVersion, COLOR_WARN);
            if (!self->m_releaseNote.isEmpty()) {
                self->m_noteLabel->setText(T("upd_release_note") + self->m_releaseNote);
                self->m_noteLabel->setVisible(true);
            }
            self->m_checkBtn->setEnabled(true);
            self->m_updateBtn->setEnabled(true);
        },
        [self](const QString &errMsg, int) {
            if (!self) return;
            self->m_checkBtn->setEnabled(true);
            self->setStatus(T("upd_check_failed") + errMsg, COLOR_FAIL);
        });
}

void UpdateWidget::startDownload()
{
    const QUrl target(HttpClient::resourceUrl(m_downloadUrl));
    const QString scheme = target.scheme().toLower();
    if (!target.isValid() || (scheme != QLatin1String("http") && scheme != QLatin1String("https"))) {
        setStatus(T("upd_bad_url"), COLOR_FAIL);
        return;
    }

    QString fileName = target.path().section(QLatin1Char('/'), -1);
    if (!fileName.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive)) {
        fileName = QStringLiteral("KQCloudSetup.exe");
    }

    QDir dir(QDir::tempPath());
    if (!dir.mkpath(QStringLiteral("WebColorUpdate"))) {
        setStatus(T("upd_save_failed") + dir.path(), COLOR_FAIL);
        return;
    }
    m_installerPath = dir.filePath(QStringLiteral("WebColorUpdate/") + fileName);
    QFile::remove(m_installerPath);

    m_dlFile = new QFile(m_installerPath);
    if (!m_dlFile->open(QIODevice::WriteOnly)) {
        setStatus(T("upd_save_failed") + m_dlFile->errorString(), COLOR_FAIL);
        m_dlFile->deleteLater();
        m_dlFile = nullptr;
        m_installerPath.clear();
        return;
    }

    m_updateBtn->setEnabled(false);
    m_checkBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0);
    setStatus(T("upd_downloading"), COLOR_INFO);

    m_dlReply = m_dlMgr->get(QNetworkRequest(target));

    QPointer<UpdateWidget> self(this);
    connect(m_dlReply, &QNetworkReply::downloadProgress, this, [self](qint64 received, qint64 total) {
        if (!self) return;
        if (total > 0) {
            self->m_progressBar->setRange(0, 100);
            self->m_progressBar->setValue(static_cast<int>(received * 100 / total));
        } else {
            self->m_progressBar->setRange(0, 0);
        }
    });

    connect(m_dlReply, &QNetworkReply::finished, this, [self, target]() {
        if (!self) { return; }
        QNetworkReply *reply = self->m_dlReply;
        self->m_dlReply = nullptr;
        const QNetworkReply::NetworkError err = reply->error();
        const QString errorText = reply->errorString();
        const int httpStatus =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();

        if (self->m_dlFile) {
            self->m_dlFile->close();
            self->m_dlFile->deleteLater();
            self->m_dlFile = nullptr;
        }
        self->m_progressBar->setVisible(false);
        self->m_checkBtn->setEnabled(true);

        if (err != QNetworkReply::NoError || httpStatus != 200
            || QFileInfo(self->m_installerPath).size() <= 0) {
            QFile::remove(self->m_installerPath);
            self->m_installerPath.clear();
            self->m_updateBtn->setText(T("update_now_btn"));
            self->m_updateBtn->setEnabled(true);
            QString reason = errorText;
            if (reason.isEmpty() && httpStatus > 0) {
                reason = QStringLiteral("HTTP %1").arg(httpStatus);
            }
            self->setStatus(T("upd_download_failed") + reason, COLOR_FAIL);
            return;
        }

        self->m_updateBtn->setText(T("upd_run_btn"));
        self->m_updateBtn->setEnabled(true);
        self->setStatus(T("upd_download_done") + target.fileName(), COLOR_OK);

        QMessageBox box(QMessageBox::Question, T("info"), T("upd_run_question"),
                        QMessageBox::NoButton, self);
        auto *runBtn = box.addButton(T("upd_run_btn"), QMessageBox::AcceptRole);
        box.addButton(T("upd_later_btn"), QMessageBox::RejectRole);
        if (box.exec() == QDialog::Accepted && box.clickedButton() == runBtn) {
            self->launchInstaller();
        } else {
            self->setStatus(T("upd_download_done") + self->m_installerPath, COLOR_OK);
        }
    });
}

void UpdateWidget::launchInstaller()
{
    if (m_installerPath.isEmpty() || !QFileInfo::exists(m_installerPath)) return;

    abortDownload();
    // /KEEPLOGIN：让安装程序跳过「装完清除本机账号密码」那一步（见 setup.iss 的 CurStepChanged），
    // 软件内更新不该把用户从登录态里踢出去；手动双击安装包没这个参数，登录页才会清空。
    QStringList installerArgs;
    installerArgs << QStringLiteral("/KEEPLOGIN");
    if (QProcess::startDetached(m_installerPath, installerArgs,
                                QFileInfo(m_installerPath).absolutePath())) {
        setStatus(T("upd_launching"), COLOR_OK);
        QApplication::quit();
    } else {
        setStatus(T("upd_launch_failed") + m_installerPath, COLOR_FAIL);
    }
}

void UpdateWidget::abortDownload()
{
    if (m_dlReply) {
        m_dlReply->disconnect(this);
        m_dlReply->abort();
        m_dlReply->deleteLater();
        m_dlReply = nullptr;
    }
    if (m_dlFile) {
        m_dlFile->close();
        m_dlFile->deleteLater();
        m_dlFile = nullptr;
    }
    if (m_progressBar) m_progressBar->setVisible(false);
}
