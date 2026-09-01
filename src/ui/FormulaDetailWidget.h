/* ============================================================
   FormulaDetailWidget.h —— 配方详情面板
   - 头部缩略图 + 颜色名/色号 + 属性标签
   - 配方信息网格
   - 色母明细表格（按层分组、编码体系切换、克重换算）
   - 纹理参数、备注
   - 下载配方按钮
   ============================================================ */

#pragma once

#include <QWidget>
#include <QJsonObject>
#include <QList>
#include "models/FormulaModel.h"

class QLabel;
class QTableWidget;
class QComboBox;
class QLineEdit;
class QVBoxLayout;
class QScrollArea;

class FormulaDetailWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FormulaDetailWidget(QWidget *parent = nullptr);

    void loadDetail(int id);
    void clear();

private:
    void renderDetail(const FormulaDetail &detail);
    void renderPigmentTable(const QList<PigmentDetailRow> &rows, int layer,
                            QVBoxLayout *container);
    void recalcLayerWeights(QTableWidget *table, const QString &targetVal);
    void downloadLayerFormula(int layer);

    // 状态
    int m_currentId = 0;
    QString m_currentPigmentSystem = QStringLiteral("internal");
    QJsonObject m_currentFormula;  // 原始 formula JSON（导出用）

    // UI
    QVBoxLayout *m_contentLayout;
    QScrollArea *m_scrollArea;
    QWidget     *m_emptyWidget;
    QWidget     *m_contentWidget;
};
