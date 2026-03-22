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
// 不继承 QQuickImageProvider，生命周期完全由调用方控制，不受 engine 影响。
// 图片数据通过 storeImage() 写入，由 StickyImageProviderProxy 的 requestImage() 读取。

class StickyImageStore : public QObject
{
    Q_OBJECT

public:
    explicit StickyImageStore(QObject *parent = nullptr);

    // 存储图片，返回可供 QML Image.source 使用的 URL 字符串
    // 格式："image://sticky/<uuid>"
    QString storeImage(const QImage &image);

    // 供 StickyImageProviderProxy 调用，线程安全
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
