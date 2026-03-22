#include "qwen_ai_call.h"
#include <QJsonArray>
#include <stdexcept>

QwenAICall::QwenAICall(const QString &apiKey, QObject *parent)
    : AICallBase(parent)
    , m_apiKey(apiKey)
    , m_apiUrl("https://dashscope.aliyuncs.com/api/v1/services/aigc/multimodal-generation/generation")
{

}

bool QwenAICall::sendRequest(const QString &prompt, const QJsonObject &params)
{
    // 参数校验
    if (m_apiKey.isEmpty()) {
        setError(AIErrorType::ParamError, "API Key 不能为空");
        return false;
    }

    if (prompt.isEmpty()) {
        setError(AIErrorType::ParamError, "提示词不能为空");
        return false;
    }

    // 取消之前的请求
    cancelRequest();

    // 构建请求
    QNetworkRequest request = buildRequest();
    QByteArray requestBody = buildRequestBody(prompt, params);
    // 发送POST请求
    m_currentReply = m_networkManager->post(request, requestBody);
    // 用局部变量捕获当前 reply，避免 cancelRequest() 将 m_currentReply 置空后
    // lambda 中持有悬空指针导致 use-after-free
    QNetworkReply *reply = m_currentReply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->AICallBase::onReplyFinished(reply);
    });
    connect(reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError errorCode) {
        this->AICallBase::onReplyError(errorCode);
    });

    // 启动超时定时器
    m_timeoutTimer->start(m_timeout);

    return true;
}

void QwenAICall::setApiUrl(const QString &url)
{
    if (!url.isEmpty()) {
        m_apiUrl = url;
    }
}

QNetworkRequest QwenAICall::buildRequest()
{
    QNetworkRequest request;
    request.setUrl(QUrl(m_apiUrl));

    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
    // 可选：添加User-Agent（部分API要求）
    request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux aarch64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/102.0.5005.200 Safari/537.36 Qaxbrowser");
    return request;
}

QByteArray QwenAICall::buildRequestBody(const QString &prompt, const QJsonObject &params)
{
    QJsonObject requestJson;

    // 默认参数
    requestJson["model"] =  "wan2.6-image";

    // 构建消息体
    QJsonObject input;
    QJsonArray messages;
    QJsonArray content;
    QJsonObject message;
    QJsonObject promptInfo;
    QJsonObject imgInfo;
    message["role"] = "user";
    promptInfo["text"] = prompt;
    imgInfo["image"] = params["img"];
    content.append(promptInfo);
    content.append(imgInfo);
    message["content"] = content;
    messages.append(message);
    input["messages"] = messages;
    requestJson["input"] = input;

    // 转换为JSON字节数组
    return QJsonDocument(requestJson).toJson(QJsonDocument::Compact);
}

QString QwenAICall::parseResponse(const QByteArray &responseData)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        throw std::runtime_error(parseError.errorString().toStdString());
    }

    if (!doc.isObject()) {
        throw std::runtime_error("响应不是有效的JSON对象");
    }

    QJsonObject rootObj = doc.object();

    // 检查API错误
    if (rootObj.contains("error")) {
        QJsonObject errorObj = rootObj["error"].toObject();
        QString errorMsg = errorObj["message"].toString("未知API错误");
        throw std::runtime_error(errorMsg.toStdString());
    }

    // 解析返回内容
    QJsonObject data = rootObj["output"].toObject();
    QJsonArray choices = data["choices"].toArray();
    if (choices.isEmpty()) {
        throw std::runtime_error("响应中无有效内容");
    }

    QJsonObject choiceObj = choices.first().toObject();
    QJsonObject messageObj = choiceObj["message"].toObject();
    QJsonArray content = messageObj["content"].toArray();
    QJsonObject imgdata = content.first().toObject();
    QString img = imgdata["image"].toString();
    if (img.isEmpty()) {
        throw std::runtime_error("响应内容为空");
    }

    return img;
}

bool QwenAICall::setApiKey(QString apiKey)
{
    m_apiKey = apiKey;
    return true;
}
