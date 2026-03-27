#include "doubao_vision_call.h"

#include <QJsonArray>

#include <stdexcept>

DoubaoVisionCall::DoubaoVisionCall(const QString &apiKey, QObject *parent)
    : AICallBase(parent)
    , m_apiKey(apiKey)
{
}

bool DoubaoVisionCall::sendRequest(const QString &prompt, const QJsonObject &params)
{
    if (m_apiKey.isEmpty()) {
        setError(AIErrorType::ParamError, QStringLiteral("视觉 API Key 不能为空"));
        return false;
    }
    if (prompt.trimmed().isEmpty()) {
        setError(AIErrorType::ParamError, QStringLiteral("提示词不能为空"));
        return false;
    }
    if (!params.contains(QStringLiteral("image_url"))
        || params.value(QStringLiteral("image_url")).toString().isEmpty()) {
        setError(AIErrorType::ParamError, QStringLiteral("缺少图片数据"));
        return false;
    }

    cancelRequest();

    const QNetworkRequest request = buildRequest();
    const QByteArray requestBody = buildRequestBody(prompt, params);
    m_currentReply = m_networkManager->post(request, requestBody);
    QNetworkReply *reply = m_currentReply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->AICallBase::onReplyFinished(reply);
    });
    connect(reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError errorCode) {
        this->AICallBase::onReplyError(errorCode);
    });
    m_timeoutTimer->start(m_timeout);
    return true;
}

void DoubaoVisionCall::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

void DoubaoVisionCall::setApiUrl(const QString &apiUrl)
{
    if (!apiUrl.trimmed().isEmpty()) {
        m_apiUrl = apiUrl.trimmed();
    }
}

void DoubaoVisionCall::setModel(const QString &model)
{
    if (!model.trimmed().isEmpty()) {
        m_model = model.trimmed();
    }
}

void DoubaoVisionCall::setProxy(bool enabled, int proxyType, const QString &host, int port)
{
    m_proxyEnabled = enabled;
    m_proxyType = proxyType;
    m_proxyHost = host;
    m_proxyPort = port;

    if (!m_networkManager) {
        return;
    }

    if (!m_proxyEnabled || m_proxyHost.trimmed().isEmpty() || m_proxyPort <= 0) {
        m_networkManager->setProxy(QNetworkProxy::NoProxy);
        return;
    }

    QNetworkProxy proxy;
    proxy.setType(m_proxyType == 1 ? QNetworkProxy::Socks5Proxy : QNetworkProxy::HttpProxy);
    proxy.setHostName(m_proxyHost.trimmed());
    proxy.setPort(static_cast<quint16>(m_proxyPort));
    m_networkManager->setProxy(proxy);
}

QNetworkRequest DoubaoVisionCall::buildRequest()
{
    QNetworkRequest request{QUrl(m_apiUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_apiKey).toUtf8());
    return request;
}

QByteArray DoubaoVisionCall::buildRequestBody(const QString &prompt, const QJsonObject &params)
{
    QJsonObject root;
    root[QStringLiteral("model")] = m_model;

    QJsonObject textPart;
    textPart[QStringLiteral("type")] = QStringLiteral("text");
    textPart[QStringLiteral("text")] = prompt;

    QJsonObject imageUrl;
    imageUrl[QStringLiteral("url")] = params.value(QStringLiteral("image_url")).toString();

    QJsonObject imagePart;
    imagePart[QStringLiteral("type")] = QStringLiteral("image_url");
    imagePart[QStringLiteral("image_url")] = imageUrl;

    QJsonArray content;
    content.append(textPart);
    content.append(imagePart);

    QJsonObject message;
    message[QStringLiteral("role")] = QStringLiteral("user");
    message[QStringLiteral("content")] = content;

    QJsonArray messages;
    messages.append(message);
    root[QStringLiteral("messages")] = messages;

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QString DoubaoVisionCall::parseResponse(const QByteArray &responseData)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        throw std::runtime_error(parseError.errorString().toStdString());
    }
    if (!doc.isObject()) {
        throw std::runtime_error("响应不是有效的JSON对象");
    }

    const QJsonObject rootObj = doc.object();
    if (rootObj.contains(QStringLiteral("error"))) {
        const QJsonObject errorObj = rootObj.value(QStringLiteral("error")).toObject();
        throw std::runtime_error(errorObj.value(QStringLiteral("message"))
                                     .toString(QStringLiteral("未知 API 错误"))
                                     .toStdString());
    }

    const QJsonArray choices = rootObj.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        throw std::runtime_error("响应中无有效内容");
    }

    const QJsonObject messageObj = choices.first().toObject().value(QStringLiteral("message")).toObject();
    const QJsonValue contentVal = messageObj.value(QStringLiteral("content"));
    if (contentVal.isString()) {
        const QString text = contentVal.toString().trimmed();
        if (!text.isEmpty()) {
            return text;
        }
    }

    if (contentVal.isArray()) {
        QStringList parts;
        const QJsonArray contentArray = contentVal.toArray();
        for (const QJsonValue &value : contentArray) {
            const QJsonObject item = value.toObject();
            if (item.value(QStringLiteral("type")).toString() == QStringLiteral("text")) {
                const QString text = item.value(QStringLiteral("text")).toString().trimmed();
                if (!text.isEmpty()) {
                    parts.append(text);
                }
            }
        }
        if (!parts.isEmpty()) {
            return parts.join(QLatin1Char('\n'));
        }
    }

    throw std::runtime_error("响应内容为空");
}
