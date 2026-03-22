#ifndef QWENAICALL_H
#define QWENAICALL_H

#include <QObject>
#include "ai_call_base.h"

class QwenAICall:public AICallBase
{
    Q_OBJECT

public:
    explicit QwenAICall(const QString &apiKey, QObject *parent = nullptr);

    // 实现基类纯虚函数
    bool sendRequest(const QString &prompt, const QJsonObject &params = QJsonObject()) override;

    // 设置API地址（默认OpenAI官方地址，可替换为代理地址）
    void setApiUrl(const QString &url);

public:
    QNetworkRequest buildRequest() override;
    QByteArray buildRequestBody(const QString &prompt, const QJsonObject &params) override;
    QString parseResponse(const QByteArray &responseData) override;
public:
    bool setApiKey(QString apiKey);
private:
    QString m_apiKey;    // API Key
    QString m_apiUrl;
};

#endif // QWENAICALL_H
