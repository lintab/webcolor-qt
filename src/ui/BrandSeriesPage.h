/* ============================================================
   BrandSeriesPage.h —— 品牌车型搜索页面
   左右两栏布局：
   - 左栏（上）：品牌 + 车系联动下拉菜单（宽度可缩小）
   - 左栏（下）：车系对应的车型列表
   - 右栏（上）：车型对应的配方列表
   - 右栏（下）：配方详情
   ============================================================ */

#pragma once

#include <QWidget>
#include <QMap>

class QComboBox;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QVBoxLayout;
class FormulaDetailWidget;

class BrandSeriesPage : public QWidget
{
    Q_OBJECT
public:
    explicit BrandSeriesPage(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    struct BrandItem {
        QString id;
        QString name;
    };
    struct SeriesItem {
        QString id;
        QString brandId;
        QString name;
    };
    struct ModelItem {
        QString id;
        QString seriesId;
        QString name;
    };

    void setupUi();
    void loadBrands();
    void onBrandChanged(int index);
    void loadSeries(const QString &brandId);
    void onSeriesChanged(int index);
    void fetchModels();
    void selectModel(const ModelItem &model);
    void fetchFormulas();
    void showFormulaDetail(int formulaId);

    // 左栏上部：品牌车系联动
    QComboBox *m_brandCombo;
    QComboBox *m_seriesCombo;

    // 左栏下部：车型列表
    QListWidget *m_modelList;
    QLabel *m_modelHint;

    // 右栏上部：配方列表
    QListWidget *m_formulaList;
    QLabel *m_formulaHint;
    QLabel *m_formulaBreadcrumb;

    // 右栏下部：配方详情
    FormulaDetailWidget *m_detailWidget;

    // 数据
    QList<BrandItem> m_brands;
    QMap<QString, QList<SeriesItem>> m_seriesCache;
    QString m_currentBrandId;
    QString m_currentSeriesId;
    QString m_currentModelId;
    QString m_currentModelName;

    bool m_brandsLoaded = false;
    bool m_loadingModels = false;
    bool m_loadingFormulas = false;
};
