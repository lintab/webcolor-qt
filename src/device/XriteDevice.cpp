/* ============================================================
   XriteDevice.cpp —— X-Rite SDX4 分光光度计包装类实现
   通过 QLibrary 动态加载 MAC.dll，实现 USB 连接、测量、光谱读取。
   ============================================================ */

#include "XriteDevice.h"
#include "utils/I18nManager.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QStringList>

// ─── 单例 ─────────────────────────────────────────────────────

XriteDevice &XriteDevice::instance()
{
    static XriteDevice inst;
    return inst;
}

// ─── 构造 / 析构 ──────────────────────────────────────────────

XriteDevice::XriteDevice(QObject *parent)
    : QObject(parent)
{
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(POLL_INTERVAL_MS);
    connect(m_pollTimer, &QTimer::timeout, this, &XriteDevice::onPollDataReady);

    loadDll();
}

XriteDevice::~XriteDevice()
{
    if (m_available && fnIsConnected && fnIsConnected()) {
        if (fnDisconnect) fnDisconnect();
    }
    freeDll();
}

// ─── DLL 加载 ─────────────────────────────────────────────────

bool XriteDevice::loadDll()
{
    if (m_available) return true;

    // MAC.dll 应位于可执行文件同目录或 resources/sdx4/ 下
    const QString appDir = QApplication::applicationDirPath();
    QStringList candidates = {
        appDir + QStringLiteral("/MAC.dll"),
        appDir + QStringLiteral("/resources/sdx4/MAC.dll"),
        QStringLiteral("MAC.dll"),   // PATH 搜索
    };

    bool anyCandidateFound = false;
    QString loadError;

    for (const QString &path : candidates) {
        if (!QFileInfo::exists(path)) continue;
        anyCandidateFound = true;

        m_lib.setFileName(path);
        if (!m_lib.load()) {
            loadError = QStringLiteral("%1（%2）").arg(m_lib.errorString(), QFileInfo(path).absoluteFilePath());
            qWarning() << "[XriteDevice] MAC.dll 加载失败:" << loadError;
            continue;
        }
        qInfo() << "[XriteDevice] 已加载 MAC.dll:" << path;
        if (resolveFunctions()) {
            m_available = true;
            m_driverStatus = DriverStatus::Ready;
            m_driverError.clear();
            return true;
        }
        // resolveFunctions 已写入 m_driverError（缺少的符号名）
        loadError = m_driverError;
        qWarning() << "[XriteDevice] MAC.dll 符号解析失败:" << loadError;
        m_lib.unload();
    }

    m_available = false;
    if (anyCandidateFound) {
        m_driverStatus = DriverStatus::LoadFailed;
        m_driverError = loadError;
        qWarning() << "[XriteDevice] SDX4 驱动加载失败:" << loadError;
    } else {
        m_driverStatus = DriverStatus::NotFound;
        m_driverError.clear();
        qWarning() << "[XriteDevice] 未找到 MAC.dll，SDX4 设备功能不可用";
    }
    return false;
}

void XriteDevice::freeDll()
{
    if (m_lib.isLoaded()) {
        m_lib.unload();
    }
    m_available = false;
    m_driverStatus = DriverStatus::NotFound;
    m_driverError.clear();
}

bool XriteDevice::resolveFunctions()
{
    // 逐一解析所有必需的函数符号
    #define RESOLVE(name, type, field) \
        field = reinterpret_cast<type>(m_lib.resolve(#name)); \
        if (!field) { \
            m_driverError = QStringLiteral("MAC.dll export not found: %1").arg(QLatin1String(#name)); \
            qWarning() << "[XriteDevice] 未解析到:" << #name; \
            return false; \
        }

    RESOLVE(Connect,              _Connect,              fnConnect);
    RESOLVE(Disconnect,           _Disconnect,           fnDisconnect);
    RESOLVE(IsConnected,          _IsConnected,          fnIsConnected);
    RESOLVE(GetSpectralSetCount,  _GetSpectralSetCount,  fnGetSpectralSetCount);
    RESOLVE(GetWavelengthCount,   _GetWavelengthCount,   fnGetWavelengthCount);
    RESOLVE(GetSpectralSetName,   _GetSpectralSetName,   fnGetSpectralSetName);
    RESOLVE(GetWavelengthValue,   _GetWavelengthValue,   fnGetWavelengthValue);
    RESOLVE(Measure,              _Measure,              fnMeasure);
    RESOLVE(GetSpectralData,      _GetSpectralData,      fnGetSpectralData);
    RESOLVE(IsDataReady,          _IsDataReady,          fnIsDataReady);
    RESOLVE(GetSerialNum,         _GetSerialNum,         fnGetSerialNum);
    RESOLVE(GetCalStatus,         _GetCalStatus,         fnGetCalStatus);
    RESOLVE(GetLastErrorString,   _GetLastErrorString,   fnGetLastErrorString);
    RESOLVE(GetLastErrorCode,     _GetLastErrorCode,     fnGetLastErrorCode);

    // 可选函数（不影响基本功能）
    fnGetAvailableSettings = reinterpret_cast<_GetAvailableSettings>(m_lib.resolve("GetAvailableSettings"));
    fnGetSettingOptions    = reinterpret_cast<_GetSettingOptions>(m_lib.resolve("GetSettingOptions"));
    fnGetOption            = reinterpret_cast<_GetOption>(m_lib.resolve("GetOption"));
    fnSetOption            = reinterpret_cast<_SetOption>(m_lib.resolve("SetOption"));
    fnExecute              = reinterpret_cast<_Execute>(m_lib.resolve("Execute"));
    fnExecuteBinary        = reinterpret_cast<_ExecuteBinary>(m_lib.resolve("ExecuteBinary"));

    #undef RESOLVE

    qInfo() << "[XriteDevice] 所有必需函数符号解析成功";
    return true;
}

// ─── 公共接口 ─────────────────────────────────────────────────

bool XriteDevice::isAvailable() const
{
    return m_available;
}

bool XriteDevice::isConnected() const
{
    if (!m_available || !fnIsConnected) return false;
    return fnIsConnected();
}

QString XriteDevice::serialNumber() const
{
    if (!m_available || !fnGetSerialNum) return {};
    return QString::fromLatin1(fnGetSerialNum());
}

QString XriteDevice::lastError() const
{
    if (!m_available || !fnGetLastErrorString) return T("dev_unknown_error");
    return QString::fromLatin1(fnGetLastErrorString());
}

void XriteDevice::connectDevice()
{
    if (!m_available) {
        emit errorOccurred(T("dev_dll_not_loaded"));
        return;
    }
    if (fnIsConnected && fnIsConnected()) {
        qInfo() << "[XriteDevice] 设备已连接，序列号:" << serialNumber();
        enableTextureAndPreview();
        emit connected();
        return;
    }

    // 设置连接方式为 USB（必须在 Connect() 之前调用）
    if (fnSetOption) {
        qInfo() << "[XriteDevice] 设置连接方式为 USB";
        fnSetOption("Connection_Method", "USB");
    }

    qInfo() << "[XriteDevice] 正在连接设备...";
    if (fnConnect && fnConnect()) {
        qInfo() << "[XriteDevice] 设备连接成功，序列号:" << serialNumber();
        enableTextureAndPreview();
        emit connected();
    } else {
        QString err = lastError();
        qWarning() << "[XriteDevice] 设备连接失败:" << err;
        emit errorOccurred(T("dev_connect_failed") + err);
    }
}

void XriteDevice::disconnectDevice()
{
    if (!m_available) return;
    if (fnIsConnected && fnIsConnected()) {
        fnDisconnect();
        qInfo() << "[XriteDevice] 设备已断开";
        emit disconnected();
    }
}

void XriteDevice::measure()
{
    measureAveraged(1);
}

void XriteDevice::measureAveraged(int times)
{
    if (!m_available) {
        emit errorOccurred(T("dev_dll_missing"));
        return;
    }
    if (!fnIsConnected || !fnIsConnected()) {
        emit errorOccurred(T("dev_not_connected"));
        return;
    }

    // 检查校准状态（0 = 已校准）
    if (fnGetCalStatus && fnGetCalStatus() != 0) {
        emit errorOccurred(T("dev_not_calibrated"));
        return;
    }

    m_shotsTotal = qMax(1, times);
    m_shotsDone = 0;
    m_accumKeys.clear();
    m_accumSets.clear();

    if (!triggerMeasure()) return;

    // 启动轮询等待数据就绪
    m_pollCount = 0;
    m_pollTimer->start();
}

bool XriteDevice::triggerMeasure()
{
    qInfo() << "[XriteDevice] 触发测量（" << (m_shotsDone + 1) << "/" << m_shotsTotal << "）...";
    if (!fnMeasure || !fnMeasure()) {
        QString err = lastError();
        qWarning() << "[XriteDevice] 测量触发失败:" << err;
        resetSequence();
        emit errorOccurred(T("dev_trigger_failed") + err);
        return false;
    }
    return true;
}

void XriteDevice::accumulateShot(const QList<SpectralSet> &shot)
{
    for (const auto &set : shot) {
        int idx = m_accumKeys.indexOf(set.geometryCode);
        if (idx < 0) {
            m_accumKeys.append(set.geometryCode);
            m_accumSets.append(set);
            continue;
        }
        SpectralSet &acc = m_accumSets[idx];
        for (int i = 0; i < acc.values.size() && i < set.values.size(); ++i) {
            acc.values[i] += set.values[i];
        }
    }
}

QList<XriteDevice::SpectralSet> XriteDevice::averagedResult() const
{
    QList<SpectralSet> out;
    for (const SpectralSet &acc : m_accumSets) {
        SpectralSet set = acc;
        for (double &v : set.values) {
            v /= m_shotsDone;
        }
        out.append(set);
    }
    return out;
}

void XriteDevice::resetSequence()
{
    m_shotsTotal = 1;
    m_shotsDone = 0;
    m_accumKeys.clear();
    m_accumSets.clear();
}

// ─── 内部：轮询测量数据 ───────────────────────────────────────

void XriteDevice::onPollDataReady()
{
    m_pollCount++;

    if (m_pollCount > POLL_TIMEOUT_COUNT) {
        m_pollTimer->stop();
        qWarning() << "[XriteDevice] 测量超时（" << (POLL_TIMEOUT_COUNT * POLL_INTERVAL_MS / 1000) << "秒）";
        emit errorOccurred(T("dev_measure_timeout"));
        return;
    }

    if (!fnIsDataReady || !fnIsDataReady()) {
        return;  // 继续轮询
    }

    // 数据就绪，停止轮询
    m_pollTimer->stop();
    qInfo() << "[XriteDevice] 测量数据就绪，读取光谱...";

    // 读取所有角度/几何条件下的光谱数据
    QList<SpectralSet> spectra;
    int setCount = fnGetSpectralSetCount ? fnGetSpectralSetCount() : 0;
    int wlCount  = fnGetWavelengthCount  ? fnGetWavelengthCount()  : 0;

    if (setCount <= 0 || wlCount <= 0) {
        emit errorOccurred(T("dev_read_spectral_failed"));
        return;
    }

    qInfo() << "[XriteDevice] 光谱集数量:" << setCount << "波长点数:" << wlCount;

    for (int s = 0; s < setCount; ++s) {
        SpectralSet set;

        // 获取几何编码
        const char *nameRaw = fnGetSpectralSetName ? fnGetSpectralSetName(s) : nullptr;
        QString setName = nameRaw ? QString::fromLatin1(nameRaw) : QString::number(s);
        set.geometryCode = toGeometryCode(setName);

        // 获取波长范围和步长
        int firstWl = fnGetWavelengthValue ? fnGetWavelengthValue(0) : 400;
        int lastWl  = fnGetWavelengthValue ? fnGetWavelengthValue(wlCount - 1) : 700;
        set.startWavelength = firstWl;
        set.endWavelength   = lastWl;
        set.step = (wlCount > 1) ? ((lastWl - firstWl) / (wlCount - 1)) : 10;

        // 读取每个波长的反射率
        set.values.reserve(wlCount);
        for (int w = 0; w < wlCount; ++w) {
            float val = fnGetSpectralData ? fnGetSpectralData(s, w) : 0.0f;
            set.values.append(static_cast<double>(val));
        }

        // 检测数据范围：如果值在 0-1 范围内，转换为百分制 (0-100)
        // 后端 UploadMeasurementRequestDTO 期望百分制数据
        bool needConvert = true;
        for (double v : set.values) {
            if (v > 1.5) {  // 如果有值大于 1.5，说明已经是百分制
                needConvert = false;
                break;
            }
        }
        if (needConvert) {
            qInfo() << "[XriteDevice] 角度" << set.geometryCode << "检测到 0-1 范围数据，转换为百分制";
            for (int i = 0; i < set.values.size(); ++i) {
                set.values[i] *= 100.0;
            }
        }

        qInfo() << "[XriteDevice] 角度" << set.geometryCode
                << "波长范围" << set.startWavelength << "-" << set.endWavelength << "nm"
                << "值数量:" << set.values.size()
                << "示例值:" << (set.values.size() > 0 ? set.values[0] : 0);

        spectra.append(set);
    }

    qInfo() << "[XriteDevice] 本次测量读数完成，共" << spectra.size() << "个光谱集";

    // 纹理与色块图片不能取平均，连测时以最后一帧为准
    m_lastTextures = readTextures();
    m_lastPreviews = readPreviewImages();

    m_shotsDone++;
    emit measurementProgress(m_shotsDone, m_shotsTotal);
    accumulateShot(spectra);

    if (m_shotsDone < m_shotsTotal) {
        // 继续下一次测量
        if (!triggerMeasure()) return;
        m_pollCount = 0;
        m_pollTimer->start();
        return;
    }

    const QList<SpectralSet> averaged = averagedResult();
    qInfo() << "[XriteDevice] 连测" << m_shotsTotal << "次取平均完成，光谱集数量:" << averaged.size();
    resetSequence();
    emit measurementReady(averaged);
}

// ─── 内部：光谱集名称 → 后端几何编码映射 ──────────────────────

QString XriteDevice::toGeometryCode(const QString &spectralSetName) const
{
    // MAC.dll 返回的名称格式如 "r45as15"、"r45as25"、"r45as45"、"r45as75"、"r45as110"
    // 后端期望的 geometryCode 格式如 "45as15"、"45as25" 等（去掉前缀 'r'）
    QString code = spectralSetName.toLower().trimmed();

    // 去掉可能的 "r" 前缀
    if (code.startsWith(QLatin1Char('r'))) {
        code = code.mid(1);
    }

    // 如果名称格式不标准，尝试直接返回原始名称（去掉前缀后）
    if (code.isEmpty()) {
        code = spectralSetName;
    }

    return code;
}

// ─── 内部：XDS4 透传命令 ───────────────────────────────────────

QString XriteDevice::runCommand(const QString &cmd) const
{
    if (!fnExecute) return {};

    const QByteArray raw = cmd.toLatin1();
    const char *ret = fnExecute(raw.constData());
    // 驱动返回的缓冲区由驱动持有，必须立即拷走
    return ret ? QString::fromLatin1(ret).trimmed() : QString();
}

void XriteDevice::enableTextureAndPreview()
{
    if (!fnExecute) return;

    // 机型不支持 HE 时这两条会失败，只记日志，不影响光谱测量
    qInfo() << "[XriteDevice] GetExtendedOption|JPEGExport:"
            << runCommand(QStringLiteral("GetExtendedOption|JPEGExport"));

    for (const char *condition : { "Texture", "PreviewFiles" }) {
        const QString ret = runCommand(
            QStringLiteral("SetMeasurementConditionValue|%1|ON").arg(QLatin1String(condition)));
        if (!ret.startsWith(QStringLiteral("<00>"))) {
            qWarning() << "[XriteDevice] 开启测量条件" << condition
                       << "失败:" << (ret.isEmpty() ? lastError() : ret);
        }
    }
}

QList<XriteDevice::TextureAngle> XriteDevice::readTextures()
{
    QList<TextureAngle> out;
    if (!fnExecute) return out;

    const int imageSetCount = runCommand(QStringLiteral("GetImageSetCount")).toInt();
    if (imageSetCount <= 0) return out;

    for (int i = 0; i < imageSetCount; ++i) {
        const QString angle = runCommand(QStringLiteral("GetImageSetName|%1").arg(i));
        const int paramCount = runCommand(QStringLiteral("GetTextureParameterCount|%1").arg(i)).toInt();
        if (angle.isEmpty() || paramCount <= 0) continue;

        TextureAngle ta;
        ta.angle = angle;
        for (int p = 0; p < paramCount; ++p) {
            const QString name  = runCommand(QStringLiteral("GetTextureParameterName|%1|%2").arg(i).arg(p));
            const QString value = runCommand(QStringLiteral("GetTextureParameterValue|%1|%2").arg(i).arg(p));
            bool ok = false;
            const double v = value.toDouble(&ok);
            if (name.isEmpty() || !ok) continue;

            ta.metrics.insert(name, v);
            if (ta.version.isEmpty()) {
                ta.version = runCommand(QStringLiteral("GetTextureParameterVersion|%1|%2").arg(i).arg(p));
            }
        }

        qInfo() << "[XriteDevice] 纹理" << ta.angle << "版本" << ta.version
                << "指标数" << ta.metrics.size();
        out.append(ta);
    }
    return out;
}

QList<XriteDevice::PreviewImage> XriteDevice::readPreviewImages()
{
    QList<PreviewImage> out;
    if (!fnExecute || !fnExecuteBinary) return out;

    const int imageSetCount = runCommand(QStringLiteral("GetImageSetCount")).toInt();
    for (int i = 0; i < imageSetCount; ++i) {
        const QStringList parts =
            runCommand(QStringLiteral("GetPreviewFileTypeAndSize|%1").arg(i)).split(QLatin1Char('|'));
        if (parts.size() < 2) continue;

        bool ok = false;
        const int size = parts.at(1).toInt(&ok);
        if (!ok || size <= 0) continue;

        long received = 0;
        QByteArray buf;
        // 命令手册写的是 GetPreviewImageData，官方示例用的是 GetPreviewFileData，两种都试
        for (const char *cmdName : { "GetPreviewFileData", "GetPreviewImageData" }) {
            const QByteArray cmd = QStringLiteral("%1|%2|%3")
                                       .arg(QLatin1String(cmdName)).arg(i).arg(size).toLatin1();
            buf = QByteArray(size, '\0');
            received = fnExecuteBinary(cmd.constData(), 0, 0, buf.data(), size);
            if (received > 0) break;
        }

        if (received <= 0) {
            qWarning() << "[XriteDevice] 色块图片读取失败，角度索引" << i << ":" << lastError();
            continue;
        }

        PreviewImage pi;
        pi.geometryCode = runCommand(QStringLiteral("GetImageSetName|%1").arg(i));
        pi.format       = parts.at(0).toLower();
        pi.data         = buf.left(static_cast<int>(received));
        qInfo() << "[XriteDevice] 色块图片" << pi.geometryCode << pi.format << pi.data.size() << "字节";
        out.append(pi);
    }
    return out;
}
