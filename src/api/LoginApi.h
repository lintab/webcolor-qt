/* ============================================================
   LoginApi.h —— 登录相关接口
   ============================================================ */

#pragma once

#include <QObject>
#include <QJsonObject>
#include <functional>

class LoginApi : public QObject
{
    Q_OBJECT
public:
    using Callback = std::function<void(const QJsonObject &)>;
    using ErrorCallback = std::function<void(const QString &errMsg, int code)>;

    static LoginApi &instance();

    /** 获取验证码 */
    void getCaptcha(Callback onSuccess, ErrorCallback onError);

    /** 登录 */
    void login(const QString &username, const QString &password,
               const QString &code, const QString &uuid,
               Callback onSuccess, ErrorCallback onError);

    /** 获取用户信息 */
    void getInfo(Callback onSuccess, ErrorCallback onError);

    /** 退出登录 */
    void logout(Callback onSuccess, ErrorCallback onError);

    /** 更新个人资料 */
    void updateProfile(const QJsonObject &data, Callback onSuccess, ErrorCallback onError);

    /** 获取个人资料 */
    void getProfile(Callback onSuccess, ErrorCallback onError);

    /** 手机号注册 */
    void registerByPhone(const QJsonObject &data, Callback onSuccess, ErrorCallback onError);

    /** 邮箱注册 */
    void registerByEmail(const QJsonObject &data, Callback onSuccess, ErrorCallback onError);

    /** 发送短信验证码 */
    void sendSmsCode(const QString &phone, const QString &scene, Callback onSuccess, ErrorCallback onError);

    /** 发送邮件验证码 */
    void sendEmailCode(const QString &email, const QString &scene, Callback onSuccess, ErrorCallback onError);

private:
    explicit LoginApi(QObject *parent = nullptr);
};
