#include "scroll_capture_view_model.h"

#include <QDateTime>
#include <QBuffer>
#include <QDir>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

namespace {
constexpr int kMaxManualFrames = 200;
constexpr int kAutoSampleMs = 90;
constexpr int kProbeWidth = 72;
constexpr int kProbeHeight = 72;
constexpr int kAvgDiffThreshold = 3;
}

ScrollCaptureViewModel::ScrollCaptureViewModel(ScreenshotViewModel &screenCapture,
                                               AppSettings &settings,
                                               QObject *parent)
    : QObject(parent)
    , m_screenCapture(screenCapture)
    , m_settings(settings)
{
    m_timer.setInterval(kAutoSampleMs);
    connect(&m_timer, &QTimer::timeout, this, [this]() { onTick(); });
}

void ScrollCaptureViewModel::start(const QRect &captureRect)
{
    if (m_isCapturing) {
        return;
    }

    const QRect rect = captureRect.normalized();
    if (rect.width() < 20 || rect.height() < 20) {
        emit captureFailed(tr("长截图区域无效"));
        return;
    }

    m_captureRect = rect;
    m_stitcher.reset();
    m_capturedFrames = 0;
    emit capturedFramesChanged();

    const QImage first = m_screenCapture.captureRectToImageLive(m_captureRect);
    if (first.isNull()) {
        emit captureFailed(tr("首帧采样失败"));
        return;
    }

    m_stitcher.setBaseFrame(first);
    m_capturedFrames = 1;
    emit capturedFramesChanged();
    setPreviewImage(m_stitcher.result());
    m_lastProbe = makeProbe(first);

    m_isCapturing = true;
    emit isCapturingChanged();
    emit captureStarted();
    setStatus(tr("滚动检测已开启，滑动内容会自动拼接"));
    m_timer.start();
}

void ScrollCaptureViewModel::stop()
{
    if (!m_isCapturing) {
        return;
    }

    const QImage current = m_stitcher.result();
    if (!current.isNull()) {
        finishSuccess(current);
    } else {
        finishWithError(tr("长截图已停止"));
    }
}

void ScrollCaptureViewModel::onTick()
{
    if (!m_isCapturing) {
        return;
    }

    const QImage frame = m_screenCapture.captureRectToImageLive(m_captureRect);
    if (frame.isNull()) {
        finishWithError(tr("采样失败，已中断"));
        return;
    }

    if (!hasMeaningfulChange(frame)) {
        setStatus(tr("等待滚动变化..."));
        return;
    }

    ++m_capturedFrames;
    emit capturedFramesChanged();
    const int added = m_stitcher.appendFrame(frame);
    setPreviewImage(m_stitcher.result());
    if (added > 0) {
        setStatus(tr("滚动已捕捉，已采样 %1 帧").arg(m_capturedFrames));
    } else {
        setStatus(tr("检测到变化但拼接增量为 0，请继续滚动"));
    }

    if (m_capturedFrames >= kMaxManualFrames) {
        finishWithError(tr("采样帧数超过上限，请先结束并保存"));
    }
}

void ScrollCaptureViewModel::setStatus(const QString &text)
{
    if (m_statusText == text) {
        return;
    }
    m_statusText = text;
    emit statusTextChanged();
}

QImage ScrollCaptureViewModel::makeProbe(const QImage &image) const
{
    if (image.isNull()) {
        return QImage();
    }
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    return gray.scaled(kProbeWidth, kProbeHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

bool ScrollCaptureViewModel::hasMeaningfulChange(const QImage &image)
{
    const QImage currentProbe = makeProbe(image);
    if (currentProbe.isNull()) {
        return false;
    }
    if (m_lastProbe.isNull() || m_lastProbe.size() != currentProbe.size()) {
        m_lastProbe = currentProbe;
        return true;
    }

    qint64 sum = 0;
    const int w = currentProbe.width();
    const int h = currentProbe.height();
    for (int y = 0; y < h; ++y) {
        const uchar *a = currentProbe.constScanLine(y);
        const uchar *b = m_lastProbe.constScanLine(y);
        for (int x = 0; x < w; ++x) {
            sum += qAbs(int(a[x]) - int(b[x]));
        }
    }

    const int avg = int(sum / qMax(1, w * h));
    const bool changed = avg >= kAvgDiffThreshold;
    if (changed) {
        m_lastProbe = currentProbe;
    }
    return changed;
}

void ScrollCaptureViewModel::setPreviewImage(const QImage &image)
{
    QString next;
    if (!image.isNull()) {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        next = QStringLiteral("data:image/png;base64,%1")
                   .arg(QString::fromLatin1(bytes.toBase64()));
    }

    if (m_previewImageUrl == next) {
        return;
    }
    m_previewImageUrl = next;
    emit previewImageUrlChanged();
}

void ScrollCaptureViewModel::finishWithError(const QString &text)
{
    m_timer.stop();
    m_isCapturing = false;
    emit isCapturingChanged();
    setStatus(text);
    emit captureFailed(text);
}

void ScrollCaptureViewModel::finishSuccess(const QImage &result)
{
    m_timer.stop();
    m_isCapturing = false;
    emit isCapturingChanged();

    if (result.isNull()) {
        emit captureFailed(tr("拼接失败，结果为空"));
        return;
    }
    if (result.height() <= m_captureRect.height() + 20) {
        emit captureFailed(tr("未检测到有效滚动，请先激活目标窗口后重试"));
        return;
    }

    m_screenCapture.copyImageToClipboard(result);

    QString dirPath = m_settings.savePath();
    if (dirPath.isEmpty()) {
        dirPath = QDir::homePath();
    }
    QDir().mkpath(dirPath);
    const QString fileName = QStringLiteral("long_%1.png")
                                 .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString savePath = QDir(dirPath).filePath(fileName);
    result.save(savePath, "PNG");

    const QString saveDirPath = QFileInfo(savePath).absolutePath();
    if (!saveDirPath.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(saveDirPath));
    }

    setStatus(tr("长截图完成"));
    emit captureSucceeded(QFileInfo(savePath).absoluteFilePath());
}
