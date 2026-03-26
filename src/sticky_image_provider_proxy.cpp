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
        const qreal dpr = image.devicePixelRatio();
        if (!image.isNull() && dpr > 0.0) {
            *size = QSize(qMax(1, qRound(image.width() / dpr)),
                          qMax(1, qRound(image.height() / dpr)));
        } else {
            *size = image.size();
        }
    }
    return image;
}
