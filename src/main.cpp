/* ============================================================
   main.cpp —— 程序入口
   - 初始化 QApplication，加载全局样式
   - 根据登录状态决定显示登录页或主窗口
   ============================================================ */

#include <QApplication>
#include <QFile>
#include "utils/Auth.h"
#include "ui/LoginWidget.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("恺祁颜色系统");
    app.setOrganizationName("KaiqiChemical");

    // 加载全局样式表
    QFile styleFile(":/styles/styles.css");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }

    // 关闭程序时自动退出登录（清除 token）
    QObject::connect(&app, &QApplication::aboutToQuit, []() {
        Auth::instance().removeToken();
    });

    // 根据登录状态决定入口
    if (Auth::instance().isLoggedIn()) {
        MainWindow *w = new MainWindow;
        w->resize(1200, 780);
        w->show();
    } else {
        LoginWidget *login = new LoginWidget;
        login->resize(480, 700);
        login->show();
    }

    return app.exec();
}
