/* ============================================================
   I18nManager.h —— 国际化翻译管理器（单例）
   - 从 JSON 文件加载多语言翻译
   - 提供 tr(key) 全局翻译函数
   - 支持运行时切换语言，通过 languageChanged 信号通知 UI 刷新
   ============================================================ */

#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QJsonObject>

class QJsonArray;

class I18nManager : public QObject
{
    Q_OBJECT
public:
    static I18nManager &instance();

    /// 获取翻译文本，key 为翻译键名，fallback 为找不到时的回退文本（通常用中文）
    QString tr(const QString &key, const QString &fallback = QString()) const;

    /// 当前语言代码
    QString currentLanguage() const;

    /// 切换语言，发射 languageChanged 信号
    void setLanguage(const QString &langCode);

    /// 所有支持的语言: code -> displayName
    QMap<QString, QString> supportedLanguages() const;

signals:
    void languageChanged();

private:
    explicit I18nManager(QObject *parent = nullptr);
    void loadLanguage(const QString &langCode);
    void buildInMemoryTranslations();

    QString m_currentLang;
    QMap<QString, QString> m_translations;  // key -> translated text
    QMap<QString, QString> m_fallbackZh;    // 中文兜底：缺键时显示中文而非键名
};

/// 全局快捷翻译函数
/// 用函数而非宏：Qt 头文件里大量使用 T 作为模板参数名，宏会在其之后展开导致编译失败
inline QString T(const char *key)
{
    const QString k = QString::fromUtf8(key);
    return I18nManager::instance().tr(k, k);
}

inline QString T2(const char *key, const QString &fallback)
{
    return I18nManager::instance().tr(QString::fromUtf8(key), fallback);
}

/// 业务数据名称按界面语言取值：中文界面用中文字段，其余语言优先用后端的英文名字段
/// 英文名字段缺失时回退中文，避免出现空白单元格
inline QString Tn(const QString &cn, const QString &en)
{
    if (I18nManager::instance().currentLanguage() == QLatin1String("zh")) return cn;
    return en.isEmpty() ? cn : en;
}

inline QString Tn(const QJsonObject &obj, const char *cnKey, const char *enKey)
{
    return Tn(obj.value(QString::fromUtf8(cnKey)).toString(),
              obj.value(QString::fromUtf8(enKey)).toString());
}
