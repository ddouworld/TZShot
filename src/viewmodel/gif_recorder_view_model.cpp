// 关键：仅在这一个文件中定义 STB_IMAGE_WRITE_IMPLEMENTATION，展开实现
#define STB_IMAGE_WRITE_IMPLEMENTATION  // 关键：先定义宏，再包含头文件
#include "stb_image_write.h"

#include "gif_recorder_view_model.h"
#include <QApplication>
#include <QScreen>
#include <QDir>
#include <QDebug>
#include <windows.h>


GifRecorder::GifRecorder(QObject *parent) : QObject(parent)
{
    m_captureTimer = new QTimer(this);
    m_captureTimer->setTimerType(Qt::PreciseTimer);
    connect(m_captureTimer, &QTimer::timeout, this, &GifRecorder::captureFrame);

    // 默认录制全屏
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        m_recordRect = screen->geometry();
    } else {
        m_recordRect = QRect(0, 0, 1920, 1080); // 兜底分辨率
    }

    // 确保输出目录存在
    QString outputDir = QFileInfo(m_outputPath).dir().absolutePath();
    if (!QDir(outputDir).exists()) {
        QDir().mkpath(outputDir);
    }
}

GifRecorder::~GifRecorder()
{
    if (m_isRecording) {
        stopRecord();
    }
    // 释放帧缓存
    for (uint8_t* buf : m_frameBuffers) {
        delete[] buf;
    }
    m_frameBuffers.clear();
    m_frameDelays.clear();
}

void GifRecorder::startRecord()
{
    if (m_isRecording) return;

    // 清空历史帧数据
    for (uint8_t* buf : m_frameBuffers) {
        delete[] buf;
    }
    m_frameBuffers.clear();
    m_frameDelays.clear();

    // 启动定时器（按帧率捕获帧）
    m_isRecording = true;
    emit isRecordingChanged();
    m_captureTimer->start(1000 / m_fps);

    qDebug() << "GIF 录制开始：" << m_outputPath;
}

void GifRecorder::stopRecord()
{
    if (!m_isRecording) return;

    // 停止定时器
    m_captureTimer->stop();
    m_isRecording = false;
    emit isRecordingChanged();

    // 生成 GIF 文件
    if (!m_frameBuffers.empty()) {
        // 转换帧缓存为 stbi_write_gif 所需的 const void** 格式
        const void** frames = new const void*[m_frameBuffers.size()];
        for (size_t i = 0; i < m_frameBuffers.size(); i++) {
            frames[i] = m_frameBuffers[i];
        }

        // 调用 stb 生成 GIF（0=无限循环）
        int result = stbi_write_gif(
            m_outputPath.toUtf8().data(),
            m_recordRect.width(),
            m_recordRect.height(),
            3,  // 通道数：RGB（3通道）
            frames,
            m_frameDelays.data(),
            0   // 循环次数：0=无限循环
            );

        delete[] frames;

        if (result != 0) {
            qDebug() << "GIF 生成成功：" << m_outputPath;
            emit recordFinished(m_outputPath);
        } else {
            qCritical() << "GIF 生成失败：" << m_outputPath;
        }
    } else {
        qWarning() << "无录制帧数据，跳过 GIF 生成";
    }
}

void GifRecorder::captureFrame()
{
    if (!m_isRecording) return;

    // 捕获屏幕帧并转换为 RGB 格式
    QImage frame = captureScreen(m_recordRect);
    if (frame.isNull()) return;

    // 转换 QImage 为 RGB 数组（stb 要求 RGB 顺序，无 Alpha）
    QImage rgbFrame = frame.convertToFormat(QImage::Format_RGB888);
    int frameSize = rgbFrame.width() * rgbFrame.height() * 3;
    uint8_t* frameBuf = new uint8_t[frameSize];
    memcpy(frameBuf, rgbFrame.bits(), frameSize);

    // 缓存帧数据和延迟（延迟 = 1000/帧率 毫秒）
    m_frameBuffers.push_back(frameBuf);
    m_frameDelays.push_back(1000 / m_fps);
}

QImage GifRecorder::captureScreen(const QRect& rect)
{
    // Windows 原生高效截图（跨平台可替换为 Qt 接口）
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, rect.width(), rect.height());
    SelectObject(hMemoryDC, hBitmap);

    BitBlt(hMemoryDC, 0, 0, rect.width(), rect.height(),
           hScreenDC, rect.x(), rect.y(), SRCCOPY);

    // 转换为 QImage
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rect.width();
    bmi.bmiHeader.biHeight = -rect.height(); // 负高度：从上到下
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    QImage image(rect.width(), rect.height(), QImage::Format_RGB888);
    GetDIBits(hMemoryDC, hBitmap, 0, rect.height(),
              image.bits(), &bmi, DIB_RGB_COLORS);

    // 释放资源
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    return image;
}

void GifRecorder::setRecordRect(const QRect& rect)
{
    if (m_recordRect != rect) {
        m_recordRect = rect;
        emit recordRectChanged();
    }
}

void GifRecorder::setFps(int fps)
{
    if (fps < 1 || fps > 30) return;
    if (m_fps != fps) {
        m_fps = fps;
        emit fpsChanged();
        if (m_isRecording) {
            m_captureTimer->setInterval(1000 / m_fps);
        }
    }
}

void GifRecorder::setOutputPath(const QString& path)
{
    if (m_outputPath != path) {
        m_outputPath = path;
        emit outputPathChanged();
        // 确保新路径目录存在
        QString outputDir = QFileInfo(m_outputPath).dir().absolutePath();
        if (!QDir(outputDir).exists()) {
            QDir().mkpath(outputDir);
        }
    }
}
