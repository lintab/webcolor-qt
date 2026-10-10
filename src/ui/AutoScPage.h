/* ============================================================
   AutoScPage.h —— 自动搜色（S&C）页面
   左右两栏布局：
   - 左栏：作业定义（梯度/设备/测量）、搜色类型、筛选、按钮
   - 右栏：搜索结果表格、配方面板、可视化面板
   ============================================================ */

#pragma once

#include <QWidget>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QColor>
#include <QSet>
#include <QStringList>
#include <QPixmap>
#include <QMouseEvent>
#include <QEvent>
#include <QShowEvent>
#include <QPointer>

#include "device/XriteDevice.h"

class QComboBox;
class QSpinBox;
class QRadioButton;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QLabel;
class QTableWidget;
class QSplitter;
class QGroupBox;
class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;

class AutoScPage : public QWidget
{
    Q_OBJECT
public:
    explicit AutoScPage(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onDeviceChanged(int index);
    void onMeasurementChanged(int index);
    void onBrandChanged(int index);
    void onSeriesChanged(int index);
    void doSearch();
    void resetFilters();
    void onResultRowSelected(int row);
    void onCorrect();
    void onSmartCorrect();
    void onEditFormula();
    void onMeasure();
    void onSaveFormula();
    /** 下载配方（layerKey: "base"/"surface"/"single"） */
    void onDownloadFormula(const QString &layerKey);

    // ─── SDX4 设备信号槽 ─────────────────────────────────────
    void onSdx4Connected();
    void onSdx4Disconnected();
    void onSdx4MeasurementReady(const QList<XriteDevice::SpectralSet> &spectra);
    void onSdx4MeasurementProgress(int done, int total);
    void onSdx4Error(const QString &msg);

private:
    struct CalsetItem { QString id; QString name; bool isDefault = false; QString alias; };
    struct DeviceItem { QString id; QString deviceId; QString model; };
    struct JobItem {
        QString id;
        QString title;
        QString swatch;
        double coarseness = 0;        // 颗粒度
        double sparkleR15As15 = 0;    // 正面闪烁度
        double sparkleR15As80 = 0;    // 侧面闪烁度
    };
    struct BrandItem { QString id; QString name; QString nameEn; };
    struct SeriesItem { QString id; QString brandId; QString name; QString nameEn; };
    struct ModelItem { QString id; QString seriesId; QString name; QString nameEn; };

    /** S&C 分数阈值配置（与后端 ScSettings 一致） */
    struct ScoreThreshold { double green = -1; double yellow = -1; };
    /** 根据分数和阈值返回颜色 tone: green/yellow/red/空 */
    static QString scoreTone(double score, const ScoreThreshold &th);
    void loadDisplaySettings();

    void setupUi();
    void setupSidebar(QWidget *sidebar);
    void setupMainArea(QWidget *mainWidget);
    void updateScrollContainerHeight();
    /** 左栏按当前视口宽度补最小高度：widgetResizable 只认 sizeHint，换行标签变高会滑不到底 */
    void syncSidebarContentHeight();
    /** 内容变化（显隐、文案变长）后延后一轮重新测量左栏高度 */
    void scheduleSidebarSync();
    void loadInitialData();
    void loadDevices();
    void loadMeasurementJobs(const QString &deviceId);
    void loadBrands();
    void loadSeries(const QString &brandId);
    /** 配方面板“品牌”（色母编号体系）下拉变化：按新体系重新拉取配方详情 */
    void onPigmentSystemChanged(int index);
    void loadModels(const QString &seriesId);
    void updateJobCard(const JobItem &job);
    void clearJobCard();
    void loadPreviewImages(const QString &measurementId);
    QJsonObject buildSearchBody();
    void populateResultTable(const QJsonObject &res);
    void loadRecipePanel(const QJsonObject &row);
    void clearRecipePanel();
    void loadVisualization(const QString &measurementId);
    void loadFormulaVisualization(const QString &measurementId, int formulaId);
    /** 按罐容量重建色母表（前端 buildLayerTonerRows）：topRows 非空时为三工序两层；
     *  如果 correctedBaseRows/correctedTopRows 非空，则同时显示初始和修色后的数据 */
    void rebuildTonerTable(const QJsonArray &baseRows, const QJsonArray &topRows,
                           const QJsonArray &correctedBaseRows = QJsonArray(),
                           const QJsonArray &correctedTopRows = QJsonArray());
    /** 按当前选中配方列就地刷新数量/累计与列高亮（点表头切列时不重建表格，页面位置保持） */
    void applyFormulaColumnSelection();
    /** 执行修色：method = CLASSIC / SMART */
    void runCorrect(const QString &method);
    /** 同步修色/智能修色按钮启用态：修色(CLASSIC)在已修色行禁用，智能修色(SMART)始终可用 */
    void syncCorrectButtonState();
    /** 当前搜色是否为效应漆（金属漆）——决定使用哪一组分数阈值 */
    bool isEffectSearch() const;

    // ─── 搜索后自动修色（对标前端 autoCorrectScSearchRows） ───
    /** 一批自动修色请求：同一 industryType 下每最多 2 个 formulaId */
    struct AutoCorrectBatch {
        QString industryType;          // PAINT / EFFECTS
        QList<double> formulaIds;      // 本批配方 ID
        QList<int> rows;               // 与 formulaIds 对齐的表格行号
    };
    void startAutoCorrect(int generation);           // 扫描结果表构建批次并启动
    void processNextAutoCorrectBatch();              // 顺序发起下一批
    void applyAutoCorrectResult(const AutoCorrectBatch &batch, const QJsonObject &res); // 回填本批
    void patchRowCorrected(int row, const QJsonObject &entry);  // 单行修色成功回填
    void markRowCorrectFailed(int row);              // 单行修色失败
    void markRowCorrectPending(int row);             // 单行标记修色中

    // ─── 保存配方功能 ─────────────────────────────────────────
    void showSaveFormulaDialog();                    // 显示保存配方对话框
    void submitSaveFormula(const QJsonObject &form); // 提交保存配方
    QJsonArray buildSaveDetails();                   // 构建保存的色母明细

    // ─── 编辑配方（对标前端 ScRecipePanel 的 startEdit/confirmEdit） ───
    /** 草稿行：一条色母在某层的克重（内部单位始终为克） */
    struct EditDraftRow {
        QString layerKey;        // single / base / surface
        QString code;            // 界面显示的色母编号（大写，AC-Edited 占比的键）
        QString predictCode;     // 提交预测用的内部编号（品牌编号经 vendor-codes 反查）
        QString pigmentName;     // 色母中文名（predict 用）
        double weightGrams = 0;
    };
    /** 编辑态下的数量输入格（索引与草稿表一致） */
    struct EditCellRef {
        QPointer<QLineEdit> editor;
        QPointer<QTableWidget> table;
        int row = -1;
        int draftIdx = -1;
        QString unit;                  // 该层数量列的单位
    };
    /** 编辑态下每一层的总计标签（罐容量在编辑态不联动，故不持有其输入框） */
    struct EditSectionRef {
        QString layerKey;
        QString unit;                  // 该层合计当前单位
        QPointer<QLabel> sumLabel;
    };
    /** 配方(%)列点选所需信息：切换列时只重算数量/累计并高亮，不重建表格（页面位置不变） */
    struct FormulaColumnView {
        QPointer<QTableWidget> table;
        QString layerKey;              // single / base / surface
        bool isBase = false;
        QJsonArray rows;               // 该层初始色母行
        QJsonArray correctedRows;      // 该层修色行
        int colInitial = -1;
        int colAc = -1;
        int colAcEdited = -1;
        int colQty = -1;
        int colCum = -1;
        QString unit;                  // 该层数量列单位
        QColor headerBg;               // 配方(%) 分组表头原底色
    };
    void startEditFormula();      // 进入编辑态：按当前选中列折算每层色母克重为草稿
    void cancelEditFormula();     // 丢弃草稿
    void confirmEditFormula();    // 草稿折算为 AC-Edited（数量按罐容量等比归一）并发起预测
    void runPredict(const QJsonArray &components);  // POST /colorcr/sc/predict
    void applyEditRecalc(QLineEdit *skip = nullptr); // 草稿变化后刷新数量/累计/总计
    /** 三工序：一层克重变化后另一层按原比例跟随 */
    void applyDraftLayerLinkage(const QString &layerKey);
    /** 当前层在草稿中的 key（三工序区分底层/面层） */
    QString editLayerKey(bool isBaseLayer) const;
    /** 编辑态下列按钮与下载按钮的可用性 */
    void syncEditActionButtons();
    /** 切换配方/清空面板时丢弃编辑与 AC-Edited 状态 */
    void resetEditState();
    /** 当前行若带 predictDetail，则恢复其 AC-Edited 占比 */
    void restoreAcEditedFromRow();
    /** 用 predict 返回的 components 重建 AC-Edited 层内占比（对标前端 syncAcEditedFromComponents） */
    bool applyEditedComponents(const QJsonArray &components);
    /** predict 结果写入当前行并刷新表格/可视化（对标前端 applyScPredictResult + applyScPredictToRow） */
    void applyPredictDetail(const QJsonObject &predictData);
    QList<EditDraftRow> m_editDraft;
    QList<EditCellRef> m_editCells;
    QList<EditSectionRef> m_editSections;
    QList<FormulaColumnView> m_formulaViews;   // 配方(%)列点选用：切列时就地刷新
    int m_editQtyCol = -1;
    int m_editCumCol = -1;
    bool m_editing = false;
    bool m_predicting = false;
    bool m_hasAcEdited = false;
    QMap<QString, double> m_acEditedPct;   // layerKey|色母编号 → 层内百分比

    // 左侧栏 - 作业定义
    QComboBox *m_calsetCombo;
    QComboBox *m_deviceCombo;
    QComboBox *m_measurementCombo;
    QPushButton *m_refreshJobsBtn;
    QLabel *m_jobPreviewLabel;
    QList<QPixmap> m_previewPixmaps;   // 按角度顺序，null 表示未加载
    QStringList m_jobPreviewUrls;      // 作业预览图 URL（按角度排序）
    QStringList m_jobPreviewGeoCodes;  // 与 m_jobPreviewUrls 对齐的角度码
    void rebuildPreviewPixmap();

    // “作业 | 配方”对比列：图片缓存 + 异步下载
    class QNetworkAccessManager *m_previewNet = nullptr; // 共享下载管理器（避免每图新建导致线程/句柄爆炸）
    QMap<QString, QPixmap> m_previewImgCache;   // 原始 url -> 已加载图
    QSet<QString> m_previewImgPending;          // 正在下载的 url
    QPixmap m_logoPixmap;                        // 缺图回退 logo
    void requestPreviewImage(const QString &url);
    void applyComparePairsForRow(int row);       // 根据当前作业图+本行配方图计算对比带
    void rebuildCompareColumns();                // 作业图晚到时重算所有行
    QLabel *m_jobGranularityLabel;
    QLabel *m_jobSparkle15Label;
    QLabel *m_jobSparkle80Label;
    QGroupBox *m_jobCard;

    // 左侧栏 - 搜色类型
    QRadioButton *m_solidRadio;
    QRadioButton *m_metalRadio;
    QCheckBox *m_threeStepCheck;

    // 左侧栏 - 筛选
    QComboBox *m_brandCombo;
    QComboBox *m_seriesCombo;
    QComboBox *m_modelCombo;
    QLineEdit *m_yearEdit;
    QLineEdit *m_colorNameEdit;
    QLineEdit *m_colorCodeEdit;

    // 左侧栏 - 按钮
    QPushButton *m_resetBtn;
    QPushButton *m_searchBtn;

    // 右侧 - 结果表格
    QTableWidget *m_resultTable;
    QLabel *m_resultCountLabel;

    // 右侧 - 配方面板
    QLabel *m_recipeMetaLabel;
    QLabel *m_recipeCoatTypeLabel;
    QComboBox *m_pigmentSystemCombo = nullptr;  // 配方面板“品牌”（色母编号体系）下拉
    QLabel *m_recipeScoreOLabel;
    QLabel *m_recipeScoreAcLabel;
    QLabel *m_recipeScoreAcEditedLabel = nullptr;   // 分数(AC-Edited)，仅编辑预测后有值
    QWidget *m_recipeScoreAcEditedGroup = nullptr;  // 上述标签及其标题，用于整组显隐
    void refreshAcEditedScoreLabel();               // 按当前行的 predictDetail 刷新分数(AC-Edited)
    QLabel *m_recipeInfoLabel;
    QWidget *m_tonerContainer = nullptr;      // 色母表容器（含罐容量）
    QTableWidget *m_tonerTable = nullptr;     // 非三工序色母表
    QTableWidget *m_baseTable = nullptr;      // 三工序底层表
    QTableWidget *m_surfaceTable = nullptr;   // 三工序面层表
    QLineEdit *m_canCapacityEdit = nullptr;   // 罐容量输入（非三工序）
    QLineEdit *m_baseCapacityEdit = nullptr;  // 底层罐容量输入
    QLineEdit *m_surfaceCapacityEdit = nullptr; // 面层罐容量输入
    QLabel *m_tonerTotalLabel = nullptr;      // 非三工序总计
    QLabel *m_baseTotalLabel = nullptr;       // 底层总计
    QLabel *m_surfaceTotalLabel = nullptr;    // 面层总计
    double m_canCapacity = 100.0;
    double m_baseCapacity = 100.0;
    double m_surfaceCapacity = 100.0;
    QPushButton *m_correctBtn;
    QPushButton *m_smartCorrectBtn;
    QPushButton *m_editBtn;          // 编辑配方
    QPushButton *m_cancelEditBtn = nullptr;   // 编辑态：取消
    QPushButton *m_confirmEditBtn = nullptr;  // 编辑态：确认（预测）
    QPushButton *m_measureBtn;       // 测量
    QPushButton *m_saveBtn;          // 保存配方
    bool m_saveAllowed = false;      // 测量完成后才允许保存（编辑/预测期间临时禁用）
    QPushButton *m_downloadBaseBtn = nullptr;      // 下载底层（三工序）
    QPushButton *m_downloadSurfaceBtn = nullptr;   // 下载面层（三工序）
    QPushButton *m_downloadSingleBtn = nullptr;    // 下载配方（非三工序）
    bool m_isThreeStep = false;                    // 当前配方是否三工序
    QJsonObject m_currentRowObj;    // 当前选中行（罐容量变化时重算用）

    // 右侧 - 可视化面板
    struct VizCard {
        QWidget *widget = nullptr;      // 卡片容器
        QComboBox *typeCombo = nullptr; // 类型选择下拉
        QWidget *contentWidget = nullptr; // 内容控件
        QString type;                   // 当前类型: spectral_data/spectral_curve/color_data/color_block
        QString illuminant = QStringLiteral("D65-10"); // 色彩数据光源选择
        QStringList angles;             // 色彩数据角度选择（默认全选）
        QString colorBlockMode = QStringLiteral("render"); // 色块显示模式：render（渲染）/image（图像）
        QPushButton *settingsBtn = nullptr; // 色块卡片右上角设置按钮
    };
    QWidget *m_vizPanel = nullptr;           // 可视化面板
    QVBoxLayout *m_vizLayout = nullptr;      // 可视化面板内容布局
    QLabel *m_vizInfoLabel = nullptr;        // 提示信息
    QList<VizCard> m_vizCards;               // 可视化卡片列表
    QJsonObject m_vizData;                   // 可视化数据缓存（测量标样）
    QJsonObject m_formulaVizData;            // 配方可视化数据（初始配方+修色后）
    QJsonObject m_searchTargetRgbByGeometry; // 搜色响应中的目标色五角度 RGB
    QJsonObject m_rowRgbByGeometry;          // 当前选中行的配方五角度 RGB
    void addVizCard(const QString &type = QStringLiteral("color_data"));
    void removeVizCard(int index);
    void updateVizCardContent(VizCard &card);
    /** 角度筛选按钮（色彩数据/色块卡片共用 card.angles） */
    void addAngleToggleButtons(QHBoxLayout *row, VizCard &card, const QStringList &availableAngles);
    /** 色块卡片「设置」弹窗：显示模式（图像 / 渲染） */
    void showColorBlockSettingsDialog(int cardIndex);
    QList<QPointer<QWidget>> m_colorBlockCells;   // 图像模式下的色块格，预览图到位后直接重绘
    void adjustVizPanelHeight();          // 自适应可视化面板高度
    void updateDeleteButtonState();
    QTableWidget *m_colorDataTable = nullptr; // 保留兼容（色彩数据表）

    /** 合并 标样(m_vizData) + 配方(m_formulaVizData) + 修色(m_currentRowObj.correctDetail)
     *  的光谱行：按 sourceKey:geometry 去重（先到先得），再按来源/角度顺序排序
     *  （对标前端 displaySpectralData 合并与 SC_VIZ_SOURCE_ORDER 排序） */
    QJsonArray buildMergedSpectralData() const;
    /** 从单条光谱数据中提取 波长(nm)→反射率 映射（兼容后端多种光谱结构） */
    static QMap<double, double> spectralValuesFromItem(const QJsonObject &item);
    /** 当前测量作业标题（标样系列显示名用） */
    QString currentJobTitle() const;

    // ─── SDX4 设备集成 ────────────────────────────────────────
    /** 将 SDX4 光谱数据上传后端，返回 measurementId 后刷新页面 */
    void uploadAndUseMeasurement(const QList<XriteDevice::SpectralSet> &spectra);
    /** 更新 SDX4 设备状态标签 */
    void updateSdx4StatusLabel();
    /** 打开测量弹窗（测量次数/数据模式/设备状态由弹窗决定） */
    void showMeasureDialog();
    /** 弹窗内按下测量：未连接时先连接，连接好了再测量 */
    void startMeasureFromDialog();
    /** 按弹窗设定应用数据模式并连测 */
    void runMeasureFromDialog();
    /** 测量期间锁定/解锁弹窗控件 */
    void setMeasureDlgBusy(bool busy);
    /** 弹窗内提示行 */
    void setMeasureDlgHint(const QString &text, bool error = false);

    // 光谱曲线卡片状态（对标前端 ScSpectralCurvePanel 的 displayMode / selectedAngles）
    QString m_curveMode = QStringLiteral("R");       // R / dR
    QStringList m_curveAvailableAngles;              // 可用角度（标准几何，按标准顺序）
    QSet<QString> m_curveSelectedAngles;             // 已选角度

    // 数据
    QList<CalsetItem> m_calsets;
    QList<DeviceItem> m_devices;
    QList<JobItem> m_jobs;
    QList<BrandItem> m_brands;
    QMap<QString, QList<SeriesItem>> m_seriesCache;
    QMap<QString, QList<ModelItem>> m_modelCache;

    QString m_currentMeasurementId;
    QString m_currentTargetHex;
    bool m_loaded = false;
    bool m_searching = false;
    QString m_pigmentCodeSystem = QStringLiteral("mengying");   // 色母编号体系（默认猛鹰）
    /** 修色后按 AC 组分的内部编号拉取一次的 vendor-codes 原始 data，切品牌仅本地映射 */
    QJsonObject m_correctedVendorData;
    QString m_correctedVendorFetchKey;
    /** 各层罐容量/数量列的单位（跨表格重建保持） */
    QMap<QString, QString> m_layerUnits;
    /** 修色完成后拉取内部编号 → 品牌编号映射（对标 loadCorrectedVendorData） */
    void loadCorrectedVendorData(const QJsonArray &components);

    // ─── SDX4 设备状态 ────────────────────────────────────────
    QLabel *m_deviceStatusLabel = nullptr;   // SDX4 连接状态指示
    QString m_lastMeasurementId;             // 最近一次测量上传后返回的 ID
    QString m_lastMeasurementUuid;           // 最近一次测量上传后返回的 UUID
    bool m_lastMeasurementHasTexture = false; // 最近一次测量是否带上了纹理指标
    int m_measureCount = 0;                  // 本次会话测量次数

    // 测量弹窗（同时只允许一个，打开时复用）
    QPointer<QWidget> m_measureDlg;
    QLabel *m_measureDlgStatus = nullptr;    // 弹窗内设备连接状态
    QLabel *m_measureDlgHint = nullptr;      // 弹窗内进度/结果提示
    QPushButton *m_measureDlgBtn = nullptr;  // 弹窗内测量按钮
    QSpinBox *m_measureDlgCount = nullptr;   // 弹窗内测量次数
    QComboBox *m_measureDlgMode = nullptr;   // 弹窗内数据模式
    bool m_measurePending = false;           // 等设备连上后再开始测量

    // 自动修色状态（searchGeneration 用于重新搜索时丢弃过期回填）
    int m_searchGeneration = 0;
    int m_autoGeneration = 0;
    QList<AutoCorrectBatch> m_autoBatches;
    int m_autoBatchIdx = 0;
    qint64 m_autoSdkCalsetId = 0;      // 发起自动修色时捕获的梯度 ID
    bool m_autoHasSdkCalset = false;

    // 分数阈值（从后端 S&C 配置加载）
    ScoreThreshold m_solidInitial;
    ScoreThreshold m_solidCorrected;
    ScoreThreshold m_effectInitial;
    ScoreThreshold m_effectCorrected;
    int m_scoreDecimalPlaces = 2;
    QJsonArray m_currentBaseRows;     // 当前配方底层色母明细
    QJsonArray m_currentTopRows;      // 当前配方面层色母明细（三工序）
    QJsonArray m_correctedBaseRows;   // 修色后底层色母明细
    QJsonArray m_correctedTopRows;    // 修色后面层色母明细
    bool m_hasCorrection = false;     // 是否已修色
    QString m_selectedFormulaCol = QStringLiteral("initial");  // 选中的配方列：initial / ac / acEdited
    QJsonArray m_searchRowDetails;     // 搜色结果自带的 details（含 rgb 颜色）

    // 右侧面板独立高度控制
    QScrollArea *m_mainScroll = nullptr;
    // 左栏滚动区（内容变高后需要按宽度重算最小高度，否则底部按钮划不到）
    QScrollArea *m_sidebarScroll = nullptr;
    QWidget *m_sidebarContainer = nullptr;
    bool m_sidebarSyncPending = false;
    QList<QWidget *> m_panelWidgets;    // 三个面板（搜索结果/配方/可视化）
    QWidget *m_recipePanel = nullptr;   // 配方面板（可折叠）
    QWidget *m_resultSpinner = nullptr; // 搜索结果加载转圈（SpinnerOverlay，macOS 风格）
};
