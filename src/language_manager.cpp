#include "language_manager.h"

#include <QCoreApplication>
#include <QProcess>
#include <QDebug>

LanguageManager::LanguageManager(AppSettings &settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
    // 启动时加载翻译
    loadTranslator(m_settings.language());
}

QString LanguageManager::language() const
{
    return m_settings.language();
}

void LanguageManager::setLanguage(const QString &lang)
{
    if (m_settings.language() == lang) return;
    m_settings.setLanguage(lang);
    emit languageChanged(lang);
    restartApp();
}

void LanguageManager::restartApp()
{
    // 用当前可执行文件路径和参数重新启动，然后退出当前进程
    QProcess::startDetached(QCoreApplication::applicationFilePath(),
                            QCoreApplication::arguments());
    QCoreApplication::quit();
}

void LanguageManager::loadTranslator(const QString &lang)
{
    // 中文是源语言：不加载翻译文件，qsTr() 直接返回中文源字符串
    if (lang == "zh_CN") return;

    m_translator = new QTranslator(this);
    QString qmFile = QString(":/i18n/app_%1.qm").arg(lang);
    if (!m_translator->load(qmFile)) {
        qmFile = QString("app_%1.qm").arg(lang);
        if (!m_translator->load(qmFile, QCoreApplication::applicationDirPath())) {
            qWarning() << "[LanguageManager] 翻译文件未找到：" << qmFile;
            delete m_translator;
            m_translator = nullptr;
            return;
        }
    }
    QCoreApplication::installTranslator(m_translator);
}
