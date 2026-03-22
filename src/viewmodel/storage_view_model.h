#ifndef STORAGE_VIEW_MODEL_H
#define STORAGE_VIEW_MODEL_H

#include <QObject>
#include <QUrl>
#include <QNetworkAccessManager>
#include "model/app_settings.h"

// StorageViewModel
// ----------------
// 负责截图文件保存的业务编排，以及 savePath 的 Q_PROPERTY 暴露。
// 数据（savePath）由 AppSettings Model 持久化，
// 文件 I/O 逻辑（Base64/URL/网络图片）保留在本类。

class StorageViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl savePath READ savePath NOTIFY savePathChanged)

public:
    explicit StorageViewModel(AppSettings &settings, QObject *parent = nullptr);

    QUrl savePath() const;
    Q_INVOKABLE void setSavePath(const QUrl &path);

    // 统一保存入口：自动识别 Base64 / 网络 URL / 本地路径
    Q_INVOKABLE bool saveImage(const QString &sourceUrl, const QUrl &targetUrl);

signals:
    void savePathChanged(const QUrl &savePath);

private:
    bool saveBase64Image(const QString &base64Str, const QUrl &targetUrl);
    bool saveLocalImage(const QUrl &sourceUrl,  const QUrl &targetUrl);
    void saveNetworkImage(const QString &networkUrl, const QUrl &targetUrl);

    AppSettings          &m_settings;
    QNetworkAccessManager m_netManager;
    QUrl                  m_pendingTargetUrl;
};

#endif // STORAGE_VIEW_MODEL_H
