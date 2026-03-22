#include "app_settings.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QLocale>

AppSettings::AppSettings()
{
    QSettings s;

    m_apiKey        = s.value("AI/apiKey", "").toString();
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
