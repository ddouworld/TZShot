#include "viewmodel/vision_view_model.h"

#include "ai_call/doubao_vision_call.h"
#include "model/app_settings.h"
#include "sticky_image_store.h"

#include <QBuffer>

namespace {
constexpr int kMaxImageEdge = 1600;

QString defaultVisionModelForProvider(int provider)
{
    switch (provider) {
    case 1:
        return QStringLiteral("qwen-vl-plus");
    case 0:
    default:
        return QStringLiteral("doubao-vision-pro-32k-2410128");
    }
}

QString apiUrlForProvider(int provider)
{
    switch (provider) {
    case 1:
        return QStringLiteral("https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions");
    case 0:
    default:
        return QStringLiteral("https://ark.cn-beijing.volces.com/api/v3/chat/completions");
    }
}
}

VisionViewModel::VisionViewModel(AppSettings &settings,
                                 StickyImageStore &store,
                                 QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_store(store)
    , m_call(new DoubaoVisionCall(settings.visionApiKey(), this))
{
    syncCallSettings();
    syncProxySettings();
    connect(m_call, &AICallBase::requestSuccess, this, [this](const QString &response) {
        setLoading(false);
        emit analysisSucceeded(m_pendingImageUrl, m_pendingPrompt, response, m_pendingImage);
    });
    connect(m_call, &AICallBase::requestFailed, this,
            [this](AIErrorType errorType, const QString &errorMsg) {
        QString prefix;
        switch (errorType) {
        case AIErrorType::NetworkError: prefix = QStringLiteral("网络错误："); break;
        case AIErrorType::ApiError: prefix = QStringLiteral("API 错误："); break;
        case AIErrorType::TimeoutError: prefix = QStringLiteral("超时错误："); break;
        case AIErrorType::ParamError: prefix = QStringLiteral("参数错误："); break;
        default: break;
        }
        setLoading(false);
        emit analysisFailed(m_pendingImageUrl, m_pendingPrompt, prefix + errorMsg);
    });
}

QString VisionViewModel::apiKey() const
{
    return m_settings.visionApiKey();
}

bool VisionViewModel::isLoading() const
{
    return m_loading;
}

int VisionViewModel::provider() const
{
    return m_settings.visionProvider();
}

QString VisionViewModel::model() const
{
    return m_settings.visionModel();
}

bool VisionViewModel::proxyEnabled() const
{
    return m_settings.visionProxyEnabled();
}

int VisionViewModel::proxyType() const
{
    return m_settings.visionProxyType();
}

QString VisionViewModel::proxyHost() const
{
    return m_settings.visionProxyHost();
}

int VisionViewModel::proxyPort() const
{
    return m_settings.visionProxyPort();
}

bool VisionViewModel::setApiKey(const QString &key)
{
    if (m_settings.visionApiKey() == key) {
        return true;
    }
    m_settings.setVisionApiKey(key);
    if (m_call) {
        m_call->setApiKey(key);
    }
    emit apiKeyChanged(key);
    return true;
}

void VisionViewModel::setProvider(int provider)
{
    if (provider < 0) {
        provider = 0;
    }
    if (m_settings.visionProvider() == provider) {
        return;
    }

    m_settings.setVisionProvider(provider);
    m_settings.setVisionModel(defaultVisionModelForProvider(provider));
    syncCallSettings();
}

void VisionViewModel::setModel(const QString &model)
{
    const QString normalizedModel = model.trimmed();
    if (normalizedModel.isEmpty()) {
        return;
    }
    if (m_settings.visionModel() == normalizedModel) {
        return;
    }

    m_settings.setVisionModel(normalizedModel);
    syncCallSettings();
}

void VisionViewModel::setProxyEnabled(bool enabled)
{
    m_settings.setVisionProxyEnabled(enabled);
    syncProxySettings();
}

void VisionViewModel::setProxyType(int type)
{
    m_settings.setVisionProxyType(type);
    syncProxySettings();
}

void VisionViewModel::setProxyHost(const QString &host)
{
    m_settings.setVisionProxyHost(host);
    syncProxySettings();
}

void VisionViewModel::setProxyPort(int port)
{
    m_settings.setVisionProxyPort(port);
    syncProxySettings();
}

void VisionViewModel::analyzeStickyImage(const QString &prompt, const QString &imageUrl)
{
    if (m_loading) {
        emit analysisFailed(imageUrl, prompt, QStringLiteral("已有视觉分析任务正在进行中"));
        return;
    }
    if (prompt.trimmed().isEmpty()) {
        emit analysisFailed(imageUrl, prompt, QStringLiteral("提示词不能为空"));
        return;
    }

    const QImage originalImage = m_store.getImageByUrl(imageUrl);
    if (originalImage.isNull()) {
        emit analysisFailed(imageUrl, prompt, QStringLiteral("读取图片失败"));
        return;
    }

    const QImage normalizedImage = normalizeImage(originalImage);
    const QString imageDataUrl = imageToDataUrl(normalizedImage);
    if (imageDataUrl.isEmpty()) {
        emit analysisFailed(imageUrl, prompt, QStringLiteral("图片编码失败"));
        return;
    }

    m_pendingImageUrl = imageUrl;
    m_pendingPrompt = prompt.trimmed();
    m_pendingImage = normalizedImage;
    emit analysisStarted(m_pendingImageUrl, m_pendingPrompt, m_pendingImage);
    setLoading(true);

    const bool ok = m_call->sendRequest(m_pendingPrompt, QJsonObject {
        { QStringLiteral("image_url"), imageDataUrl }
    });
    if (!ok) {
        setLoading(false);
        emit analysisFailed(m_pendingImageUrl,
                            m_pendingPrompt,
                            QStringLiteral("请求发起失败：") + m_call->lastErrorString());
    }
}

void VisionViewModel::setLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    emit isLoadingChanged(loading);
}

void VisionViewModel::syncCallSettings()
{
    if (!m_call) {
        return;
    }

    QString modelName = m_settings.visionModel().trimmed();
    if (modelName.isEmpty()) {
        modelName = defaultVisionModelForProvider(m_settings.visionProvider());
        m_settings.setVisionModel(modelName);
    }

    m_call->setApiUrl(apiUrlForProvider(m_settings.visionProvider()));
    m_call->setModel(modelName);
}

void VisionViewModel::syncProxySettings()
{
    if (!m_call) {
        return;
    }
    m_call->setProxy(m_settings.visionProxyEnabled(),
                     m_settings.visionProxyType(),
                     m_settings.visionProxyHost(),
                     m_settings.visionProxyPort());
}

QString VisionViewModel::imageToDataUrl(const QImage &image) const
{
    if (image.isNull()) {
        return {};
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG")) {
        return {};
    }
    return QStringLiteral("data:image/png;base64,%1")
        .arg(QString::fromLatin1(bytes.toBase64()));
}

QImage VisionViewModel::normalizeImage(const QImage &image) const
{
    if (image.isNull()) {
        return {};
    }

    if (image.width() <= kMaxImageEdge && image.height() <= kMaxImageEdge) {
        return image;
    }

    return image.scaled(kMaxImageEdge,
                        kMaxImageEdge,
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation);
}
