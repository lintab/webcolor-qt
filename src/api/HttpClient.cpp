/* ============================================================
   HttpClient.cpp —— HTTP 请求封装实现
   ============================================================ */

#include "HttpClient.h"
#include "utils/Auth.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QUrl>
#include <QUrlQuery>

static const QString BACKEND_BASE = QStringLiteral("https://color.kaiqichemical.com");

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
    , m_mgr(new QNetworkAccessManager(this))
{}

HttpClient &HttpClient::instance()
{
    static HttpClient inst;
    return inst;
}

QString HttpClient::baseUrl()
{
    return BACKEND_BASE;
}

QString HttpClient::resourceUrl(const QString &path)
{
    if (path.isEmpty()) return {};
    if (path.startsWith(QStringLiteral("http://")) || path.startsWith(QStringLiteral("https://")))
        return path;
    if (path.startsWith(QLatin1Char('/')))
        return BACKEND_BASE + path;
    return BACKEND_BASE + QLatin1Char('/') + path;
}

void HttpClient::get(const QString &path, const QJsonObject &params,
                     Callback onSuccess, ErrorCallback onError, bool isToken)
{
    request(path, QStringLiteral("get"), params, isToken, onSuccess, onError);
}

void HttpClient::post(const QString &path, const QJsonObject &data,
                      Callback onSuccess, ErrorCallback onError, bool isToken)
{
    request(path, QStringLiteral("post"), data, isToken, onSuccess, onError);
}

void HttpClient::put(const QString &path, const QJsonObject &data,
                     Callback onSuccess, ErrorCallback onError, bool isToken)
{
    request(path, QStringLiteral("put"), data, isToken, onSuccess, onError);
}

void HttpClient::request(const QString &path, const QString &method,
                         const QJsonObject &params, bool isToken,
                         Callback onSuccess, ErrorCallback onError)
{
    // 组装 URL
    QString url = BACKEND_BASE + path;
    QString upperMethod = method.toUpper();

    if (upperMethod == QStringLiteral("GET") && !params.isEmpty()) {
        QUrlQuery query;
        for (auto it = params.begin(); it != params.end(); ++it) {
            const auto &val = it.value();
            if (val.isNull() || val.isUndefined()) continue;
            QString v = val.isString() ? val.toString() : QString::number(val.toDouble());
            if (v.isEmpty()) continue;
            query.addQueryItem(it.key(), v);
        }
        QString qs = query.toString(QUrl::FullyEncoded);
        if (!qs.isEmpty()) {
            url += (url.contains(QLatin1Char('?')) ? QLatin1Char('&') : QLatin1Char('?')) + qs;
        }
    }

    QUrl qurl(url);
    QNetworkRequest req{qurl};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json;charset=utf-8"));

    // 附带 Bearer token
    if (isToken) {
        QString token = Auth::instance().getToken();
        if (!token.isEmpty()) {
            req.setRawHeader("Authorization", QByteArray("Bearer ") + token.toLatin1());
        }
    }

    QNetworkReply *reply = nullptr;
    if (upperMethod == QStringLiteral("GET")) {
        reply = m_mgr->get(req);
    } else {
        QByteArray body = QJsonDocument(params).toJson(QJsonDocument::Compact);
        if (upperMethod == QStringLiteral("POST"))
            reply = m_mgr->post(req, body);
        else if (upperMethod == QStringLiteral("PUT"))
            reply = m_mgr->put(req, body);
        else
            reply = m_mgr->deleteResource(req);
    }

    connect(reply, &QNetworkReply::finished, this, [reply, onSuccess, onError]() {
        reply->deleteLater();

        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray raw = reply->readAll();

        // 网络层错误
        if (reply->error() != QNetworkReply::NoError && raw.isEmpty()) {
            if (onError) onError(QStringLiteral("网络请求失败，请检查后端服务是否可访问"), -1);
            return;
        }

        // 401 token 失效
        if (httpStatus == 401) {
            Auth::instance().removeToken();
            emit HttpClient::instance().tokenExpired();
            if (onError) onError(QStringLiteral("登录已过期，请重新登录"), 401);
            return;
        }

        // 解析 JSON
        QJsonDocument doc = QJsonDocument::fromJson(raw);
        QJsonObject body = doc.object();

        if (body.isEmpty() && !raw.isEmpty()) {
            if (onError) onError(QStringLiteral("后端返回数据格式异常"), -1);
            return;
        }

        // 业务失败
        if (body.contains(QStringLiteral("code"))) {
            int code = body.value(QStringLiteral("code")).toInt();
            if (code != 200) {
                QString msg = body.value(QStringLiteral("msg")).toString();
                if (msg.isEmpty()) msg = QStringLiteral("业务处理失败");
                if (onError) onError(msg, code);
                return;
            }
        }

        if (onSuccess) onSuccess(body);
    });
}
