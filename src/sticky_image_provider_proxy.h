#ifndef STICKY_IMAGE_PROVIDER_PROXY_H
#define STICKY_IMAGE_PROVIDER_PROXY_H

#include <QQuickImageProvider>
#include "sticky_image_store.h"

// StickyImageProviderProxy
// ------------------------
// 纯粹的 QQuickImageProvider 实现，不继承 QObject。
// 生命周期由 QQmlApplicationEngine 通过 addImageProvider() 接管并负责释放。
// 所有实际数据操作委托给 StickyImageStore，自身不持有任何图片数据。

class StickyImageProviderProxy : public QQuickImageProvider
{
public:
    explicit StickyImageProviderProxy(StickyImageStore &store);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    StickyImageStore &m_store;  // 引用，不持有所有权
};

#endif // STICKY_IMAGE_PROVIDER_PROXY_H
