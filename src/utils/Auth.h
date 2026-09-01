/* ============================================================
   Auth.h —— Token 与用户信息本地持久化（QSettings 替代 localStorage）
   - 关闭窗口后 token 保留（不自动登出）
   ============================================================ */

#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>

class Auth : public QObject
{
    Q_OBJECT
public:
    static Auth &instance();

    void setToken(const QString &token);
    QString getToken() const;
    void removeToken();
    bool isLoggedIn() const;

    void setUser(const QJsonObject &user);
    QJsonObject getUser() const;

signals:
    void loginStateChanged(bool loggedIn);

private:
    explicit Auth(QObject *parent = nullptr);
};
