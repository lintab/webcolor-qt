/* ============================================================
   XriteDevice.h —— X-Rite SDX4 分光光度计包装类
   通过 QLibrary 动态加载 MAC.dll（XDS4 驱动），提供 USB 连接、
   测量、光谱数据读取等功能，替代 Device Bridge 握手流程。
   ============================================================ */

#pragma once

#include <QObject>
#include <QLibrary>
#include <QTimer>
#include <QString>
#include <QList>
#include <QMap>

class XriteDevice : public QObject
{
    Q_OBJECT
public:
    /** 驱动可用性：区分「文件没找到」和「找到了但加载/解析失败」 */
    enum class DriverStatus { Ready, NotFound, LoadFailed };

    /** 单个角度/几何条件下的光谱反射率数据 */
    struct SpectralSet {
        QString geometryCode;   // 后端几何编码，如 "45as15"
        int startWavelength;    // 起始波长（nm），通常 400
        int endWavelength;      // 结束波长（nm），通常 700
        int step;               // 波长步长（nm），通常 10
        QList<double> values;   // 反射率值列表（100% = 100.00）
    };

    /** 一个图像角度下的纹理指标（SDX4 HE 机型），名称保持 SDK 原样 */
    struct TextureAngle {
        QString angle;                  // 图像角度名，如 "r15as15"
        QString version;                // 纹理算法版本，如 "5.2"
        QMap<QString, double> metrics;  // SG/CV/SD/SC/CO/DC/C → 数值
    };

    /** 一个角度下的色块相机图（JPEG 原始字节） */
    struct PreviewImage {
        QString geometryCode;   // 与 TextureAngle::angle 同一套角度名
        QString format;         // "jpg"
        QByteArray data;
    };

    static XriteDevice &instance();

    /** DLL 是否成功加载（MAC.dll 存在且函数符号解析完毕） */
    bool isAvailable() const;
    /** 驱动当前状态（未找到 / 加载失败 / 可用） */
    DriverStatus driverStatus() const { return m_driverStatus; }
    /** 加载或符号解析失败的具体原因（供界面提示排障用） */
    QString driverError() const { return m_driverError; }
    /** 设备是否已连接 */
    bool isConnected() const;
    /** 设备序列号（连接后可用） */
    QString serialNumber() const;
    /** 最近一次错误描述 */
    QString lastError() const;
    /** 最近一次测量读到的纹理数据（HE 机型才有；低配机型为空） */
    QList<TextureAngle> lastTextures() const { return m_lastTextures; }
    /** 最近一次测量读到的色块图片（HE 机型才有；低配机型为空） */
    QList<PreviewImage> lastPreviewImages() const { return m_lastPreviews; }

    /** 连接设备（USB） */
    void connectDevice();
    /** 断开设备 */
    void disconnectDevice();
    /** 触发测量（异步：启动后通过 measurementReady 信号返回结果） */
    void measure();
    /** 连测 times 次并取平均，结束后只发一次 measurementReady（times<=1 等同 measure()） */
    void measureAveraged(int times);

signals:
    void connected();
    void disconnected();
    /** 测量完成，返回所有角度的光谱数据 */
    void measurementReady(const QList<XriteDevice::SpectralSet> &spectra);
    /** 连测进度：已完成 done 次，共 total 次 */
    void measurementProgress(int done, int total);
    /** 发生错误 */
    void errorOccurred(const QString &msg);

private:
    explicit XriteDevice(QObject *parent = nullptr);
    ~XriteDevice() override;
    Q_DISABLE_COPY(XriteDevice)

    bool loadDll();
    void freeDll();
    bool resolveFunctions();
    QString toGeometryCode(const QString &spectralSetName) const;

    // ─── DLL 动态加载 ────────────────────────────────────────
    QLibrary m_lib;
    bool m_available = false;
    DriverStatus m_driverStatus = DriverStatus::NotFound;
    QString m_driverError;

    // ─── MAC.dll 导出函数指针（stdcall 调用约定） ─────────────
    // 基础类型定义
    typedef bool        (__stdcall *_Connect)();
    typedef bool        (__stdcall *_Disconnect)();
    typedef bool        (__stdcall *_IsConnected)();
    typedef int         (__stdcall *_GetSpectralSetCount)();
    typedef int         (__stdcall *_GetWavelengthCount)();
    typedef const char* (__stdcall *_GetSpectralSetName)(int);
    typedef int         (__stdcall *_GetWavelengthValue)(int);
    typedef bool        (__stdcall *_Measure)();
    typedef float       (__stdcall *_GetSpectralData)(int, int);
    typedef bool        (__stdcall *_IsDataReady)();
    typedef const char* (__stdcall *_GetSerialNum)();
    typedef int         (__stdcall *_GetCalStatus)();
    typedef const char* (__stdcall *_GetLastErrorString)();
    typedef int         (__stdcall *_GetLastErrorCode)();
    typedef const char* (__stdcall *_GetAvailableSettings)();
    typedef const char* (__stdcall *_GetSettingOptions)(const char*);
    typedef const char* (__stdcall *_GetOption)(const char*);
    typedef bool        (__stdcall *_SetOption)(const char*, const char*);
    typedef const char* (__stdcall *_Execute)(const char*);
    typedef long        (__stdcall *_ExecuteBinary)(const char*, long, long, char*, long);

    _Connect                fnConnect               = nullptr;
    _Disconnect             fnDisconnect            = nullptr;
    _IsConnected            fnIsConnected           = nullptr;
    _GetSpectralSetCount    fnGetSpectralSetCount   = nullptr;
    _GetWavelengthCount     fnGetWavelengthCount    = nullptr;
    _GetSpectralSetName     fnGetSpectralSetName    = nullptr;
    _GetWavelengthValue     fnGetWavelengthValue    = nullptr;
    _Measure                fnMeasure               = nullptr;
    _GetSpectralData        fnGetSpectralData       = nullptr;
    _IsDataReady            fnIsDataReady           = nullptr;
    _GetSerialNum           fnGetSerialNum          = nullptr;
    _GetCalStatus           fnGetCalStatus          = nullptr;
    _GetLastErrorString     fnGetLastErrorString    = nullptr;
    _GetLastErrorCode       fnGetLastErrorCode      = nullptr;
    _GetAvailableSettings   fnGetAvailableSettings  = nullptr;
    _GetSettingOptions      fnGetSettingOptions     = nullptr;
    _GetOption              fnGetOption             = nullptr;
    _SetOption              fnSetOption             = nullptr;
    _Execute                fnExecute               = nullptr;
    _ExecuteBinary          fnExecuteBinary         = nullptr;

    // ─── 测量轮询状态 ────────────────────────────────────────
    QTimer *m_pollTimer = nullptr;
    int m_pollCount = 0;
    static constexpr int POLL_INTERVAL_MS = 100;
    static constexpr int POLL_TIMEOUT_COUNT = 300;  // 30 秒超时

    void onPollDataReady();

    // ─── 连测取平均状态 ──────────────────────────────────────
    bool triggerMeasure();                    // 单次触发设备测量
    void accumulateShot(const QList<SpectralSet> &shot);
    QList<SpectralSet> averagedResult() const;
    void resetSequence();

    // ─── XDS4 透传命令（纹理 / 色块图片，仅 HE 机型） ─────────
    QString runCommand(const QString &cmd) const;
    void enableTextureAndPreview();           // 打开 Texture / PreviewFiles 测量条件
    QList<TextureAngle> readTextures();
    QList<PreviewImage> readPreviewImages();

    int m_shotsTotal = 1;    // 本轮需要测量的次数
    int m_shotsDone = 0;     // 本轮已完成的次数
    QStringList m_accumKeys;                    // 累加器里的角度顺序
    QList<SpectralSet> m_accumSets;             // 各角度反射率累加值
    QList<TextureAngle> m_lastTextures;         // 最近一次测量的纹理（多帧时为最后一帧）
    QList<PreviewImage> m_lastPreviews;         // 最近一次测量的色块图片
};
