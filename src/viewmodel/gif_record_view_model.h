#ifndef GIF_RECORD_VIEW_MODEL_H
#define GIF_RECORD_VIEW_MODEL_H

#include <QObject>
#include <QVector>
#include <QImage>
#include <QRect>
#include <QFutureWatcher>

#include "gif_frame_grabber.h"
#include "model/app_settings.h"

// GifRecordViewModel
// ------------------
// GIF 录制的核心状态机，暴露给 QML。
//
// 状态：
//   Idle → (startRecording) → Recording → (stopRecording) → Encoding → Idle
//                                                          ↘ (cancelRecording) → Idle
//
// QML 属性：
//   isRecording  - 是否正在录制
//   isEncoding   - 是否正在编码（子线程）
//   frameCount   - 已录帧数
//   elapsedSecs  - 已录时长（秒）
//
// QML 信号：
//   encodingFinished(savedPath)  - 编码完成，返回保存路径
//   encodingFailed(error)        - 编码失败，返回错误信息

class GifRecordViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
    Q_PROPERTY(bool isEncoding  READ isEncoding  NOTIFY isEncodingChanged)
    Q_PROPERTY(int  frameCount  READ frameCount  NOTIFY frameCountChanged)
    Q_PROPERTY(int  elapsedSecs READ elapsedSecs NOTIFY elapsedSecsChanged)
    Q_PROPERTY(int  qualityPreset READ qualityPreset WRITE setQualityPreset NOTIFY qualityPresetChanged)

public:
    explicit GifRecordViewModel(AppSettings& settings, QObject* parent = nullptr);
    ~GifRecordViewModel() override;

    bool isRecording() const { return m_recording; }
    bool isEncoding()  const { return m_encoding; }
    int  frameCount()  const { return m_frames.size(); }
    int  elapsedSecs() const { return m_elapsedSecs; }
    int  qualityPreset() const { return m_settings.gifQualityPreset(); }

    // QML 调用：开始录制，captureRect 为屏幕逻辑坐标选区
    Q_INVOKABLE void startRecording(int x, int y, int w, int h);

    // QML 调用：录制中实时更新抓取区域（拖动选区时调用）
    Q_INVOKABLE void updateCaptureRect(int x, int y, int w, int h);

    // QML 调用：停止录制并触发编码
    Q_INVOKABLE void stopRecording();

    // QML 调用：取消录制，丢弃所有帧
    Q_INVOKABLE void cancelRecording();
    Q_INVOKABLE void setQualityPreset(int preset);

    // 最大录制帧数（防止内存过大），默认 15fps × 30s = 450 帧
    static constexpr int kMaxFrames = 450;

signals:
    void isRecordingChanged();
    void isEncodingChanged();
    void frameCountChanged();
    void elapsedSecsChanged();
    void qualityPresetChanged();

    void encodingFinished(const QString& savedPath);
    void encodingFailed(const QString& error);

private slots:
    void onFrameCaptured(const QImage& frame);
    void onElapsedTimer();
    void onEncodingFinished();

private:
    QString generateSavePath() const;
    void    startEncoding();
    void    resetState();

    AppSettings&       m_settings;
    GifFrameGrabber    m_grabber;
    QVector<QImage>    m_frames;
    QRect              m_captureRect;

    bool               m_recording  = false;
    bool               m_encoding   = false;
    int                m_elapsedSecs = 0;
    QTimer*            m_elapsedTimer = nullptr;

    // 异步编码
    QFutureWatcher<bool>* m_encodeWatcher = nullptr;
    QString               m_pendingSavePath;

    static constexpr int kFps = 15;
};

#endif // GIF_RECORD_VIEW_MODEL_H
