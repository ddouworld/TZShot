#ifndef GIF_ENCODER_H
#define GIF_ENCODER_H

#include <QVector>
#include <QImage>
#include <QString>

// GifEncoder
// ----------
// 将 QImage 帧序列编码为 GIF 文件。
// - 支持透明通道（alpha < 128 的像素映射到 GIF 透明索引）
// - 编码使用 gif.h 单头文件库（6x6x6 色彩量化 + LZW 压缩）
// - encode() 是同步操作，建议在子线程（QtConcurrent）调用

class GifEncoder
{
public:
    GifEncoder() = default;

    // fps: 帧率（如 15）
    // enableTransparency: 是否将半透明像素处理为透明
    // 返回 true 表示成功，errorString() 返回错误信息
    bool encode(const QVector<QImage>& frames,
                const QString& filePath,
                int fps,
                bool enableTransparency = true);

    QString errorString() const { return m_errorString; }

private:
    // 将 QImage 转为 gif.h 需要的 RGBA8888 原始字节数组
    // 如果 enableTransparency=true，alpha<128 的像素 alpha 强制为 0
    QVector<uint8_t> toRGBA(const QImage& img, bool enableTransparency) const;

    QString m_errorString;
};

#endif // GIF_ENCODER_H
