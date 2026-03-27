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
    void setProxy(bool enabled, int proxyType, const QString &host, int port);

protected:
    QNetworkRequest buildRequest() override;
    QByteArray buildRequestBody(const QString &prompt, const QJsonObject &params) override;
    QString parseResponse(const QByteArray &responseData) override;

private:
    QString m_apiKey;
    bool m_proxyEnabled = false;
    int m_proxyType = 0;
    QString m_proxyHost;
    int m_proxyPort = 0;
    QString m_model = QStringLiteral("doubao-vision-pro-32k-2410128");
    QString m_apiUrl = QStringLiteral("https://ark.cn-beijing.volces.com/api/v3/chat/completions");
};

#endif // DOUBAO_VISION_CALL_H
