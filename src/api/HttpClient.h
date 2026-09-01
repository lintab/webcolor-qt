/* ============================================================
   HttpClient.h —— 统一 HTTP 请求封装
   - 基于 QNetworkAccessManager
   - 自动拼接后端地址、附带 Bearer token
   - 统一错误处理：401 清 token、业务码非 200 报错
   ============================================================ */

#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

class HttpClient : public QObject
{
    Q_OBJECT
public:
    static HttpClient &instance();

    using Callback = std::function<void(const QJsonObject &)>;
    using ErrorCallback = std::function<void(const QString &errMsg, int code)>;

    /**
     * 发起请求
     * @param path     接口路径（如 /login）
     * @param method   GET / POST
     * @param params   查询参数（GET）或请求体（POST）
     * @param isToken  是否附带 Bearer token
     * @param onSuccess  成功回调
     * @param onError    失败回调
     */
    void request(const QString &path,
                 const QString &method,
                 const QJsonObject &params,
                 bool isToken,
                 Callback onSuccess,
                 ErrorCallback onError);

    void get(const QString &path, const QJsonObject &params,
             Callback onSuccess, ErrorCallback onError, bool isToken = true);

    void post(const QString &path, const QJsonObject &data,
              Callback onSuccess, ErrorCallback onError, bool isToken = true);

    void put(const QString &path, const QJsonObject &data,
             Callback onSuccess, ErrorCallback onError, bool isToken = true);

    /** 后端基础地址 */
    static QString baseUrl();

    /** 资源地址补全 */
    static QString resourceUrl(const QString &path);

signals:
    /** token 失效时发出，供 UI 层监听跳登录 */
    void tokenExpired();

private:
    explicit HttpClient(QObject *parent = nullptr);
    QNetworkAccessManager *m_mgr;
};
