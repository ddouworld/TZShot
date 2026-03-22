#include "storage_view_model.h"

#include <QImage>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>

StorageViewModel::StorageViewModel(AppSettings &settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    connect(&m_netManager, &QNetworkAccessManager::finished,
            this, [this](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QImage img;
            if (img.loadFromData(data)) {
                img.save(m_pendingTargetUrl.toLocalFile());
            } else {
                qWarning() << "[StorageViewModel] 网络图片解码失败";
            }
        } else {
            qWarning() << "[StorageViewModel] 网络图片下载失败：" << reply->errorString();
        }
        reply->deleteLater();
    });
}

QUrl StorageViewModel::savePath() const
{
    return QUrl::fromLocalFile(m_settings.savePath());
}

void StorageViewModel::setSavePath(const QUrl &path)
{
    QString localPath = path.toLocalFile();
    if (localPath.isEmpty()) localPath = path.toString();
    if (localPath.startsWith("file://"))
        localPath = QUrl(localPath).toLocalFile();

    if (localPath.isEmpty() || m_settings.savePath() == localPath) return;

    m_settings.setSavePath(localPath);
    emit savePathChanged(savePath());
}

bool StorageViewModel::saveImage(const QString &sourceUrl, const QUrl &targetUrl)
{
    if (sourceUrl.isEmpty()) {
        qWarning() << "[StorageViewModel] 源地址为空";
        return false;
    }
    if (targetUrl.isEmpty() || !targetUrl.isLocalFile()) {
        qWarning() << "[StorageViewModel] 目标路径无效";
        return false;
    }

    const QString trimmed = sourceUrl.trimmed();

    // Base64
    if (trimmed.startsWith("data:image/")) {
        int idx = trimmed.indexOf(";base64,");
        if (idx > 0 && trimmed.length() - idx > 8)
            return saveBase64Image(trimmed.mid(idx + 8), targetUrl);
    }

    // 网络图片（异步）
    const QUrl srcUrl(sourceUrl);
    if (srcUrl.isValid() &&
            (srcUrl.scheme() == "http" || srcUrl.scheme() == "https")) {
        saveNetworkImage(sourceUrl, targetUrl);
        return false; // 异步，结果通过信号通知
    }

    // 本地 / QRC
    return saveLocalImage(QUrl::fromUserInput(sourceUrl), targetUrl);
}

bool StorageViewModel::saveBase64Image(const QString &base64Str, const QUrl &targetUrl)
{
    QByteArray data = QByteArray::fromBase64(base64Str.toUtf8());
    if (data.isEmpty()) {
        qWarning() << "[StorageViewModel] Base64 解码失败";
        return false;
    }
    QImage img;
    if (!img.loadFromData(data)) {
        qWarning() << "[StorageViewModel] Base64 数据转图片失败";
        return false;
    }
    return img.save(targetUrl.toLocalFile());
}

bool StorageViewModel::saveLocalImage(const QUrl &sourceUrl, const QUrl &targetUrl)
{
    QImage img;
    if (sourceUrl.isLocalFile())
        img.load(sourceUrl.toLocalFile());
    else if (sourceUrl.scheme() == "qrc")
        img.load(sourceUrl.toString().replace("qrc:/", ":/"));
    else {
        qWarning() << "[StorageViewModel] 不支持的图片来源：" << sourceUrl;
        return false;
    }
    if (img.isNull()) {
        qWarning() << "[StorageViewModel] 图片加载失败：" << sourceUrl;
        return false;
    }
    return img.save(targetUrl.toLocalFile());
}

void StorageViewModel::saveNetworkImage(const QString &networkUrl, const QUrl &targetUrl)
{
    m_pendingTargetUrl = targetUrl;
    QNetworkRequest req;
    req.setUrl(QUrl(networkUrl));
    req.setRawHeader("User-Agent",
                     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    m_netManager.get(req);
}
