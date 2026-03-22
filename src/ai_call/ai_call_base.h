#ifndef AICALLBASE_H
#define AICALLBASE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QUrl>

// AI 调用错误类型枚举
enum class AIErrorType {
    NoError,            // 无错误
    NetworkError,       // 网络错误
    ApiError,           // API 返回错误
    TimeoutError,       // 请求超时
    ParamError          // 参数错误
};

// AI 调用基类（抽象类）
class AICallBase : public QObject
{
    Q_OBJECT
public:
    explicit AICallBase(QObject *parent = nullptr);
    virtual ~AICallBase();

    // 设置请求超时时间（默认30秒）
    void setTimeout(int msec);

    // 获取最后一次错误信息
    QString lastErrorString() const;

    // 获取最后一次错误类型
    AIErrorType lastErrorType() const;

    /**
     * @brief 发送AI请求（核心接口，子类实现）
     * @param prompt 提示词/问题
     * @param params 额外参数（不同AI接口的自定义参数）
     * @return 是否成功发起请求
     */
    virtual bool sendRequest(const QString &prompt, const QJsonObject &params = QJsonObject()) = 0;

    /**
     * @brief 取消当前请求
     */
    void cancelRequest();

signals:
    // 请求成功，返回响应内容
    void requestSuccess(const QString &response);
    // 请求失败，返回错误类型和错误信息
    void requestFailed(AIErrorType errorType, const QString &errorMsg);
    // 请求进度（可选，如流式响应）
    void requestProgress(int progress);

protected:
    // 构建请求头（子类实现）
    virtual QNetworkRequest buildRequest() = 0;

    // 构建请求体（子类实现）
    virtual QByteArray buildRequestBody(const QString &prompt, const QJsonObject &params) = 0;

    // 解析响应数据（子类实现）
    virtual QString parseResponse(const QByteArray &responseData) = 0;

    // 设置错误信息
    void setError(AIErrorType errorType, const QString &errorMsg);

    // 网络请求管理器（所有子类共享）
    QNetworkAccessManager *m_networkManager;

public slots:
    // 网络响应处理
    virtual void onReplyFinished(QNetworkReply *reply);
    // 网络错误处理
    virtual void onReplyError(QNetworkReply::NetworkError error);
    // 请求超时处理
    virtual void onRequestTimeout();

public:
    QString m_lastErrorString;      // 最后一次错误信息
    AIErrorType m_lastErrorType;    // 最后一次错误类型
    int m_timeout;                  // 超时时间（毫秒）
    QTimer *m_timeoutTimer;         // 超时定时器
    QNetworkReply *m_currentReply;  // 当前请求的Reply
};

#endif // AICALLBASE_H
