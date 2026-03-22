#ifndef MAGNIFIER_PROVIDER_H
#define MAGNIFIER_PROVIDER_H

#include <QQuickImageProvider>
#include "viewmodel/screenshot_view_model.h"

// MagnifierProvider
// -----------------
// QQuickImageProvider 子类，供 QML Image 通过
//   source: "image://magnifier/<x>,<y>"
// 获取以 (x,y) 为中心的放大镜图像。
//
// 图像规格：
//   输出尺寸：110×110（QML 侧再用 width/height 撑到 130×130 预留边距）
//   采样区域：以 (x,y) 为中心、半径 40 像素（共 80×80）从桌面快照裁剪
//   放大倍率：~1.375x（110/80）
//   十字准线：红色，绘制在 requestImage() 内部
//
// 线程安全：requestImage() 可能在非主线程调用，只对 QImage 做只读 copy()。

class MagnifierProvider : public QQuickImageProvider
{
public:
    explicit MagnifierProvider(ScreenshotViewModel *viewModel);

    // QQuickImageProvider interface
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    ScreenshotViewModel *m_viewModel;

    static const int kSrcRadius  = 20;   // 采样半径（源区域 40×40）
    static const int kOutputSize = 110;  // 输出图像尺寸
};

#endif // MAGNIFIER_PROVIDER_H
