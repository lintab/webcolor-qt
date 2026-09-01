/* ============================================================
   LoginApi.cpp —— 登录接口实现
   ============================================================ */

#include "LoginApi.h"
#include "HttpClient.h"

LoginApi::LoginApi(QObject *parent) : QObject(parent) {}

LoginApi &LoginApi::instance()
{
    static LoginApi inst;
    return inst;
}

void LoginApi::getCaptcha(Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().get(QStringLiteral("/captchaImage"), {}, onSuccess, onError, /*isToken=*/false);
}

void LoginApi::login(const QString &username, const QString &password,
                     const QString &code, const QString &uuid,
                     Callback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data[QStringLiteral("username")]   = username;
    data[QStringLiteral("password")]   = password;
    data[QStringLiteral("code")]       = code;
    data[QStringLiteral("uuid")]       = uuid;
    data[QStringLiteral("clientType")] = QStringLiteral("web");

    HttpClient::instance().post(QStringLiteral("/login"), data, onSuccess, onError, /*isToken=*/false);
}

void LoginApi::getInfo(Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().get(QStringLiteral("/getInfo"), {}, onSuccess, onError);
}

void LoginApi::logout(Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().post(QStringLiteral("/logout"), {}, onSuccess, onError);
}

void LoginApi::updateProfile(const QJsonObject &data, Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().put(QStringLiteral("/system/user/profile"), data, onSuccess, onError);
}

void LoginApi::getProfile(Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().get(QStringLiteral("/system/user/profile"), {}, onSuccess, onError);
}

void LoginApi::registerByPhone(const QJsonObject &data, Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().post(QStringLiteral("/colorcr/app/auth/register"), data, onSuccess, onError, /*isToken=*/false);
}

void LoginApi::registerByEmail(const QJsonObject &data, Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().post(QStringLiteral("/colorcr/app/auth/email/register"), data, onSuccess, onError, /*isToken=*/false);
}

void LoginApi::sendSmsCode(const QString &phone, const QString &scene, Callback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data[QStringLiteral("phone")] = phone;
    data[QStringLiteral("scene")] = scene;
    HttpClient::instance().post(QStringLiteral("/colorcr/app/auth/sms/send"), data, onSuccess, onError, /*isToken=*/false);
}

void LoginApi::sendEmailCode(const QString &email, const QString &scene, Callback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data[QStringLiteral("email")] = email;
    data[QStringLiteral("scene")] = scene;
    HttpClient::instance().post(QStringLiteral("/colorcr/app/auth/email/send"), data, onSuccess, onError, /*isToken=*/false);
}
