#ifndef DOUBAO_VISION_CALL_H
#define DOUBAO_VISION_CALL_H

#include "ai_call_base.h"

#include <QNetworkProxy>

class DoubaoVisionCall : public AICallBase
{
    Q_OBJECT

public:
    explicit DoubaoVisionCall(const QString &apiKey, QObject *parent = nullptr);

    bool sendRequest(const QString &prompt, const QJsonObject &params = QJsonObject()) override;
    void setApiKey(const QString &apiKey);
    void setApiUrl(const QString &apiUrl);
    void setModel(const QString &model);
    void setWebSearchEnabled(bool enabled);
    void setProxy(bool enabled, int proxyType, const QString &host, int port);

protected:
    QNetworkRequest buildRequest() override;
    QByteArray buildRequestBody(const QString &prompt, const QJsonObject &params) override;
    QString parseResponse(const QByteArray &responseData) override;
    void onReplyFinished(QNetworkReply *reply) override;

private:
    void resetStreamState();
    void onReplyReadyRead(QNetworkReply *reply);
    void processStreamBuffer(bool flushTail = false);
    void processStreamEvent(const QByteArray &eventBlock);

    QString m_apiKey;
    bool m_proxyEnabled = false;
    int m_proxyType = 0;
    QString m_proxyHost;
    int m_proxyPort = 0;
    bool m_webSearchEnabled = false;
    bool m_streamEnabled = false;
    QString m_model = QStringLiteral("doubao-vision-pro-32k-2410128");
    QString m_apiUrl = QStringLiteral("https://ark.cn-beijing.volces.com/api/v3/chat/completions");
    QByteArray m_streamBuffer;
    QJsonObject m_streamCompletedResponse;
    QString m_streamAccumulatedText;
    QString m_streamErrorMessage;
};

#endif // DOUBAO_VISION_CALL_H
