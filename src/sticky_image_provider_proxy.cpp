#include "sticky_image_provider_proxy.h"

StickyImageProviderProxy::StickyImageProviderProxy(StickyImageStore &store)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_store(store)
{
}

QImage StickyImageProviderProxy::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize)

    QString cleanId = id;
    const int queryIndex = cleanId.indexOf('?');
    if (queryIndex >= 0) {
        cleanId = cleanId.left(queryIndex);
    }

    QImage image = m_store.getImage(cleanId);

    if (size) {
        *size = image.size();
    }
    return image;
}
