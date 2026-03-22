#ifndef LANGUAGE_MANAGER_H
#define LANGUAGE_MANAGER_H

#include <QObject>
#include <QTranslator>
#include "model/app_settings.h"

// LanguageManager
// ───────────────
// 启动时根据 AppSettings 中保存的语言加载对应翻译文件。
// setLanguage() 只持久化设置，重启后生效。
// 暴露给 QML 为 O_LanguageManager。

class LanguageManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)

public:
    explicit LanguageManager(AppSettings &settings, QObject *parent = nullptr);

    QString language() const;

    // 保存新语言并立即重启应用
    Q_INVOKABLE void setLanguage(const QString &lang);

signals:
    void languageChanged(const QString &lang);

private:
    void loadTranslator(const QString &lang);
    static void restartApp();

    AppSettings  &m_settings;
    QTranslator  *m_translator = nullptr;
};

#endif // LANGUAGE_MANAGER_H
