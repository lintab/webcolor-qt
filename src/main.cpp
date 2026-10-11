/* ============================================================
   main.cpp —— 程序入口
   - 初始化 QApplication，加载全局样式
   - 根据登录状态决定显示登录页或主窗口
   ============================================================ */

#include <QApplication>
#include <QFile>
#include <QString>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include "utils/Auth.h"
#include "ui/LoginWidget.h"
#include "ui/MainWindow.h"

// 日志文件路径
static QFile *g_logFile = nullptr;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type)
    Q_UNUSED(context)
    if (g_logFile && g_logFile->isOpen()) {
        QByteArray localMsg = msg.toLocal8Bit();
        g_logFile->write(QDateTime::currentDateTime().toString("hh:mm:ss").toLocal8Bit());
        g_logFile->write(" ");
        g_logFile->write(localMsg);
        g_logFile->write("\n");
        g_logFile->flush();
    }
    // 同时输出到控制台
    fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());
    fflush(stderr);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // applicationName 决定 QSettings 路径（HKCU\Software\KaiqiChemical\恺祁颜色系统），
    // 语言、记住的账号、保存路径、搜索历史都存在这里；改名会让这些设置清零，
    // 所以这里保留旧值，对外显示名走 i18n 的 app_title（现为 KQ Cloud）。
    app.setApplicationName("恺祁颜色系统");
    app.setOrganizationName("KaiqiChemical");
    app.setApplicationVersion(QStringLiteral(APP_VERSION));

    // 初始化日志文件
    QString logPath = QDir::tempPath() + "/webcolor-qt-debug.log";
    g_logFile = new QFile(logPath);
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        g_logFile->write("\n=== "
            + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLocal8Bit()
            + " ===\n");
        g_logFile->flush();
        qInstallMessageHandler(messageHandler);
        qInfo() << "日志文件:" << logPath;
    }

    // 使用 Fusion 样式引擎，确保 QComboBox 弹出菜单等控件在全局 stylesheet 下正确渲染
    app.setStyle(QStringLiteral("Fusion"));

    // 加载全局样式表
    QFile styleFile(":/styles/styles.css");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }

    // 关闭程序时自动退出登录（清除 token）
    QObject::connect(&app, &QApplication::aboutToQuit, []() {
        qInfo() << "aboutToQuit - 清除 token";
        Auth::instance().removeToken();
        qInfo() << "aboutToQuit - token 已清除";
    });

    // 根据登录状态决定入口
    if (Auth::instance().isLoggedIn()) {
        MainWindow *w = new MainWindow;
        w->show();
    } else {
        LoginWidget *login = new LoginWidget;
        login->resize(480, 700);
        login->show();
    }

    return app.exec();
}
