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

    int  selectedModel() const;
    void setSelectedModel(int index);

    // 语言配置（"zh_CN" 或 "en"）
    QString language() const;
    void    setLanguage(const QString &lang);

    // 截图保存路径
    QString savePath() const;
    void    setSavePath(const QString &path);

private:
    QString m_apiKey;
    int     m_selectedModel = 0;
    QString m_language      = "zh_CN";
    QString m_savePath;
};

#endif // APP_SETTINGS_H
