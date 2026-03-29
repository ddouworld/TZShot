#include "ai_view_model.h"
#include "ai_call/qwen_ai_call.h"
#include "ai_call/seedream_ai_call.h"
#include "ai_call/ai_type.h"
#include "sticky_image_store.h"

#include <QBuffer>
#include <QDebug>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

AIViewModel::AIViewModel(AppSettings &settings,
                         StickyImageStore &store,
                         QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_store(store)
{
    rebuildAiCall();
}

QString AIViewModel::apiKey() const       { return m_settings.apiKey(); }
int     AIViewModel::selectedModel() const { return m_settings.selectedModel(); }
bool    AIViewModel::isLoading() const     { return m_loading; }

void AIViewModel::setLoading(bool v)
{
    if (m_loading == v) return;
    m_loading = v;
    emit isLoadingChanged(v);
}

// ── 工厂：根据 selectedModel 创建/重建 AI 调用实例 ──────────
void AIViewModel::rebuildAiCall()
{
    if (m_aiCall) {
        m_aiCall->cancelRequest();
        m_aiCall->deleteLater();
        m_aiCall = nullptr;
    }

    const QString key = m_settings.apiKey();

    switch (m_settings.selectedModel()) {
    case SEEDREAMAI: {
        m_aiCall = new SeedreamAiCall(key, this);
        qInfo() << "[AIViewModel] 使用模型：火山引擎 Seedream";
        break;
    }
    case QWENAI:
    default: {
        m_aiCall = new QwenAICall(key, this);
        qInfo() << "[AIViewModel] 使用模型：阿里云百炼 Qwen";
        break;
    }
    }

    // requestSuccess 只表示 AI 接口返回了图片 URL，
    // 真正完成还需要下载该图片（在 sendPrompt 的 lambda 里处理）
    connect(m_aiCall, &AICallBase::requestFailed, this,
            [this](AIErrorType errorType, const QString &errorMsg) {
        QString prefix;
        switch (errorType) {
        case AIErrorType::NetworkError:  prefix = AIViewModel::tr("网络错误：");  break;
        case AIErrorType::ApiError:      prefix = AIViewModel::tr("API 错误：");  break;
        case AIErrorType::TimeoutError:  prefix = AIViewModel::tr("超时错误：");  break;
        case AIErrorType::ParamError:    prefix = AIViewModel::tr("参数错误：");  break;
        default:                         prefix = AIViewModel::tr("未知错误：");  break;
        }
        const QString msg = prefix + errorMsg;
        qWarning() << "[AIViewModel]" << msg;
        setLoading(false);
        emit signalRequestFailed(msg);
    });
}

// ── setApiKey ────────────────────────────────────────────────
bool AIViewModel::setApiKey(const QString &key)
{
    if (m_settings.apiKey() == key) return true;
    m_settings.setApiKey(key);
    rebuildAiCall();
    emit apiKeyChanged(key);
    return true;
}

// ── setSelectedModel ─────────────────────────────────────────
void AIViewModel::setSelectedModel(int index)
{
    if (m_settings.selectedModel() == index) return;
    m_settings.setSelectedModel(index);
    rebuildAiCall();
    emit selectedModelChanged(index);
}

// ── sendPrompt ───────────────────────────────────────────────
// 流程：
//   1. 从 StickyImageStore 取出原图，编码为 base64 传给 AI
//   2. AI 返回新图片的网络 URL（requestSuccess）
//   3. 用内置 QNetworkAccessManager 下载该图片
//   4. 存入 StickyImageStore，emit signalRequestComplete(新 image://sticky/... URL)
void AIViewModel::sendPrompt(const QString &prompt, const QString &imageUrl)
{
    if (m_loading) {
        qWarning() << "[AIViewModel] 已有请求进行中，忽略本次调用";
        return;
    }
    if (prompt.trimmed().isEmpty()) {
        emit signalRequestFailed(tr("提示词不能为空"));
        return;
    }

    // 从 Store 取原图并编码为 base64（API 接受 data URI）
    QString imgParam;
    {
        const QString prefix = QStringLiteral("image://sticky/");
        QString id = imageUrl.startsWith(prefix)
                     ? imageUrl.mid(prefix.size())
                     : QString();
        if (!id.isEmpty()) {
            QImage img = m_store.getImage(id);
            if (!img.isNull()) {
                QByteArray ba;
                QBuffer buf(&ba);
                buf.open(QIODevice::WriteOnly);
                img.save(&buf, "PNG");
                imgParam = QStringLiteral("data:image/png;base64,")
                           + QString::fromLatin1(ba.toBase64());
            }
        }
    }

    if (imgParam.isEmpty()) {
        imgParam = imageUrl;
        qWarning() << "[AIViewModel] 未能从 Store 获取原图，回退使用 imageUrl";
    }

    setLoading(true);

    // 断开上次 requestSuccess 的连接（避免重建前残留信号叠加）
    disconnect(m_aiCall, &AICallBase::requestSuccess, this, nullptr);

    // 捕获原图 URL，用于 signalRequestComplete 让各窗口识别是否是自己的请求
    const QString originalUrl = imageUrl;

    connect(m_aiCall, &AICallBase::requestSuccess, this,
            [this, originalUrl](const QString &aiImageUrl) {
        qDebug() << "[AIViewModel] AI 返回图片 URL：" << aiImageUrl;

        auto *dlManager = new QNetworkAccessManager(this);
        QNetworkReply *reply = dlManager->get(QNetworkRequest(QUrl(aiImageUrl)));

        connect(reply, &QNetworkReply::finished, this,
                [this, reply, dlManager, originalUrl]() {
            reply->deleteLater();
            dlManager->deleteLater();
            setLoading(false);

            if (reply->error() != QNetworkReply::NoError) {
                const QString msg = tr("下载生成图片失败：")
                                    + reply->errorString();
                qWarning() << "[AIViewModel]" << msg;
                emit signalRequestFailed(msg);
                return;
            }

            QImage newImage;
            if (!newImage.load(reply, nullptr)) {
                const QString msg = tr("生成图片解码失败");
                qWarning() << "[AIViewModel]" << msg;
                emit signalRequestFailed(msg);
                return;
            }

            const QString newStickyUrl = m_store.storeImage(newImage);
            qDebug() << "[AIViewModel] 新贴图 URL：" << newStickyUrl;
            // 同时发出原图 URL，让各贴图窗口判断是否是自己触发的请求
            emit signalRequestComplete(originalUrl, newStickyUrl);
        });
    });

    bool ok = m_aiCall->sendRequest(prompt, QJsonObject{ {"img", imgParam} });
    if (!ok) {
        setLoading(false);
        disconnect(m_aiCall, &AICallBase::requestSuccess, this, nullptr);
        emit signalRequestFailed(
            tr("请求发起失败：") + m_aiCall->lastErrorString());
    }
}
