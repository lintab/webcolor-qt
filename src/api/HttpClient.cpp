/* ============================================================
   HttpClient.cpp —— HTTP 请求封装实现
   ============================================================ */

#include "HttpClient.h"
#include "utils/Auth.h"
#include "utils/I18nManager.h"
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

    ++m_pending;
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, onSuccess, onError, reqUrl = req.url()]() {
        dispatchReply(reply, onSuccess, onError, reqUrl);

        // 回调执行完毕后才报空闲，避免销毁中的页面仍被回调访问
        if (m_pending > 0) --m_pending;
        emit pendingChanged(m_pending);
    });
}

void HttpClient::dispatchReply(QNetworkReply *reply,
                               const Callback &onSuccess,
                               const ErrorCallback &onError,
                               const QUrl &reqUrl)
{
    reply->deleteLater();

    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray raw = reply->readAll();

    qInfo() << "[HttpClient]" << reqUrl << "HTTP" << httpStatus << "body size:" << raw.size();

    // 网络层错误
    if (reply->error() != QNetworkReply::NoError && raw.isEmpty()) {
        qWarning() << "[HttpClient] 网络层错误:" << reply->errorString();
        if (onError) onError(T("http_network_error"), -1);
        return;
    }

    // 401 token 失效
    if (httpStatus == 401) {
        Auth::instance().removeToken();
        emit tokenExpired();
        if (onError) onError(T("http_session_expired"), 401);
        return;
    }

    // 解析 JSON
    QJsonDocument doc = QJsonDocument::fromJson(raw);
    QJsonObject body = doc.object();

    if (body.isEmpty() && !raw.isEmpty()) {
        qWarning() << "[HttpClient] 后端返回非 JSON 数据:" << QByteArray(raw.left(500));
        if (onError) onError(T("http_bad_response") + QString::fromUtf8(raw.left(200)), -1);
        return;
    }

    // 业务失败
    if (body.contains(QStringLiteral("code"))) {
        int code = body.value(QStringLiteral("code")).toInt();
        if (code != 200) {
            QString msg = body.value(QStringLiteral("msg")).toString();
            if (msg.isEmpty()) msg = body.value(QStringLiteral("message")).toString();
            if (msg.isEmpty()) msg = T("http_business_failed");
            // 业务级别 401 也视为 token 失效
            if (code == 401) {
                Auth::instance().removeToken();
                emit tokenExpired();
            }
            if (onError) onError(msg, code);
            return;
        }
    }

    if (onSuccess) onSuccess(body);
}
