/* ============================================================
   ScApi.cpp —— 自动搜色（S&C）接口实现
   ============================================================ */

#include "ScApi.h"
#include "HttpClient.h"

ScApi::ScApi(QObject *parent)
    : QObject(parent)
{}

ScApi &ScApi::instance()
{
    static ScApi inst;
    return inst;
}

void ScApi::getCalsets(int pageNum, int pageSize,
                       Callback onSuccess, ErrorCallback onError)
{
    QJsonObject params;
    params[QStringLiteral("pageNum")] = pageNum;
    params[QStringLiteral("pageSize")] = pageSize;
    HttpClient::instance().get(QStringLiteral("/colorcr/calset/list"), params, onSuccess, onError);
}

void ScApi::getMyDevices(Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().get(QStringLiteral("/measurement/app/device/mine"), QJsonObject{}, onSuccess, onError);
}

void ScApi::getMeasurementJobs(const QString &deviceId,
                               Callback onSuccess, ErrorCallback onError)
{
    QJsonObject params;
    params[QStringLiteral("deviceId")] = deviceId;
    HttpClient::instance().get(QStringLiteral("/measurement/app/jobs"), params, onSuccess, onError);
}

void ScApi::searchColor(const QJsonObject &body,
                        Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().post(QStringLiteral("/colorcr/sc/search"), body, onSuccess, onError);
}

void ScApi::correctColor(const QJsonObject &body,
                         Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().post(QStringLiteral("/colorcr/sc/correct"), body, onSuccess, onError);
}

void ScApi::predictColor(const QJsonObject &body,
                         Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().post(QStringLiteral("/colorcr/sc/predict"), body, onSuccess, onError);
}

void ScApi::getVisualization(const QString &measurementId,
                             Callback onSuccess, ErrorCallback onError)
{
    QString path = QStringLiteral("/colorcr/sc/visualization/measurements/") + measurementId;
    HttpClient::instance().get(path, QJsonObject{}, onSuccess, onError);
}

void ScApi::getVisualizationWithFormula(const QString &measurementId, int formulaId,
                                        Callback onSuccess, ErrorCallback onError)
{
    QString path = QStringLiteral("/colorcr/sc/visualization/measurements/") + measurementId;
    QJsonObject params;
    params[QStringLiteral("formulaId")] = formulaId;
    HttpClient::instance().get(path, params, onSuccess, onError);
}

void ScApi::getMeasurementPreviewImages(const QString &measurementId,
                                        Callback onSuccess, ErrorCallback onError)
{
    QString path = QStringLiteral("/measurement/app/jobs/") + measurementId + QStringLiteral("/preview-images");
    HttpClient::instance().get(path, QJsonObject{}, onSuccess, onError);
}

void ScApi::getScSettings(Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().get(QStringLiteral("/colorcr/sc/settings"),
                               QJsonObject{}, onSuccess, onError);
}

void ScApi::getScMineSettings(Callback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().get(QStringLiteral("/colorcr/sc/settings/mine"),
                               QJsonObject{}, onSuccess, onError);
}

void ScApi::uploadMeasurement(const QJsonObject &body,
                              Callback onSuccess, ErrorCallback onError)
{
    // 标准设备上传入口：返回体 data 为 DeviceMeasurement（含数字 id 与 uuid）
    HttpClient::instance().post(QStringLiteral("/api/v1/device/measurements/standard"),
                                body, onSuccess, onError);
}
