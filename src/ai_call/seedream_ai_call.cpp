#include "seedream_ai_call.h"
#include <QDebug>
#include <QJsonArray>
#include <stdexcept>

// 默认模型名称（火山引擎方舟 Seedream 5.0）
static const QString kDefaultModel  = "doubao-seedream-5-0-260128";
static const QString kDefaultApiUrl = "https://ark.cn-beijing.volces.com/api/v3/images/generations";

SeedreamAiCall::SeedreamAiCall(const QString &apiKey, QObject *parent)
    : AICallBase(parent)
    , m_apiKey(apiKey)
    , m_apiUrl(kDefaultApiUrl)
    , m_model(kDefaultModel)
{
    // 图片生成耗时较长，默认超时 150 秒
    m_timeout = 150000;
}

void SeedreamAiCall::setApiKey(const QString &key)
{
    if (!key.isEmpty())
        m_apiKey = key;
}

void SeedreamAiCall::setModel(const QString &model)
{
    if (!model.isEmpty())
        m_model = model;
}

void SeedreamAiCall::setApiUrl(const QString &url)
{
    if (!url.isEmpty())
        m_apiUrl = url;
}

bool SeedreamAiCall::sendRequest(const QString &prompt, const QJsonObject &params)
{
    if (m_apiKey.isEmpty()) {
        setError(AIErrorType::ParamError, "API Key 不能为空");
        return false;
    }
    if (prompt.isEmpty()) {
        setError(AIErrorType::ParamError, "提示词不能为空");
        return false;
    }

    cancelRequest();

    QNetworkRequest request = buildRequest();
    QByteArray body = buildRequestBody(prompt, params);
    if (body.isEmpty())
        return false;

    m_currentReply = m_networkManager->post(request, body);
    QNetworkReply *reply = m_currentReply;

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->AICallBase::onReplyFinished(reply);
    });
    connect(reply, &QNetworkReply::errorOccurred, this,
            [this](QNetworkReply::NetworkError errorCode) {
                this->AICallBase::onReplyError(errorCode);
            });

    m_timeoutTimer->start(m_timeout);
    return true;
}

// ── 构建 HTTP 请求头 ──────────────────────────────────────
QNetworkRequest SeedreamAiCall::buildRequest()
{
    QNetworkRequest request;
    request.setUrl(QUrl(m_apiUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization",
                         QStringLiteral("Bearer %1").arg(m_apiKey).toUtf8());
    return request;
}

// ── 构建请求体 ────────────────────────────────────────────
// 请求体格式（文生图 / 图生图统一端点）：
// {
//   "model":         "doubao-seedream-5-0-260128",  // 必填
//   "prompt":        "<用户提示词>",                 // 必填
//   "image":         "<url 或 base64>",              // 可选，有则为图生图
//   "size":          "2K",                           // 可选，如 "1K"/"2K"/"4K" 或 "WxH"
//   "output_format": "png",                          // 可选，"png"/"jpeg"/"webp"
//   "watermark":     false,                          // 可选
//   "seed":          12345                           // 可选
// }
QByteArray SeedreamAiCall::buildRequestBody(const QString &prompt, const QJsonObject &params)
{
    QJsonObject body;
    body["model"]  = m_model;
    body["prompt"] = prompt;

    // 可选：参考图片（字符串，非数组）
    QString img = params.value("img").toString();
    if (!img.isEmpty())
        body["image"] = img;

    // 可选：图片尺寸，如 "2K"、"1024x1024"
    QString size = params.value("size").toString();
    if (!size.isEmpty())
        body["size"] = size;

    // 可选：输出格式，默认 "png"
    body["output_format"] = params.value("output_format").toString("png");

    // 可选：水印，默认 false
    body["watermark"] = params.value("watermark").toBool(false);

    // 可选：随机种子（>= 0 时才写入）
    if (params.contains("seed")) {
        int seed = params.value("seed").toInt(-1);
        if (seed >= 0)
            body["seed"] = seed;
    }

    const QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);
    //qDebug() << "[SeedreamAiCall] 请求体：" << json;
    return json;
}

// ── 解析响应 ──────────────────────────────────────────────
// 成功响应格式：
// {
//   "model":   "doubao-seedream-5-0-260128",
//   "created": 1757323224,
//   "data": [
//     { "url": "https://...", "size": "1760x2368" }
//   ],
//   "usage": { "generated_images": 1, "output_tokens": 16280, "total_tokens": 16280 }
// }
// 错误响应格式：
// {
//   "error": { "code": "...", "message": "...", "param": null, "type": "..." }
// }
QString SeedreamAiCall::parseResponse(const QByteArray &responseData)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError)
        throw std::runtime_error(parseError.errorString().toStdString());

    if (!doc.isObject())
        throw std::runtime_error("响应不是有效的 JSON 对象");

    QJsonObject root = doc.object();

    // API 层错误
    if (root.contains("error")) {
        QJsonObject err = root["error"].toObject();
        QString code    = err["code"].toString();
        QString msg     = err["message"].toString("未知 API 错误");
        throw std::runtime_error(
            QStringLiteral("[%1] %2").arg(code, msg).toStdString());
    }

    QJsonArray data = root["data"].toArray();
    if (data.isEmpty())
        throw std::runtime_error("响应中 data 数组为空");

    QString url = data.first().toObject()["url"].toString();
    if (url.isEmpty())
        throw std::runtime_error("响应中 url 字段为空");

    qDebug() << "[SeedreamAiCall] 生成图片 URL：" << url;
    return url;
}

