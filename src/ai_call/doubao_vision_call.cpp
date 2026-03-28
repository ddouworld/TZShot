#include "doubao_vision_call.h"

#include <QJsonArray>
#include <QStringList>

#include <stdexcept>

namespace {
bool useResponsesApi(const QString &apiUrl)
{
    return apiUrl.contains(QStringLiteral("/responses"));
}

bool useDashScopeApi(const QString &apiUrl)
{
    return apiUrl.contains(QStringLiteral("dashscope.aliyuncs.com"));
}

QString parseChoicesText(const QJsonObject &rootObj)
{
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

QString parseResponsesText(const QJsonObject &rootObj)
{
    const QString outputText = rootObj.value(QStringLiteral("output_text")).toString().trimmed();
    if (!outputText.isEmpty()) {
        return outputText;
    }

    QStringList parts;
    const QJsonArray outputArray = rootObj.value(QStringLiteral("output")).toArray();
    for (const QJsonValue &outputValue : outputArray) {
        const QJsonObject outputObj = outputValue.toObject();
        if (outputObj.value(QStringLiteral("type")).toString() != QStringLiteral("message")) {
            continue;
        }

        const QJsonArray contentArray = outputObj.value(QStringLiteral("content")).toArray();
        for (const QJsonValue &contentValue : contentArray) {
            const QJsonObject contentObj = contentValue.toObject();
            const QString text = contentObj.value(QStringLiteral("text")).toString().trimmed();
            if (!text.isEmpty()) {
                parts.append(text);
            }
        }
    }

    if (!parts.isEmpty()) {
        return parts.join(QLatin1Char('\n'));
    }

    throw std::runtime_error("响应内容为空");
}
}

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
    resetStreamState();
    m_streamEnabled = useResponsesApi(m_apiUrl);

    const QNetworkRequest request = buildRequest();
    const QByteArray requestBody = buildRequestBody(prompt, params);
    m_currentReply = m_networkManager->post(request, requestBody);
    QNetworkReply *reply = m_currentReply;
    if (m_streamEnabled) {
        connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
            onReplyReadyRead(reply);
        });
    }
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
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

void DoubaoVisionCall::setWebSearchEnabled(bool enabled)
{
    m_webSearchEnabled = enabled;
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

    if (useResponsesApi(m_apiUrl)) {
        root[QStringLiteral("stream")] = true;

        if (m_webSearchEnabled) {
            QJsonArray tools;
            if (useDashScopeApi(m_apiUrl)) {
                for (const QString &toolType : { QStringLiteral("web_search"),
                                                 QStringLiteral("web_extractor"),
                                                 QStringLiteral("code_interpreter") }) {
                    QJsonObject tool;
                    tool[QStringLiteral("type")] = toolType;
                    tools.append(tool);
                }
                root[QStringLiteral("enable_thinking")] = true;
            } else {
                QJsonObject webSearchTool;
                webSearchTool[QStringLiteral("type")] = QStringLiteral("web_search");
                tools.append(webSearchTool);
            }
            root[QStringLiteral("tools")] = tools;
        }

        QJsonObject textPart;
        textPart[QStringLiteral("type")] = QStringLiteral("input_text");
        textPart[QStringLiteral("text")] = prompt;

        QJsonObject imagePart;
        imagePart[QStringLiteral("type")] = QStringLiteral("input_image");
        imagePart[QStringLiteral("image_url")] = params.value(QStringLiteral("image_url")).toString();

        QJsonArray content;
        content.append(textPart);
        content.append(imagePart);

        QJsonObject message;
        message[QStringLiteral("role")] = QStringLiteral("user");
        message[QStringLiteral("content")] = content;

        QJsonArray input;
        input.append(message);
        root[QStringLiteral("input")] = input;
        return QJsonDocument(root).toJson(QJsonDocument::Compact);
    }

    if (m_webSearchEnabled) {
        root[QStringLiteral("enable_search")] = true;
    }

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

    if (rootObj.contains(QStringLiteral("choices"))) {
        return parseChoicesText(rootObj);
    }

    if (rootObj.contains(QStringLiteral("output"))) {
        return parseResponsesText(rootObj);
    }

    throw std::runtime_error("响应内容为空");
}

void DoubaoVisionCall::onReplyFinished(QNetworkReply *reply)
{
    if (!m_streamEnabled) {
        AICallBase::onReplyFinished(reply);
        return;
    }

    m_timeoutTimer->stop();

    if (reply && reply->bytesAvailable() > 0) {
        onReplyReadyRead(reply);
    }
    processStreamBuffer(true);

    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorString = reply->errorString();

    reply->deleteLater();
    m_currentReply = nullptr;

    if (networkError != QNetworkReply::NoError) {
        const QString errorMessage = m_streamErrorMessage.isEmpty()
            ? networkErrorString
            : m_streamErrorMessage;
        resetStreamState();
        setError(AIErrorType::NetworkError, errorMessage);
        return;
    }

    QString result = m_streamAccumulatedText;
    if (result.trimmed().isEmpty() && !m_streamCompletedResponse.isEmpty()) {
        try {
            result = parseResponsesText(m_streamCompletedResponse);
        } catch (...) {
        }
    }

    const QString streamErrorMessage = m_streamErrorMessage;
    resetStreamState();

    if (!streamErrorMessage.isEmpty()) {
        setError(AIErrorType::ApiError, streamErrorMessage);
        return;
    }

    if (result.trimmed().isEmpty()) {
        setError(AIErrorType::ApiError, QStringLiteral("响应内容为空"));
        return;
    }

    emit requestSuccess(result);
}

void DoubaoVisionCall::resetStreamState()
{
    m_streamEnabled = false;
    m_streamBuffer.clear();
    m_streamCompletedResponse = QJsonObject();
    m_streamAccumulatedText.clear();
    m_streamErrorMessage.clear();
}

void DoubaoVisionCall::onReplyReadyRead(QNetworkReply *reply)
{
    if (!reply) {
        return;
    }

    const QByteArray chunk = reply->readAll();
    if (chunk.isEmpty()) {
        return;
    }

    m_timeoutTimer->start(m_timeout);
    m_streamBuffer.append(chunk);
    processStreamBuffer(false);
}

void DoubaoVisionCall::processStreamBuffer(bool flushTail)
{
    m_streamBuffer.replace("\r", "");

    while (true) {
        const int separatorIndex = m_streamBuffer.indexOf("\n\n");
        if (separatorIndex < 0) {
            break;
        }

        const QByteArray eventBlock = m_streamBuffer.left(separatorIndex);
        m_streamBuffer.remove(0, separatorIndex + 2);
        processStreamEvent(eventBlock);
    }

    if (flushTail) {
        const QByteArray eventBlock = m_streamBuffer.trimmed();
        m_streamBuffer.clear();
        if (!eventBlock.isEmpty()) {
            processStreamEvent(eventBlock);
        }
    }
}

void DoubaoVisionCall::processStreamEvent(const QByteArray &eventBlock)
{
    if (eventBlock.trimmed().isEmpty()) {
        return;
    }

    QByteArray eventType;
    QByteArray dataPayload;

    const QList<QByteArray> lines = eventBlock.split('\n');
    for (const QByteArray &line : lines) {
        if (line.startsWith("event:")) {
            eventType = line.mid(6);
            if (!eventType.isEmpty() && eventType.startsWith(' ')) {
                eventType.remove(0, 1);
            }
            eventType = eventType.trimmed();
            continue;
        }

        if (line.startsWith("data:")) {
            QByteArray dataLine = line.mid(5);
            if (!dataLine.isEmpty() && dataLine.startsWith(' ')) {
                dataLine.remove(0, 1);
            }
            if (!dataPayload.isEmpty()) {
                dataPayload.append('\n');
            }
            dataPayload.append(dataLine);
        }
    }

    const QByteArray normalizedPayload = dataPayload.trimmed();
    if (normalizedPayload.isEmpty() || normalizedPayload == "[DONE]") {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(normalizedPayload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject rootObj = doc.object();
    QString type = QString::fromUtf8(eventType);
    if (type.isEmpty()) {
        type = rootObj.value(QStringLiteral("type")).toString();
    }

    if (rootObj.contains(QStringLiteral("error"))) {
        const QJsonObject errorObj = rootObj.value(QStringLiteral("error")).toObject();
        m_streamErrorMessage = errorObj.value(QStringLiteral("message"))
                                   .toString(QStringLiteral("流式请求失败"));
        return;
    }

    if (type == QStringLiteral("error") || type == QStringLiteral("response.failed")) {
        m_streamErrorMessage = rootObj.value(QStringLiteral("message"))
                                   .toString(QStringLiteral("流式请求失败"));
        if (m_streamErrorMessage.isEmpty()) {
            const QJsonObject responseObj = rootObj.value(QStringLiteral("response")).toObject();
            const QJsonObject errorObj = responseObj.value(QStringLiteral("error")).toObject();
            m_streamErrorMessage = errorObj.value(QStringLiteral("message"))
                                       .toString(QStringLiteral("流式请求失败"));
        }
        return;
    }

    if (type == QStringLiteral("response.output_text.delta")
        || type == QStringLiteral("response.text.delta")) {
        const QString delta = rootObj.value(QStringLiteral("delta")).toString();
        if (!delta.isEmpty()) {
            m_streamAccumulatedText += delta;
            emit requestDelta(delta);
        }
        return;
    }

    if (type == QStringLiteral("response.completed")) {
        m_streamCompletedResponse = rootObj.value(QStringLiteral("response")).toObject();
        if (m_streamAccumulatedText.trimmed().isEmpty() && !m_streamCompletedResponse.isEmpty()) {
            try {
                m_streamAccumulatedText = parseResponsesText(m_streamCompletedResponse);
            } catch (...) {
            }
        }
    }
}
