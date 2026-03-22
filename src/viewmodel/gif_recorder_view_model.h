#ifndef GIFRECORDER_H
#define GIFRECORDER_H

#include <QObject>
#include <QTimer>
#include <QImage>
#include <QRect>
#include <QFile>
#include <QStandardPaths>
#include <vector>
#include <cstdint>

// 正确声明 stbi_write_gif（extern "C" 避免 C++ 名称修饰）
extern "C" {
int stbi_write_gif(const char *filename, int w, int h, int comp,
                   const void **frames, const int *delays, int loop);
}
class GifRecorder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
    Q_PROPERTY(QRect recordRect READ recordRect WRITE setRecordRect NOTIFY recordRectChanged)
    Q_PROPERTY(int fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(QString outputPath READ outputPath WRITE setOutputPath NOTIFY outputPathChanged)

public:
    explicit GifRecorder(QObject *parent = nullptr);
    ~GifRecorder();

    Q_INVOKABLE void startRecord();
    Q_INVOKABLE void stopRecord();

    bool isRecording() const { return m_isRecording; }
    QRect recordRect() const { return m_recordRect; }
    void setRecordRect(const QRect& rect);
    int fps() const { return m_fps; }
    void setFps(int fps);
    QString outputPath() const { return m_outputPath; }
    void setOutputPath(const QString& path);

signals:
    void isRecordingChanged();
    void recordRectChanged();
    void fpsChanged();
    void outputPathChanged();
    void recordFinished(const QString& filePath);

private slots:
    void captureFrame();

private:
    QImage captureScreen(const QRect& rect);

    // 录制状态
    bool m_isRecording = false;
    QRect m_recordRect;
    int m_fps = 10;
    QString m_outputPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/record.gif";
    QTimer* m_captureTimer = nullptr;

    // stb_image_write 所需的帧数据缓存
    std::vector<uint8_t*> m_frameBuffers;  // 存储每帧的 RGB 数据
    std::vector<int> m_frameDelays;        // 存储每帧的延迟（毫秒）
};

#endif // GIFRECORDER_H
