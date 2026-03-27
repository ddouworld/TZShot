#include "app_settings.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QLocale>
#include <QtGlobal>

AppSettings::AppSettings()
{
    QSettings s;

    m_apiKey        = s.value("AI/apiKey", "").toString();
    m_visionApiKey  = s.value("Vision/apiKey", "").toString();
    m_visionProvider = s.value("Vision/provider", 0).toInt();
    m_visionModel = s.value("Vision/model", "doubao-vision-pro-32k-2410128").toString();
    m_visionProxyEnabled = s.value("Vision/proxyEnabled", false).toBool();
    m_visionProxyType = s.value("Vision/proxyType", 0).toInt();
    m_visionProxyHost = s.value("Vision/proxyHost", "127.0.0.1").toString();
    m_visionProxyPort = s.value("Vision/proxyPort", 7890).toInt();
    m_selectedModel = s.value("AI/selectedModel", 0).toInt();
    // 语言：若用户从未手动设置过，则根据系统语言选择默认值
    if (s.contains("App/language")) {
        m_language = s.value("App/language").toString();
    } else {
        // QLocale::system().name() 返回形如 "zh_CN"、"en_US" 等
        QString sysLang = QLocale::system().name();  // e.g. "zh_CN", "en_US"
        if (sysLang.startsWith("zh")) {
            m_language = "zh_CN";
        } else {
            m_language = "en";
        }
    }

    QString savedPath = s.value("ImageSaver/savePath").toString();
    if (!savedPath.isEmpty() && QDir(savedPath).exists()) {
        m_savePath = savedPath;
    } else {
        QString picPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        m_savePath = QDir(picPath).filePath("TZshot");
        QDir dir;
        if (!dir.exists(m_savePath)) {
            if (!dir.mkpath(m_savePath)) {
                qWarning() << "[AppSettings] 创建默认保存路径失败，回退到图片根目录";
                m_savePath = picPath;
            }
        }
    }
    qInfo() << "[AppSettings] 保存路径：" << m_savePath;

    const int savedPreset = s.value("GIF/qualityPreset", 1).toInt();
    m_gifQualityPreset = qBound(0, savedPreset, 2);
}

// ── AI 配置 ───────────────────────────────────────────────

QString AppSettings::apiKey() const
{
    return m_apiKey;
}

void AppSettings::setApiKey(const QString &key)
{
    if (m_apiKey == key) return;
    m_apiKey = key;
    QSettings().setValue("AI/apiKey", m_apiKey);
}

QString AppSettings::visionApiKey() const
{
    return m_visionApiKey;
}

void AppSettings::setVisionApiKey(const QString &key)
{
    if (m_visionApiKey == key) return;
    m_visionApiKey = key;
    QSettings().setValue("Vision/apiKey", m_visionApiKey);
}

int AppSettings::visionProvider() const
{
    return m_visionProvider;
}

void AppSettings::setVisionProvider(int provider)
{
    if (m_visionProvider == provider) return;
    m_visionProvider = provider;
    QSettings().setValue("Vision/provider", m_visionProvider);
}

QString AppSettings::visionModel() const
{
    return m_visionModel;
}

void AppSettings::setVisionModel(const QString &model)
{
    if (m_visionModel == model) return;
    m_visionModel = model;
    QSettings().setValue("Vision/model", m_visionModel);
}

bool AppSettings::visionProxyEnabled() const
{
    return m_visionProxyEnabled;
}

void AppSettings::setVisionProxyEnabled(bool enabled)
{
    if (m_visionProxyEnabled == enabled) return;
    m_visionProxyEnabled = enabled;
    QSettings().setValue("Vision/proxyEnabled", m_visionProxyEnabled);
}

int AppSettings::visionProxyType() const
{
    return m_visionProxyType;
}

void AppSettings::setVisionProxyType(int type)
{
    if (m_visionProxyType == type) return;
    m_visionProxyType = type;
    QSettings().setValue("Vision/proxyType", m_visionProxyType);
}

QString AppSettings::visionProxyHost() const
{
    return m_visionProxyHost;
}

void AppSettings::setVisionProxyHost(const QString &host)
{
    if (m_visionProxyHost == host) return;
    m_visionProxyHost = host;
    QSettings().setValue("Vision/proxyHost", m_visionProxyHost);
}

int AppSettings::visionProxyPort() const
{
    return m_visionProxyPort;
}

void AppSettings::setVisionProxyPort(int port)
{
    if (m_visionProxyPort == port) return;
    m_visionProxyPort = port;
    QSettings().setValue("Vision/proxyPort", m_visionProxyPort);
}

int AppSettings::selectedModel() const
{
    return m_selectedModel;
}

void AppSettings::setSelectedModel(int index)
{
    if (m_selectedModel == index) return;
    m_selectedModel = index;
    QSettings().setValue("AI/selectedModel", m_selectedModel);
}

// ── 语言配置 ──────────────────────────────────────────────

QString AppSettings::language() const
{
    return m_language;
}

void AppSettings::setLanguage(const QString &lang)
{
    if (m_language == lang) return;
    m_language = lang;
    QSettings().setValue("App/language", m_language);
}

// ── 保存路径 ──────────────────────────────────────────────

QString AppSettings::savePath() const
{
    return m_savePath;
}

void AppSettings::setSavePath(const QString &path)
{
    if (m_savePath == path || path.isEmpty()) return;
    m_savePath = path;

    QDir dir;
    if (!dir.exists(m_savePath))
        dir.mkpath(m_savePath);

    QSettings().setValue("ImageSaver/savePath", m_savePath);
}

int AppSettings::gifQualityPreset() const
{
    return m_gifQualityPreset;
}

void AppSettings::setGifQualityPreset(int preset)
{
    const int normalized = qBound(0, preset, 2);
    if (m_gifQualityPreset == normalized) return;
    m_gifQualityPreset = normalized;
    QSettings().setValue("GIF/qualityPreset", m_gifQualityPreset);
}
