#ifndef STICKY_IMAGE_STORE_H
#define STICKY_IMAGE_STORE_H

#include <QObject>
#include <QHash>
#include <QImage>
#include <QMutex>
#include <QUrl>
#include <QtGlobal>

// StickyImageStore
// ----------------
// 纯业务类（QObject），负责贴图的存储、释放、保存、复制到剪贴板等操作。
// 生命周期完全由调用方控制，不依赖 Qt Quick 运行时。

class StickyImageStore : public QObject
{
    Q_OBJECT

public:
    explicit StickyImageStore(QObject *parent = nullptr);

    // 存储图片，返回统一的贴图 URL 字符串
    // 格式："image://sticky/<uuid>"
    QString storeImage(const QImage &image);

    // 线程安全读取
    QImage getImage(const QString &id) const;
    QImage getImageByUrl(const QString &imageUrl) const;

    Q_INVOKABLE void releaseImage(const QString &imageUrl);
    Q_INVOKABLE bool saveImage(const QString &imageUrl, const QUrl &targetUrl);
    Q_INVOKABLE bool replaceImage(const QString &imageUrl, const QImage &image);
    Q_INVOKABLE bool copyImageToClipboard(const QString &imageUrl);
    Q_INVOKABLE qint64 processRssKb() const;
    Q_INVOKABLE int imageCount() const;
    Q_INVOKABLE void logMemoryUsage(const QString &tag) const;

private:
    QString extractId(const QString &imageUrl) const;

    QHash<QString, QImage> m_images;
    mutable QMutex m_mutex;
};

#endif // STICKY_IMAGE_STORE_H
