#include "ai_call_base.h"

AICallBase::AICallBase(QObject *parent)
    : QObject(parent)
    , m_lastErrorType(AIErrorType::NoError)
    , m_timeout(30000)  // 默认30秒超时
    , m_timeoutTimer(new QTimer(this))
    , m_currentReply(nullptr)
{
    // 初始化网络管理器
    m_networkManager = new QNetworkAccessManager(this);

    // 连接超时定时器
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &AICallBase::onRequestTimeout);
}

AICallBase::~AICallBase()
{
    cancelRequest();
}

void AICallBase::setTimeout(int msec)
{
    if (msec > 0) {
        m_timeout = msec;
    }
}

QString AICallBase::lastErrorString() const
{
    return m_lastErrorString;
}

AIErrorType AICallBase::lastErrorType() const
{
    return m_lastErrorType;
}

void AICallBase::cancelRequest()
{
    // 停止超时定时器
    m_timeoutTimer->stop();

    // 取消当前请求
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    // 重置错误信息
    m_lastErrorType = AIErrorType::NoError;
    m_lastErrorString.clear();
}

void AICallBase::setError(AIErrorType errorType, const QString &errorMsg)
{
    m_lastErrorType = errorType;
    m_lastErrorString = errorMsg;
    emit requestFailed(errorType, errorMsg);
}

void AICallBase::onReplyFinished(QNetworkReply *reply)
{
    // 停止超时定时器
    m_timeoutTimer->stop();

    // 清理Reply
    reply->deleteLater();
    m_currentReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        setError(AIErrorType::NetworkError, reply->errorString());
        return;
    }

    // 读取响应数据
    QByteArray responseData = reply->readAll();

    // 解析响应（子类实现）
    QString result;
    try {
        result = parseResponse(responseData);
    } catch (const std::exception &e) {
        setError(AIErrorType::ApiError, tr("响应解析失败：%1").arg(e.what()));
        return;
    } catch (...) {
        setError(AIErrorType::ApiError, tr("响应解析失败：未知错误"));
        return;
    }

    // 发送成功信号
    emit requestSuccess(result);
}

void AICallBase::onReplyError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    // 错误处理在onReplyFinished中统一处理
}

void AICallBase::onRequestTimeout()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    setError(AIErrorType::TimeoutError, tr("请求超时（%1毫秒）").arg(m_timeout));
}
