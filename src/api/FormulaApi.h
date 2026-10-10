/* ============================================================
   FormulaApi.h —— 配方查询接口
   ============================================================ */

#pragma once

#include <QObject>
#include <QJsonObject>
#include <QStringList>
#include <functional>

class FormulaApi : public QObject
{
    Q_OBJECT
public:
    using Callback = std::function<void(const QJsonObject &)>;
    using ErrorCallback = std::function<void(const QString &errMsg, int code)>;

    static FormulaApi &instance();

    /** 搜索公司配方（分页） */
    void searchFormula(const QString &keyword, int pageNum, int pageSize,
                       Callback onSuccess, ErrorCallback onError);

    /** 搜索联想 */
    void suggest(const QString &q, int limit,
                 Callback onSuccess, ErrorCallback onError);

    /** 配方详情 */
    void getFormulaDetail(int id, const QString &pigmentCodeSystem,
                          Callback onSuccess, ErrorCallback onError);

    /** 获取汽车品牌列表（分页） */
    void getBrands(int pageNum, int pageSize,
                   Callback onSuccess, ErrorCallback onError);

    /** 获取品牌下车系 */
    void getSeriesByBrand(const QString &brandId,
                          Callback onSuccess, ErrorCallback onError);

    /** 获取车系下车型（分页） */
    void getModelsBySeries(const QString &seriesId, int pageNum, int pageSize,
                           Callback onSuccess, ErrorCallback onError);

    /** 按车型查询配方列表（分页） */
    void getFormulasByModel(const QString &modelId, int pageNum, int pageSize,
                            Callback onSuccess, ErrorCallback onError);

    /** 批量查询色母内部编号对应的各品牌（猛鹰/雄途/恺祁/施卡斯）编码 */
    void getPigmentVendorCodes(const QStringList &pigmentCodes,
                               Callback onSuccess, ErrorCallback onError);

    /** 上传用户配方（S&C 保存功能） */
    void uploadUserFormula(const QJsonObject &body,
                           Callback onSuccess, ErrorCallback onError);

private:
    explicit FormulaApi(QObject *parent = nullptr);
};
