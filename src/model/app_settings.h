#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <QString>
#include <QUrl>

// AppSettings — Model
// -------------------
// 负责应用配置数据的读写（QSettings 持久化）。
// 不继承 QObject，不依赖 QML，纯数据存取。
// 所有 setter 在值变更时自动写入 QSettings。

class AppSettings
{
public:
    AppSettings();

    // AI 配置
    QString apiKey() const;
    void    setApiKey(const QString &key);
    QString visionApiKey() const;
    void    setVisionApiKey(const QString &key);
    int     visionProvider() const;
    void    setVisionProvider(int provider);
    QString visionModel() const;
    void    setVisionModel(const QString &model);
    bool    visionWebSearchEnabled() const;
    void    setVisionWebSearchEnabled(bool enabled);
    bool    visionProxyEnabled() const;
    void    setVisionProxyEnabled(bool enabled);
    int     visionProxyType() const;
    void    setVisionProxyType(int type);
    QString visionProxyHost() const;
    void    setVisionProxyHost(const QString &host);
    int     visionProxyPort() const;
    void    setVisionProxyPort(int port);

    int  selectedModel() const;
    void setSelectedModel(int index);

    // 语言配置（"zh_CN" 或 "en"）
    QString language() const;
    void    setLanguage(const QString &lang);

    // 截图保存路径
    QString savePath() const;
    void    setSavePath(const QString &path);

    // GIF 体积质量预设：0=高质量，1=均衡，2=小体积
    int  gifQualityPreset() const;
    void setGifQualityPreset(int preset);

private:
    QString m_apiKey;
    QString m_visionApiKey;
    int     m_visionProvider = 0;
    QString m_visionModel = QStringLiteral("doubao-vision-pro-32k-2410128");
    bool    m_visionWebSearchEnabled = true;
    bool    m_visionProxyEnabled = false;
    int     m_visionProxyType = 0;
    QString m_visionProxyHost = QStringLiteral("127.0.0.1");
    int     m_visionProxyPort = 7890;
    int     m_selectedModel = 0;
    QString m_language      = "zh_CN";
    QString m_savePath;
    int     m_gifQualityPreset = 1;
};

#endif // APP_SETTINGS_H
