/* ============================================================
   ScApi.h —— 自动搜色（S&C）接口
   ============================================================ */

#pragma once

#include <QObject>
#include <QJsonObject>
#include <functional>

class ScApi : public QObject
{
    Q_OBJECT
public:
    using Callback = std::function<void(const QJsonObject &)>;
    using ErrorCallback = std::function<void(const QString &errMsg, int code)>;

    static ScApi &instance();

    /** 梯度数据列表 */
    void getCalsets(int pageNum, int pageSize,
                    Callback onSuccess, ErrorCallback onError);

    /** 我的设备列表 */
    void getMyDevices(Callback onSuccess, ErrorCallback onError);

    /** 测量作业列表 */
    void getMeasurementJobs(const QString &deviceId,
                            Callback onSuccess, ErrorCallback onError);

    /** 搜色 */
    void searchColor(const QJsonObject &body,
                     Callback onSuccess, ErrorCallback onError);

    /** 修色 */
    void correctColor(const QJsonObject &body,
                      Callback onSuccess, ErrorCallback onError);

    /** 配方预测 */
    void predictColor(const QJsonObject &body,
                      Callback onSuccess, ErrorCallback onError);

    /** 可视化数据 */
    void getVisualization(const QString &measurementId,
                          Callback onSuccess, ErrorCallback onError);

    /** 可视化数据（带配方ID，获取配方+修色光谱） */
    void getVisualizationWithFormula(const QString &measurementId, int formulaId,
                                     Callback onSuccess, ErrorCallback onError);

    /** 测量作业预览图 */
    void getMeasurementPreviewImages(const QString &measurementId,
                                     Callback onSuccess, ErrorCallback onError);

    /** S&C 全局配置（分数阈值等） */
    void getScSettings(Callback onSuccess, ErrorCallback onError);

    /** S&C 当前用户配置 */
    void getScMineSettings(Callback onSuccess, ErrorCallback onError);

    /** 上传设备测量数据（SDX4 光谱数据到 /api/v1/measurements） */
    void uploadMeasurement(const QJsonObject &body,
                           Callback onSuccess, ErrorCallback onError);

private:
    explicit ScApi(QObject *parent = nullptr);
};
