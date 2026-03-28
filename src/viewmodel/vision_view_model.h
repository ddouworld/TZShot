#ifndef VISION_VIEW_MODEL_H
#define VISION_VIEW_MODEL_H

#include <QObject>
#include <QImage>

#include "ai_call/ai_call_base.h"

class AppSettings;
class StickyImageStore;
class DoubaoVisionCall;

class VisionViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString apiKey READ apiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    explicit VisionViewModel(AppSettings &settings,
                             StickyImageStore &store,
                             QObject *parent = nullptr);

    QString apiKey() const;
    bool isLoading() const;
    int provider() const;
    QString model() const;
    bool webSearchEnabled() const;
    bool proxyEnabled() const;
    int proxyType() const;
    QString proxyHost() const;
    int proxyPort() const;

    Q_INVOKABLE bool setApiKey(const QString &key);
    Q_INVOKABLE void setProvider(int provider);
    Q_INVOKABLE void setModel(const QString &model);
    Q_INVOKABLE void setWebSearchEnabled(bool enabled);
    Q_INVOKABLE void setProxyEnabled(bool enabled);
    Q_INVOKABLE void setProxyType(int type);
    Q_INVOKABLE void setProxyHost(const QString &host);
    Q_INVOKABLE void setProxyPort(int port);
    Q_INVOKABLE void analyzeStickyImage(const QString &prompt, const QString &imageUrl);

signals:
    void apiKeyChanged(const QString &apiKey);
    void isLoadingChanged(bool loading);
    void webSearchEnabledChanged(bool enabled);
    void analysisStarted(const QString &imageUrl, const QString &prompt, const QImage &image);
    void analysisDelta(const QString &imageUrl, const QString &prompt, const QString &delta);
    void analysisSucceeded(const QString &imageUrl,
                           const QString &prompt,
                           const QString &result,
                           const QImage &image);
    void analysisFailed(const QString &imageUrl, const QString &prompt, const QString &errorMsg);

private:
    void syncCallSettings();
    void setLoading(bool loading);
    void syncProxySettings();
    QString imageToDataUrl(const QImage &image) const;
    QImage normalizeImage(const QImage &image) const;

    AppSettings &m_settings;
    StickyImageStore &m_store;
    DoubaoVisionCall *m_call = nullptr;
    bool m_loading = false;
    QString m_pendingImageUrl;
    QString m_pendingPrompt;
    QImage m_pendingImage;
};

#endif // VISION_VIEW_MODEL_H
