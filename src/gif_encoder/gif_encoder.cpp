#include "gif_encoder.h"
#include "gif.h"

#include <QFileInfo>
#include <QDir>

bool GifEncoder::encode(const QVector<QImage>& frames,
                        const QString& filePath,
                        int fps,
                        bool enableTransparency)
{
    if (frames.isEmpty()) {
        m_errorString = "No frames to encode";
        return false;
    }

    // GIF delay 单位是 1/100 秒
    // fps=15 -> delay = 100/15 ≈ 7 (70ms)
    uint32_t delay = (fps > 0) ? (uint32_t)(100 / fps) : 7;
    if (delay < 1) delay = 1;

    // 确保目录存在
    QDir dir = QFileInfo(filePath).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 以第一帧的尺寸作为 GIF 尺寸
    const QImage& first = frames.first();
    uint32_t width  = (uint32_t)first.width();
    uint32_t height = (uint32_t)first.height();

    if (width == 0 || height == 0) {
        m_errorString = "Invalid frame size";
        return false;
    }

    GifWriter writer;
    memset(&writer, 0, sizeof(writer));

    if (!GifBegin(&writer, filePath.toLocal8Bit().constData(),
                  width, height, delay, 8, false)) {
        m_errorString = QString("Cannot open file: %1").arg(filePath);
        return false;
    }

    for (const QImage& frame : frames) {
        // 确保图像尺寸一致（裁剪或缩放到第一帧尺寸）
        QImage scaled = frame;
        if ((uint32_t)scaled.width() != width || (uint32_t)scaled.height() != height) {
            scaled = scaled.scaled((int)width, (int)height, Qt::IgnoreAspectRatio,
                                   Qt::SmoothTransformation);
        }

        QVector<uint8_t> rgba = toRGBA(scaled, enableTransparency);
        GifWriteFrame(&writer, rgba.constData(), width, height, delay, 8, false);
    }

    GifEnd(&writer);

    m_errorString.clear();
    return true;
}

QVector<uint8_t> GifEncoder::toRGBA(const QImage& img, bool enableTransparency) const
{
    // 统一转换为 RGBA8888 格式
    QImage rgba = img.convertToFormat(QImage::Format_RGBA8888);

    int w = rgba.width();
    int h = rgba.height();
    QVector<uint8_t> data(w * h * 4);

    for (int y = 0; y < h; ++y) {
        const uint8_t* src = rgba.constScanLine(y);
        uint8_t* dst = data.data() + y * w * 4;
        if (enableTransparency) {
            for (int x = 0; x < w; ++x) {
                uint8_t a = src[x * 4 + 3];
                if (a < 128) {
                    // 透明像素：alpha 强制为 0，RGB 任意（gif.h 会映射到透明索引）
                    dst[x * 4 + 0] = 0;
                    dst[x * 4 + 1] = 0;
                    dst[x * 4 + 2] = 0;
                    dst[x * 4 + 3] = 0;
                } else {
                    dst[x * 4 + 0] = src[x * 4 + 0];  // R
                    dst[x * 4 + 1] = src[x * 4 + 1];  // G
                    dst[x * 4 + 2] = src[x * 4 + 2];  // B
                    dst[x * 4 + 3] = 255;
                }
            }
        } else {
            memcpy(dst, src + 0, (size_t)(w * 4));
        }
    }

    return data;
}
