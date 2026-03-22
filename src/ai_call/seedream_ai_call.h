#ifndef SEEDREAM_AI_CALL_H
#define SEEDREAM_AI_CALL_H

#include <QObject>
#include "ai_call_base.h"

// 图片生成请求参数（对应火山引擎方舟 /api/v3/images/generations）
struct SeedreamRequestParams {
    // 参考图片 URL 或 Base64（image-to-image 时传入；纯文生图可留空）
    QString imageUrl;

    // 图片尺寸，格式 "宽x高"，如 "1024x1024"，默认空表示由模型决定
    QString size;

    // 生成数量，默认 1，最大值视模型而定
    int    n             = 1;

    // 输出格式："png" / "jpeg" / "webp"，默认 "png"
    QString outputFormat = "png";

    // 是否添加水印，默认 false
    bool   watermark     = false;

    // 引导强度（图生图时有效），范围 0.0~1.0，默认 0.5
    double guidanceScale = 0.5;

    // 随机种子，-1 表示随机
    int    seed          = -1;
};

class SeedreamAiCall : public AICallBase
{
    Q_OBJECT

public:
    explicit SeedreamAiCall(const QString &apiKey,
                            QObject       *parent = nullptr);

    // 设置 API Key（覆盖构造时传入的值）
    void setApiKey(const QString &key);

    // 设置模型名称，默认 "doubao-seedream-5-0-260128"
    void setModel(const QString &model);

    // 覆盖 API 地址（一般不需要修改）
    void setApiUrl(const QString &url);

    // 发起请求
    // prompt : 文字提示词
    // params : QJsonObject，支持以下 key：
    //   "img"           - 参考图片 URL/Base64（图生图）
    //   "size"          - 尺寸字符串
    //   "n"             - 生成数量
    //   "output_format" - 输出格式
    //   "watermark"     - bool
    //   "guidance_scale"- double
    //   "seed"          - int
    bool sendRequest(const QString     &prompt,
                     const QJsonObject &params = QJsonObject()) override;

protected:
    QNetworkRequest buildRequest() override;
    QByteArray      buildRequestBody(const QString     &prompt,
                                     const QJsonObject &params) override;
    QString         parseResponse(const QByteArray &responseData) override;

private:
    QString m_apiKey;
    QString m_apiUrl;
    QString m_model;
};

#endif // SEEDREAM_AI_CALL_H
