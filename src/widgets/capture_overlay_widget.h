#ifndef CAPTURE_OVERLAY_WIDGET_H
#define CAPTURE_OVERLAY_WIDGET_H

#include <QPointer>
#include <QColor>
#include <QPoint>
#include <QString>
#include <QVector>
#include <QWidget>

#include "paint_board/shape/shape_type.h"
#include "widgets/selection_mask_controller.h"

class QToolButton;
class QLabel;
class QProgressBar;
class QTimer;
class QCheckBox;
class QLineEdit;
class ScreenshotViewModel;
class StickyViewModel;
class StorageViewModel;
class OcrViewModel;
class GifRecordViewModel;
class GifRecordOverlayWidget;
class MagnifierWidget;
class WidgetWindowBridge;
class StickyCanvasWidget;

class CaptureOverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CaptureOverlayWidget(ScreenshotViewModel *screenCapture,
                                  StickyViewModel *stickyViewModel,
                                  StorageViewModel *storageViewModel,
                                  OcrViewModel *ocrViewModel,
                                  GifRecordViewModel *gifRecordViewModel = nullptr,
                                  WidgetWindowBridge *widgetWindowBridge = nullptr,
                                  QWidget *parent = nullptr);

    void showAndActivate(const QString &mode = QStringLiteral("copy"));
    void toggleCapture(const QString &mode = QStringLiteral("copy"));
    void setWidgetWindowBridge(WidgetWindowBridge *widgetWindowBridge);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class CaptureAction {
        Copy,
        Save,
        Sticky,
        Ocr,
        LongCapture,
        Gif
    };

    void resetState();
    void refreshSnapshot();
    QRect selectionInGlobal() const;
    QRect currentSelectionRect() const;
    QImage currentCaptureImage() const;
    QImage currentOutputImage() const;
    void refreshCanvasForSelection(bool clearAnnotations);
    void setActiveTool(Shapeype type, QToolButton *button);
    void syncCanvasToolSettings();
    void updateToolOptionsPanel();
    QToolButton *currentActiveToolButton() const;
    void updateToolbarGeometry();
    void updateMagnifier(const QPoint &localPos);
    void updateToolOptionsGeometry();
    QRect toolOptionsRect() const;
    void showTipBubble(QToolButton *button);
    void hideTipBubble();
    void setDefaultAction(const QString &mode);
    void performAction(CaptureAction action);
    void finishCapture();
    QString defaultSaveFilePath() const;

    QPointer<ScreenshotViewModel> m_screenCapture;
    QPointer<StickyViewModel> m_stickyViewModel;
    QPointer<StorageViewModel> m_storageViewModel;
    QPointer<OcrViewModel> m_ocrViewModel;
    QPointer<GifRecordViewModel> m_gifRecordViewModel;
    QPointer<WidgetWindowBridge> m_widgetWindowBridge;

    QRect m_virtualGeometry;
    CaptureAction m_defaultAction = CaptureAction::Copy;
    SelectionMaskController m_selection;
    bool m_gifRecordingMode = false;
    QRect m_gifGlobalCaptureRect;

    QWidget *m_toolbar = nullptr;
    StickyCanvasWidget *m_canvas = nullptr;
    QWidget *m_toolOptions = nullptr;
    QWidget *m_inlineTextPanel = nullptr;
    QLineEdit *m_inlineTextEditor = nullptr;
    QToolButton *m_inlineTextConfirmButton = nullptr;
    QToolButton *m_inlineTextCancelButton = nullptr;
    QLineEdit *m_textInput = nullptr;
    QCheckBox *m_textBackgroundCheck = nullptr;
    QCheckBox *m_numberAutoIncrementCheck = nullptr;
    QToolButton *m_numberMinusButton = nullptr;
    QLabel *m_numberLabel = nullptr;
    QToolButton *m_numberPlusButton = nullptr;
    QToolButton *m_copyButton = nullptr;
    QToolButton *m_saveButton = nullptr;
    QToolButton *m_stickyButton = nullptr;
    QToolButton *m_ocrButton = nullptr;
    QToolButton *m_longCaptureButton = nullptr;
    QToolButton *m_gifButton = nullptr;
    QToolButton *m_cancelButton = nullptr;
    QToolButton *m_pencilButton = nullptr;
    QToolButton *m_rectButton = nullptr;
    QToolButton *m_circleButton = nullptr;
    QToolButton *m_arrowButton = nullptr;
    QToolButton *m_mosaicButton = nullptr;
    QToolButton *m_blurButton = nullptr;
    QToolButton *m_textButton = nullptr;
    QToolButton *m_numberButton = nullptr;
    QToolButton *m_undoButton = nullptr;
    QVector<QToolButton*> m_colorButtons;
    QVector<QToolButton*> m_sizeButtons;
    QColor m_currentPenColor = QColor("#F43F5E");
    int m_currentPenSize = 6;
    QString m_annotationText = QStringLiteral("文本");
    bool m_textBackgroundEnabled = true;
    bool m_numberAutoIncrement = true;
    int m_numberValue = 1;
    QPoint m_pendingTextPoint;

    GifRecordOverlayWidget *m_gifOverlay = nullptr;
    MagnifierWidget *m_magnifier = nullptr;
    QWidget *m_tipBubble = nullptr;
    QLabel *m_tipLabel = nullptr;
};

#endif // CAPTURE_OVERLAY_WIDGET_H
