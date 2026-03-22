#ifndef STICKY_VIEW_MODEL_H
#define STICKY_VIEW_MODEL_H

#include <QObject>
#include <QRect>
#include <QImage>
#include "sticky_image_store.h"

// StickyViewModel
// ---------------
// 负责贴图会话的业务逻辑：
//   - 从 StickyImageStore 读写贴图数据（save/copy/release）
//   - 通过信号通知 QML 创建贴图窗口（View 监听信号后自行创建 TZStickyWindow）
// QML 侧只需监听 stickyReady 信号，无需知道截图细节。

class StickyViewModel : public QObject
{
    Q_OBJECT

public:
    explicit StickyViewModel(StickyImageStore &store, QObject *parent = nullptr);

    // 通知 QML 创建贴图窗口，由 ScreenshotViewModel::captureRectToStickyUrl 产生的 URL 触发
    // QML 监听 stickyReady(imageUrl, imgRect) 信号后创建 TZStickyWindow
    Q_INVOKABLE void requestSticky(const QString &imageUrl, const QRect &imgRect);

    // 以下三个方法供 TZStickyWindow.qml 直接调用（贴图窗口内部操作）
    Q_INVOKABLE void    releaseImage(const QString &imageUrl);
    Q_INVOKABLE bool    saveImage(const QString &imageUrl, const QUrl &targetUrl);
    Q_INVOKABLE bool    copyImageToClipboard(const QString &imageUrl);
    Q_INVOKABLE QImage  getImageByUrl(const QString &imageUrl) const;
    Q_INVOKABLE bool    overwriteWithAnnotations(const QString &imageUrl, const QImage &annotationLayer);
    Q_INVOKABLE bool    saveMergedImage(const QString &imageUrl, const QImage &annotationLayer, const QUrl &targetUrl);
    Q_INVOKABLE bool    rotateImage(const QString &imageUrl, int degreesClockwise);
    Q_INVOKABLE bool    mirrorImage(const QString &imageUrl, bool horizontal, bool vertical);

signals:
    // QML 监听此信号，收到后创建并显示 TZStickyWindow
    void stickyReady(const QString &imageUrl, const QRect &imgRect);

private:
    QImage mergeLayers(const QString &imageUrl, const QImage &annotationLayer) const;

    StickyImageStore &m_store;
};

#endif // STICKY_VIEW_MODEL_H
