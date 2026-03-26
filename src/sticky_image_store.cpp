#include "sticky_image_store.h"

#include <QFile>
#include <QGuiApplication>
#include <QMutexLocker>
#include <QStringList>
#include <QTextStream>
#include <QUuid>
#include <QClipboard>
#ifdef Q_OS_LINUX
#include <malloc.h>
#include <unistd.h>
#endif
#include <QDebug>

StickyImageStore::StickyImageStore(QObject *parent)
    : QObject(parent)
{
}

QString StickyImageStore::storeImage(const QImage &image)
{
    if (image.isNull()) {
        return QString();
    }

#ifdef STICKY_MEM_DEBUG
    logMemoryUsage(QStringLiteral("store-before"));
#endif

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        QMutexLocker locker(&m_mutex);
        m_images.insert(id, image);
    }

#ifdef STICKY_MEM_DEBUG
    logMemoryUsage(QStringLiteral("store-after"));
#endif

    return QStringLiteral("image://sticky/") + id;
}

QImage StickyImageStore::getImage(const QString &id) const
{
    QMutexLocker locker(&m_mutex);
    return m_images.value(id);
}

QImage StickyImageStore::getImageByUrl(const QString &imageUrl) const
{
    const QString id = extractId(imageUrl);
    if (id.isEmpty()) {
        return {};
    }
    return getImage(id);
}

void StickyImageStore::releaseImage(const QString &imageUrl)
{
#ifdef STICKY_MEM_DEBUG
    logMemoryUsage(QStringLiteral("release-before"));
#endif

    const QString id = extractId(imageUrl);
    if (id.isEmpty()) {
        qWarning() << "[StickyMemory] release-skip invalid url:" << imageUrl;
        return;
    }

    {
        QMutexLocker locker(&m_mutex);
        m_images.remove(id);
    }

#if defined(STICKY_MEM_DEBUG) && defined(Q_OS_LINUX)
    malloc_trim(0);
#endif

#ifdef STICKY_MEM_DEBUG
    logMemoryUsage(QStringLiteral("release-after"));
#endif
}

bool StickyImageStore::saveImage(const QString &imageUrl, const QUrl &targetUrl)
{
    if (targetUrl.isEmpty() || !targetUrl.isLocalFile()) {
        return false;
    }

    const QString id = extractId(imageUrl);
    if (id.isEmpty()) {
        return false;
    }

    QImage image;
    {
        QMutexLocker locker(&m_mutex);
        image = m_images.value(id);
    }

    if (image.isNull()) {
        return false;
    }

    return image.save(targetUrl.toLocalFile());
}

bool StickyImageStore::replaceImage(const QString &imageUrl, const QImage &image)
{
    if (image.isNull()) {
        return false;
    }

    const QString id = extractId(imageUrl);
    if (id.isEmpty()) {
        return false;
    }

    QMutexLocker locker(&m_mutex);
    if (!m_images.contains(id)) {
        return false;
    }
    m_images[id] = image;
    return true;
}

bool StickyImageStore::copyImageToClipboard(const QString &imageUrl)
{
    const QString id = extractId(imageUrl);
    if (id.isEmpty()) {
        return false;
    }

    QImage image;
    {
        QMutexLocker locker(&m_mutex);
        image = m_images.value(id);
    }

    if (image.isNull()) {
        return false;
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard) {
        return false;
    }

    clipboard->setImage(image, QClipboard::Clipboard);
#ifdef Q_OS_LINUX
    clipboard->setImage(image, QClipboard::Selection);
#endif
    return true;
}

QString StickyImageStore::extractId(const QString &imageUrl) const
{
    static const QString prefix = QStringLiteral("image://sticky/");
    if (!imageUrl.startsWith(prefix)) {
        return QString();
    }

    QString id = imageUrl.mid(prefix.size());
    const int queryIndex = id.indexOf('?');
    if (queryIndex >= 0) {
        id = id.left(queryIndex);
    }
    return id;
}

qint64 StickyImageStore::processRssKb() const
{
#ifdef Q_OS_LINUX
    QFile statusFile(QStringLiteral("/proc/self/status"));
    if (!statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[StickyMemory] open /proc/self/status failed:" << statusFile.errorString();
    } else {
        QTextStream in(&statusFile);
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (!line.contains(QStringLiteral("VmRSS:"))) {
                continue;
            }

            const QStringList parts = line.simplified().split(' ');
            if (parts.size() >= 2) {
                bool ok = false;
                const qint64 value = parts.at(1).toLongLong(&ok);
                if (ok) {
                    return value;
                }
            }
            qWarning() << "[StickyMemory] parse VmRSS failed, line:" << line;
            break;
        }
    }

    QFile statmFile(QStringLiteral("/proc/self/statm"));
    if (statmFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QByteArray raw = statmFile.readAll().trimmed();
        const QList<QByteArray> fields = raw.split(' ');
        if (fields.size() >= 2) {
            bool ok = false;
            const qint64 residentPages = fields.at(1).toLongLong(&ok);
            if (ok) {
                const long pageSize = sysconf(_SC_PAGESIZE);
                if (pageSize > 0) {
                    return (residentPages * pageSize) / 1024;
                }
            }
        }
        qWarning() << "[StickyMemory] parse /proc/self/statm failed, raw:" << raw;
    } else {
        qWarning() << "[StickyMemory] open /proc/self/statm failed:" << statmFile.errorString();
    }

    return -1;
#else
    return -1;
#endif
}

int StickyImageStore::imageCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_images.size();
}

void StickyImageStore::logMemoryUsage(const QString &tag) const
{
    qWarning().nospace() << "[StickyMemory] " << tag
                         << " rss_kb=" << processRssKb()
                         << " image_count=" << imageCount();
}
