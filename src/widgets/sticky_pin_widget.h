#ifndef STICKY_PIN_WIDGET_H
#define STICKY_PIN_WIDGET_H

#include <QImage>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QVector>
#include <QWidget>

#include "paint_board/shape/shape_type.h"

class StickyImageStore;
class AIViewModel;
class QCheckBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QToolButton;
class StickyCanvasWidget;

class StickyPinWidget : public QWidget
{
    Q_OBJECT

public:
    StickyPinWidget(const QString &imageUrl,
                    const QRect &physicalRect,
                    const QImage &image,
                    StickyImageStore *store,
                    AIViewModel *aiViewModel = nullptr,
                    QWidget *parent = nullptr);
    ~StickyPinWidget() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void setActiveTool(Shapeype type, QToolButton *activeButton);
    void syncCanvasToolSettings();
    void updateToolOptionsPanel();
    void updateLayoutAndSize();
    QToolButton *currentActiveToolButton() const;
    void setZoomFactor(qreal value);
    void openAiEditor();
    void applyAiImage(const QString &oldImageUrl, const QString &newImageUrl);
    void releaseStoredImage();
    bool saveCurrentImage();
    qreal screenScaleForRect(const QRect &physicalRect) const;
    void applyNativePosition(const QRect &physicalRect);
    QPoint currentContentTopLeftPhysical() const;
    QRect contentRect() const;
    QRect toolOptionsRect() const;
    QRect toolbarRect() const;
    int shadowPadding() const;
    int toolbarGap() const;
    int toolOptionsGap() const;
    int toolOptionsHeight() const;
    int toolOptionsWidth() const;
    int toolbarHeight() const;
    int toolbarWidth() const;

    QString m_imageUrl;
    QRect m_physicalRect;
    QImage m_image;
    QSize m_baseImageDisplaySize;
    QSize m_imageDisplaySize;
    QPointer<StickyImageStore> m_store;
    QPointer<AIViewModel> m_aiViewModel;
    StickyCanvasWidget *m_canvas = nullptr;
    QWidget *m_toolOptions = nullptr;
    QWidget *m_toolbar = nullptr;
    QLabel *m_zoomLabel = nullptr;
    QVector<QToolButton*> m_colorButtons;
    QVector<QToolButton*> m_sizeButtons;
    QLineEdit *m_inlineTextEditor = nullptr;
    QLineEdit *m_textInput = nullptr;
    QCheckBox *m_textBackgroundCheck = nullptr;
    QCheckBox *m_numberAutoIncrementCheck = nullptr;
    QToolButton *m_numberMinusButton = nullptr;
    QLabel *m_numberLabel = nullptr;
    QToolButton *m_numberPlusButton = nullptr;
    QSpinBox *m_numberValueSpin = nullptr;
    QToolButton *m_pencilButton = nullptr;
    QToolButton *m_rectButton = nullptr;
    QToolButton *m_circleButton = nullptr;
    QToolButton *m_arrowButton = nullptr;
    QToolButton *m_highlightButton = nullptr;
    QToolButton *m_mosaicButton = nullptr;
    QToolButton *m_blurButton = nullptr;
    QToolButton *m_textButton = nullptr;
    QToolButton *m_numberButton = nullptr;
    QToolButton *m_undoButton = nullptr;
    QToolButton *m_aiButton = nullptr;
    QToolButton *m_copyButton = nullptr;
    QToolButton *m_saveButton = nullptr;
    QToolButton *m_toggleToolbarButton = nullptr;
    QToolButton *m_closeButton = nullptr;
    QColor m_currentPenColor = QColor("#F43F5E");
    int m_currentPenSize = 6;
    QString m_annotationText = QStringLiteral("文本");
    QPoint m_pendingTextPoint;
    bool m_textBackgroundEnabled = true;
    bool m_numberAutoIncrement = true;
    int m_numberValue = 1;
    bool m_toolbarVisible = true;
    bool m_shadowVisible = true;
    qreal m_zoomFactor = 1.0;
    bool m_released = false;
};

#endif // STICKY_PIN_WIDGET_H
