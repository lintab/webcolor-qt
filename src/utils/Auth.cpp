/* ============================================================
   Auth.cpp —— Token 与用户信息持久化实现
   ============================================================ */

#include "Auth.h"
#include <QSettings>
#include <QJsonDocument>

static const QString TOKEN_KEY = QStringLiteral("webcolor_token");
static const QString USER_KEY  = QStringLiteral("webcolor_user");

Auth::Auth(QObject *parent) : QObject(parent) {}

Auth &Auth::instance()
{
    static Auth inst;
    return inst;
}

void Auth::setToken(const QString &token)
{
    QSettings settings;
    settings.setValue(TOKEN_KEY, token);
    emit loginStateChanged(!token.isEmpty());
}

QString Auth::getToken() const
{
    QSettings settings;
    return settings.value(TOKEN_KEY).toString();
}

void Auth::removeToken()
{
    QSettings settings;
    settings.remove(TOKEN_KEY);
    settings.remove(USER_KEY);
    emit loginStateChanged(false);
}

bool Auth::isLoggedIn() const
{
    return !getToken().isEmpty();
}

void Auth::setUser(const QJsonObject &user)
{
    QSettings settings;
    QJsonDocument doc(user);
    settings.setValue(USER_KEY, QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

QJsonObject Auth::getUser() const
{
    QSettings settings;
    QString raw = settings.value(USER_KEY).toString();
    if (raw.isEmpty()) return QJsonObject();
    QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
    return doc.object();
}
