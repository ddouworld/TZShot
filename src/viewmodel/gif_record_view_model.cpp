#include "gif_record_view_model.h"
#include "gif_encoder/gif_encoder.h"

#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

GifRecordViewModel::GifRecordViewModel(AppSettings& settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_grabber(this)
{
    connect(&m_grabber, &GifFrameGrabber::frameCaptured,
            this, &GifRecordViewModel::onFrameCaptured);

    m_elapsedTimer = new QTimer(this);
    m_elapsedTimer->setInterval(1000);
    connect(m_elapsedTimer, &QTimer::timeout, this, &GifRecordViewModel::onElapsedTimer);

    m_encodeWatcher = new QFutureWatcher<bool>(this);
    connect(m_encodeWatcher, &QFutureWatcher<bool>::finished,
            this, &GifRecordViewModel::onEncodingFinished);
}

GifRecordViewModel::~GifRecordViewModel()
{
    m_grabber.stop();
    m_elapsedTimer->stop();
}

void GifRecordViewModel::startRecording(int x, int y, int w, int h)
{
    if (m_recording || m_encoding) return;

    // 标准化选区（处理负宽高）
    m_captureRect = QRect(
        qMin(x, x + w), qMin(y, y + h),
        qAbs(w), qAbs(h)
    );

    if (m_captureRect.isEmpty()) {
        emit encodingFailed(tr("录制区域无效"));
        return;
    }

    m_frames.clear();
    m_elapsedSecs = 0;
    m_recording = true;

    emit isRecordingChanged();
    emit frameCountChanged();
    emit elapsedSecsChanged();

    m_grabber.start(m_captureRect, kFps);
    m_elapsedTimer->start();

    qDebug() << "[GifRecord] 开始录制，区域：" << m_captureRect << "，帧率：" << kFps;
}

void GifRecordViewModel::updateCaptureRect(int x, int y, int w, int h)
{
    if (!m_recording) return;

    QRect newRect(
        qMin(x, x + w), qMin(y, y + h),
        qAbs(w), qAbs(h)
    );

    if (newRect.isEmpty()) return;

    m_captureRect = newRect;
    // 通知 GifFrameGrabber 立即使用新区域（下一帧起生效）
    m_grabber.updateRect(newRect);

    qDebug() << "[GifRecord] 更新录制区域：" << m_captureRect;
}

void GifRecordViewModel::stopRecording()
{
    if (!m_recording) return;

    m_grabber.stop();
    m_elapsedTimer->stop();
    m_recording = false;
    emit isRecordingChanged();

    qDebug() << "[GifRecord] 停止录制，已采集" << m_frames.size() << "帧";

    if (m_frames.isEmpty()) {
        emit encodingFailed(tr("未采集到任何帧"));
        return;
    }

    startEncoding();
}

void GifRecordViewModel::cancelRecording()
{
    m_grabber.stop();
    m_elapsedTimer->stop();
    resetState();
    qDebug() << "[GifRecord] 取消录制";
}

void GifRecordViewModel::onFrameCaptured(const QImage& frame)
{
    if (!m_recording) return;

    m_frames.append(frame);
    emit frameCountChanged();

    // 达到最大帧数时自动停止
    if (m_frames.size() >= kMaxFrames) {
        qDebug() << "[GifRecord] 达到最大帧数，自动停止录制";
        stopRecording();
    }
}

void GifRecordViewModel::onElapsedTimer()
{
    ++m_elapsedSecs;
    emit elapsedSecsChanged();
}

void GifRecordViewModel::onEncodingFinished()
{
    bool success = m_encodeWatcher->result();
    m_encoding = false;
    emit isEncodingChanged();

    if (success) {
        qDebug() << "[GifRecord] 编码完成：" << m_pendingSavePath;
        emit encodingFinished(m_pendingSavePath);

        const QString saveDirPath = QFileInfo(m_pendingSavePath).absolutePath();
        if (!saveDirPath.isEmpty()) {
            const bool opened = QDesktopServices::openUrl(QUrl::fromLocalFile(saveDirPath));
            if (!opened) {
                qWarning() << "[GifRecord] Failed to open save directory:" << saveDirPath;
            }
        }
    } else {
        // 错误信息无法从子线程直接传回，通过约定字符串传递
        emit encodingFailed(tr("GIF 编码失败，请检查保存路径或磁盘空间"));
    }

    m_frames.clear();
    m_pendingSavePath.clear();
}

void GifRecordViewModel::startEncoding()
{
    m_pendingSavePath = generateSavePath();
    m_encoding = true;
    emit isEncodingChanged();

    qDebug() << "[GifRecord] 开始编码 →" << m_pendingSavePath;

    // 捕获当前帧序列的副本（子线程持有）
    QVector<QImage> framesToEncode = m_frames;
    QString savePath = m_pendingSavePath;

    QFuture<bool> future = QtConcurrent::run([framesToEncode, savePath]() -> bool {
        GifEncoder encoder;
        bool ok = encoder.encode(framesToEncode, savePath, GifRecordViewModel::kFps, true);
        if (!ok) {
            qDebug() << "[GifRecord] 编码错误：" << encoder.errorString();
        }
        return ok;
    });

    m_encodeWatcher->setFuture(future);
}

QString GifRecordViewModel::generateSavePath() const
{
    QString dirPath = m_settings.savePath();
    if (dirPath.isEmpty()) {
        dirPath = QDir::homePath();
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QDir(dirPath).filePath(QString("gif_%1.gif").arg(timestamp));
}

void GifRecordViewModel::resetState()
{
    m_recording   = false;
    m_encoding    = false;
    m_elapsedSecs = 0;
    m_frames.clear();
    m_captureRect = QRect();

    emit isRecordingChanged();
    emit isEncodingChanged();
    emit frameCountChanged();
    emit elapsedSecsChanged();
}
