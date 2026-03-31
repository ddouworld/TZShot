#include "widgets/capture_overlay_widget.h"

#include "widgets/gif_record_overlay_widget.h"
#include "widgets/magnifier_widget.h"
#include "widgets/sticky_canvas_widget.h"
#include "widgets/widget_window_bridge.h"
#include "viewmodel/gif_record_view_model.h"
#include "viewmodel/ocr_view_model.h"
#include "viewmodel/screenshot_view_model.h"
#include "viewmodel/sticky_view_model.h"
#include "viewmodel/storage_view_model.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDateTime>
#include <QEvent>
#include <QDir>
#include <QFileDialog>
#include <QFile>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPainter>
#include <QProgressBar>
#include <QScreen>
#include <QSizePolicy>
#include <QStyle>
#include <QToolButton>
#include <QFrame>
#include <QLabel>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QTimer>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>
#include <QVariant>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

class AnnotationTextPanel : public QWidget
{
public:
    explicit AnnotationTextPanel(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TranslucentBackground, true);
    }

    void setBorderColor(const QColor &color)
    {
        if (m_borderColor == color) {
            return;
        }
        m_borderColor = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QWidget::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 1));
        painter.drawRect(rect());

        QPen pen(m_borderColor);
        pen.setWidth(1);
        pen.setStyle(Qt::CustomDashLine);
        pen.setDashPattern({ 3, 3 });
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }

private:
    QColor m_borderColor = QColor(QStringLiteral("#F43F5E"));
};

QString loadCaptureOverlayStyleSheet()
{
    QFile file(QStringLiteral(":/resource/qss/capture_overlay_widget.qss"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

QToolButton *createToolbarButton(const QString &iconPath,
                                 const QString &toolTip,
                                 QWidget *parent)
{
    auto *button = new QToolButton(parent);
    button->setIcon(QIcon(iconPath));
    button->setToolTip(toolTip);
    button->setAutoRaise(false);
    button->setCheckable(false);
    button->setFixedSize(32, 32);
    button->setIconSize(QSize(18, 18));
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

QString normalizeAnnotationDraft(QString text)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return text;
}

QString finalizeAnnotationText(const QString &text)
{
    return normalizeAnnotationDraft(text).trimmed();
}

void configureAnnotationEditor(QPlainTextEdit *editor)
{
    if (!editor) {
        return;
    }

    editor->setFrameStyle(QFrame::NoFrame);
    editor->setBackgroundVisible(false);
    editor->setTabChangesFocus(false);
    editor->setWordWrapMode(QTextOption::NoWrap);
    editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

int annotationTextPixelSize(int penSize)
{
    return qMax(12, penSize * 4);
}

QFont annotationTextFont(int penSize)
{
    QFont font(QStringLiteral("Microsoft YaHei"));
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setHintingPreference(QFont::PreferNoHinting);
    font.setPixelSize(annotationTextPixelSize(penSize));
    return font;
}

void syncAnnotationEditorFont(QPlainTextEdit *editor, int penSize)
{
    if (!editor) {
        return;
    }

    editor->setFont(annotationTextFont(penSize));
}

void syncInlineAnnotationEditorAppearance(QWidget *panel,
                                          QPlainTextEdit *editor,
                                          const QColor &color,
                                          int penSize)
{
    if (!panel || !editor) {
        return;
    }

    syncAnnotationEditorFont(editor, penSize);
    editor->setStyleSheet(QStringLiteral("color:%1; margin:0px; padding:0px; background:transparent; border:none;")
        .arg(color.name()));
    static_cast<AnnotationTextPanel *>(panel)->setBorderColor(color);
}

void syncOptionButtonState(const QVector<QToolButton *> &buttons,
                           const char *propertyName,
                           const QVariant &expectedValue)
{
    for (QToolButton *button : buttons) {
        if (!button) {
            continue;
        }
        button->setChecked(button->property(propertyName) == expectedValue);
    }
}

}

CaptureOverlayWidget::CaptureOverlayWidget(ScreenshotViewModel *screenCapture,
                                           StickyViewModel *stickyViewModel,
                                           StorageViewModel *storageViewModel,
                                           OcrViewModel *ocrViewModel,
                                           GifRecordViewModel *gifRecordViewModel,
                                           WidgetWindowBridge *widgetWindowBridge,
                                           QWidget *parent)
    : QWidget(parent)
    , m_screenCapture(screenCapture)
    , m_stickyViewModel(stickyViewModel)
    , m_storageViewModel(storageViewModel)
    , m_ocrViewModel(ocrViewModel)
    , m_gifRecordViewModel(gifRecordViewModel)
    , m_widgetWindowBridge(widgetWindowBridge)
{
    m_annotationText.clear();

    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    setStyleSheet(loadCaptureOverlayStyleSheet());

    m_magnifier = new MagnifierWidget(this);
    m_magnifierUpdateTimer = new QTimer(this);
    m_magnifierUpdateTimer->setSingleShot(true);
    connect(m_magnifierUpdateTimer, &QTimer::timeout, this, [this]() {
        if (!m_hasPendingMagnifierPos) {
            return;
        }
        m_hasPendingMagnifierPos = false;
        updateMagnifier(m_pendingMagnifierPos);
        m_magnifierFrameTimer.restart();
    });
    m_canvas = new StickyCanvasWidget(this);
    m_canvas->hide();
    m_canvas->setViewScale(1.0);
    m_canvas->setBackgroundVisible(false);
    m_canvas->setPenColor(m_currentPenColor);
    m_canvas->setPenSize(m_currentPenSize);
    m_canvas->setActiveShapeType(PEN);
    m_canvas->setNumberAutoIncrement(m_numberAutoIncrement);
    m_canvas->setNumberValue(m_numberValue);
    connect(m_canvas, &StickyCanvasWidget::numberValueChanged, this, [this](int value) {
        m_numberValue = value;
        if (m_numberLabel) {
            m_numberLabel->setText(QString::number(value));
        }
        if (m_numberAutoIncrementCheck) {
            m_numberAutoIncrementCheck->setChecked(m_numberAutoIncrement);
        }
    });
    connect(m_canvas, &StickyCanvasWidget::textPlacementRequested, this, [this](const QPoint &point) {
        m_inlineEditingExistingText = false;
        m_inlineOriginalText.clear();
        m_annotationText.clear();
        showInlineTextEditor(point, QString());
    });
    connect(m_canvas, &StickyCanvasWidget::textEditRequested, this,
            [this](const QPoint &point,
                   const QString &text,
                   const QColor &color,
                   int size,
                   bool withBackground) {
        m_inlineEditingExistingText = true;
        m_inlineOriginalText = text;
        m_inlineOriginalColor = color;
        m_inlineOriginalSize = size;
        m_inlineOriginalTextBackground = withBackground;
        m_currentPenColor = color;
        m_currentPenSize = size;
        m_annotationText = text;
        syncCanvasToolSettings();
        updateToolOptionsPanel();
        updateToolOptionsGeometry();
        showInlineTextEditor(point, text);
    });

    m_toolOptions = new QWidget(this);
    m_toolOptions->setObjectName(QStringLiteral("captureOverlayToolOptions"));
    m_toolOptions->setAttribute(Qt::WA_StyledBackground, true);
    m_toolOptions->hide();

    const QVector<QColor> colors {
        QColor("#F43F5E"), QColor("#EF4444"), QColor("#F97316"), QColor("#EAB308"),
        QColor("#22C55E"), QColor("#3B82F6"), QColor("#8B5CF6"), QColor("#000000"), QColor("#FFFFFF")
    };
    for (const QColor &color : colors) {
        auto *button = new QToolButton(m_toolOptions);
        button->setCheckable(true);
        button->setAutoRaise(false);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setCursor(Qt::PointingHandCursor);
        button->setProperty("annotationColor", color);
        button->setStyleSheet(QStringLiteral(
            "QToolButton { border: 1px solid #CBD5E1; border-radius: 9px; background-color: %1; }"
            "QToolButton:checked { border: 2px solid #3B82F6; background-color: %1; }"
        ).arg(color.name()));
        connect(button, &QToolButton::clicked, this, [this, button, color]() {
            for (QToolButton *other : m_colorButtons) {
                if (other != button) {
                    other->setChecked(false);
                }
            }
            button->setChecked(true);
            m_currentPenColor = color;
            syncCanvasToolSettings();
        });
        m_colorButtons.append(button);
    }

    struct SizePreset { int value; QString label; };
    const QVector<SizePreset> sizePresets {
        { 2, tr("细") }, { 4, tr("中") }, { 6, tr("粗") }, { 10, tr("特粗") }
    };
    for (const SizePreset &preset : sizePresets) {
        auto *button = new QToolButton(m_toolOptions);
        button->setCheckable(true);
        button->setText(preset.label);
        button->setCursor(Qt::PointingHandCursor);
        button->setProperty("annotationSize", preset.value);
        connect(button, &QToolButton::clicked, this, [this, button, preset]() {
            for (QToolButton *other : m_sizeButtons) {
                if (other != button) {
                    other->setChecked(false);
                }
            }
            button->setChecked(true);
            m_currentPenSize = preset.value;
            syncCanvasToolSettings();
            if (m_canvas && m_canvas->drawingEnabled() && m_canvas->activeShapeType() == TEXT) {
                updateToolOptionsPanel();
                updateToolOptionsGeometry();
            }
        });
        m_sizeButtons.append(button);
    }

    m_inlineTextPanel = new AnnotationTextPanel(this);
    m_inlineTextPanel->setObjectName(QStringLiteral("captureOverlayInlineTextPanel"));
    m_inlineTextPanel->setAttribute(Qt::WA_StyledBackground, true);
    m_inlineTextPanel->setAttribute(Qt::WA_TranslucentBackground, true);
    m_inlineTextPanel->hide();
    auto *inlineLayout = new QHBoxLayout(m_inlineTextPanel);
    inlineLayout->setContentsMargins(3, 3, 3, 3);
    inlineLayout->setSpacing(0);

    m_inlineTextEditor = new QPlainTextEdit(m_inlineTextPanel);
    m_inlineTextEditor->setObjectName(QStringLiteral("captureOverlayInlineTextEditor"));
    configureAnnotationEditor(m_inlineTextEditor);
    m_inlineTextEditor->setPlaceholderText(QString());
    m_inlineTextEditor->installEventFilter(this);
    inlineLayout->addWidget(m_inlineTextEditor, 1);
    connect(m_inlineTextEditor, &QPlainTextEdit::textChanged, this, [this]() {
        m_annotationText = normalizeAnnotationDraft(m_inlineTextEditor->toPlainText());
        if (m_inlineTextPanel && m_inlineTextPanel->isVisible()) {
            updateInlineTextEditorGeometry();
        }
    });

    m_numberAutoIncrementCheck = new QCheckBox(tr("自动递增"), m_toolOptions);
    m_numberAutoIncrementCheck->setChecked(m_numberAutoIncrement);
    connect(m_numberAutoIncrementCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_numberAutoIncrement = checked;
        if (m_canvas) {
            m_canvas->setNumberAutoIncrement(checked);
        }
    });

    m_numberMinusButton = new QToolButton(m_toolOptions);
    m_numberMinusButton->setText(QStringLiteral("-"));
    m_numberMinusButton->setCursor(Qt::PointingHandCursor);
    connect(m_numberMinusButton, &QToolButton::clicked, this, [this]() {
        const int nextValue = qMax(1, m_numberValue - 1);
        m_numberValue = nextValue;
        if (m_numberLabel) {
            m_numberLabel->setText(QString::number(nextValue));
        }
        if (m_canvas) {
            m_canvas->setNumberValue(nextValue);
        }
    });

    m_numberLabel = new QLabel(QString::number(m_numberValue), m_toolOptions);
    m_numberLabel->setObjectName(QStringLiteral("captureOverlayNumberLabel"));
    m_numberLabel->setAlignment(Qt::AlignCenter);

    m_numberPlusButton = new QToolButton(m_toolOptions);
    m_numberPlusButton->setText(QStringLiteral("+"));
    m_numberPlusButton->setCursor(Qt::PointingHandCursor);
    connect(m_numberPlusButton, &QToolButton::clicked, this, [this]() {
        const int nextValue = qMin(9999, m_numberValue + 1);
        m_numberValue = nextValue;
        if (m_numberLabel) {
            m_numberLabel->setText(QString::number(nextValue));
        }
        if (m_canvas) {
            m_canvas->setNumberValue(nextValue);
        }
    });

    m_toolbar = new QWidget(this);
    m_toolbar->setObjectName(QStringLiteral("captureOverlayToolbar"));
    m_toolbar->setAttribute(Qt::WA_StyledBackground, true);
    auto *toolbarLayout = new QHBoxLayout(m_toolbar);
    toolbarLayout->setContentsMargins(6, 5, 6, 5);
    toolbarLayout->setSpacing(2);

    m_pencilButton = createToolbarButton(QStringLiteral(":/resource/img/lc_pencil.svg"), tr("画笔"), m_toolbar);
    m_rectButton = createToolbarButton(QStringLiteral(":/resource/img/lc_square.svg"), tr("矩形"), m_toolbar);
    m_circleButton = createToolbarButton(QStringLiteral(":/resource/img/lc_circle.svg"), tr("圆形"), m_toolbar);
    m_arrowButton = createToolbarButton(QStringLiteral(":/resource/img/lc_arrow.svg"), tr("箭头"), m_toolbar);
    m_mosaicButton = createToolbarButton(QStringLiteral(":/resource/img/lc_mosaic.svg"), tr("马赛克"), m_toolbar);
    m_blurButton = createToolbarButton(QStringLiteral(":/resource/img/lc_blur.svg"), tr("高斯模糊"), m_toolbar);
    m_textButton = createToolbarButton(QStringLiteral(":/resource/img/lc_text.svg"), tr("文字"), m_toolbar);
    m_numberButton = createToolbarButton(QStringLiteral(":/resource/img/lc_list_ordered.svg"), tr("序号"), m_toolbar);
    m_undoButton = createToolbarButton(QStringLiteral(":/resource/img/lc_undo.svg"), tr("撤销"), m_toolbar);
    m_copyButton = createToolbarButton(QStringLiteral(":/resource/img/lc_copy.svg"), tr("复制"), m_toolbar);
    m_saveButton = createToolbarButton(QStringLiteral(":/resource/img/lc_save.svg"), tr("保存"), m_toolbar);
    m_stickyButton = createToolbarButton(QStringLiteral(":/resource/img/lc_pin.svg"), tr("贴图"), m_toolbar);
    m_ocrButton = createToolbarButton(QStringLiteral(":/resource/img/lc_ocr.svg"), tr("OCR"), m_toolbar);
    m_longCaptureButton = createToolbarButton(QStringLiteral(":/resource/img/lc_longshot.svg"), tr("长截图"), m_toolbar);
    m_gifButton = createToolbarButton(QStringLiteral(":/resource/img/lc_gif_record.svg"), tr("录制 GIF"), m_toolbar);
    m_cancelButton = createToolbarButton(QStringLiteral(":/resource/img/lc_x.svg"), tr("取消"), m_toolbar);

    const QVector<QToolButton*> drawButtons {
        m_pencilButton, m_rectButton, m_circleButton, m_arrowButton, m_mosaicButton, m_blurButton, m_textButton, m_numberButton
    };
    for (QToolButton *button : drawButtons) {
        button->setCheckable(true);
        toolbarLayout->addWidget(button);
    }
    toolbarLayout->addWidget(m_undoButton);
    auto *divider = new QFrame(m_toolbar);
    divider->setObjectName(QStringLiteral("captureOverlayToolbarDivider"));
    divider->setFixedSize(1, 18);
    toolbarLayout->addWidget(divider, 0, Qt::AlignVCenter);
    toolbarLayout->addWidget(m_copyButton);
    toolbarLayout->addWidget(m_saveButton);
    toolbarLayout->addWidget(m_stickyButton);
    toolbarLayout->addWidget(m_ocrButton);
    toolbarLayout->addWidget(m_longCaptureButton);
    toolbarLayout->addWidget(m_gifButton);
    auto *divider2 = new QFrame(m_toolbar);
    divider2->setObjectName(QStringLiteral("captureOverlayToolbarDivider"));
    divider2->setFixedSize(1, 18);
    toolbarLayout->addWidget(divider2, 0, Qt::AlignVCenter);
    toolbarLayout->addWidget(m_cancelButton);

    m_toolbar->hide();

    connect(m_copyButton, &QToolButton::clicked, this, [this]() { performAction(CaptureAction::Copy); });
    connect(m_saveButton, &QToolButton::clicked, this, [this]() { performAction(CaptureAction::Save); });
    connect(m_stickyButton, &QToolButton::clicked, this, [this]() { performAction(CaptureAction::Sticky); });
    connect(m_ocrButton, &QToolButton::clicked, this, [this]() { performAction(CaptureAction::Ocr); });
    connect(m_longCaptureButton, &QToolButton::clicked, this, [this]() { performAction(CaptureAction::LongCapture); });
    connect(m_gifButton, &QToolButton::clicked, this, [this]() { performAction(CaptureAction::Gif); });
    connect(m_cancelButton, &QToolButton::clicked, this, [this]() { finishCapture(); });
    connect(m_undoButton, &QToolButton::clicked, this, [this]() {
        if (m_canvas) {
            m_canvas->undo();
        }
    });

    const auto setOnlyChecked = [drawButtons](QToolButton *activeButton) {
        for (QToolButton *button : drawButtons) {
            if (button != activeButton) {
                button->setChecked(false);
            }
        }
    };
    connect(m_pencilButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_pencilButton); setActiveTool(PEN, m_pencilButton); });
    connect(m_rectButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_rectButton); setActiveTool(RECTANGLE, m_rectButton); });
    connect(m_circleButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_circleButton); setActiveTool(ELLIPSE, m_circleButton); });
    connect(m_arrowButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_arrowButton); setActiveTool(ARROW, m_arrowButton); });
    connect(m_mosaicButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_mosaicButton); setActiveTool(MOSAIC, m_mosaicButton); });
    connect(m_blurButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_blurButton); setActiveTool(BLUR, m_blurButton); });
    connect(m_textButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_textButton); setActiveTool(TEXT, m_textButton); });
    connect(m_numberButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_numberButton); setActiveTool(NUMBER, m_numberButton); });

    const QList<QToolButton *> tipButtons {
        m_pencilButton,
        m_rectButton,
        m_circleButton,
        m_arrowButton,
        m_mosaicButton,
        m_blurButton,
        m_textButton,
        m_numberButton,
        m_undoButton,
        m_copyButton,
        m_saveButton,
        m_stickyButton,
        m_ocrButton,
        m_longCaptureButton,
        m_gifButton,
        m_cancelButton
    };
    for (QToolButton *button : tipButtons) {
        if (button) {
            button->installEventFilter(this);
        }
    }

    m_gifOverlay = new GifRecordOverlayWidget();
    connect(this, &QObject::destroyed, m_gifOverlay, &QObject::deleteLater);
    connect(m_gifOverlay, &GifRecordOverlayWidget::stopRequested, this, [this]() {
        if (m_gifRecordViewModel) {
            m_gifRecordViewModel->stopRecording();
        }
    });
    connect(m_gifOverlay, &GifRecordOverlayWidget::cancelRequested, this, [this]() {
        if (m_gifRecordViewModel) {
            m_gifRecordViewModel->cancelRecording();
        }
        finishCapture();
    });

    m_tipBubble = new QWidget(this);
    m_tipBubble->setObjectName(QStringLiteral("captureOverlayTipBubble"));
    m_tipBubble->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_tipBubble->setAttribute(Qt::WA_StyledBackground, true);
    auto *tipLayout = new QHBoxLayout(m_tipBubble);
    tipLayout->setContentsMargins(14, 4, 14, 4);
    tipLayout->setSpacing(0);
    m_tipLabel = new QLabel(m_tipBubble);
    m_tipLabel->setObjectName(QStringLiteral("captureOverlayTipLabel"));
    tipLayout->addWidget(m_tipLabel);
    m_tipBubble->hide();

    if (m_gifRecordViewModel) {
        connect(m_gifRecordViewModel, &GifRecordViewModel::frameCountChanged, this, [this]() {
            if (m_gifOverlay) {
                m_gifOverlay->setFrameCount(m_gifRecordViewModel->frameCount());
            }
        });
        connect(m_gifRecordViewModel, &GifRecordViewModel::elapsedSecsChanged, this, [this]() {
            if (m_gifOverlay) {
                m_gifOverlay->setElapsedSecs(m_gifRecordViewModel->elapsedSecs());
            }
        });
        connect(m_gifRecordViewModel, &GifRecordViewModel::isRecordingChanged, this, [this]() {
            if (m_gifOverlay) {
                m_gifOverlay->setRecordingState(m_gifRecordViewModel->isRecording(),
                                                m_gifRecordViewModel->isEncoding());
            }
        });
        connect(m_gifRecordViewModel, &GifRecordViewModel::isEncodingChanged, this, [this]() {
            if (m_gifOverlay) {
                m_gifOverlay->setRecordingState(m_gifRecordViewModel->isRecording(),
                                                m_gifRecordViewModel->isEncoding());
            }
        });
        connect(m_gifRecordViewModel, &GifRecordViewModel::encodingFinished, this, [this](const QString &) {
            finishCapture();
        });
        connect(m_gifRecordViewModel, &GifRecordViewModel::encodingFailed, this, [this](const QString &) {
            finishCapture();
        });
    }

    if (!m_colorButtons.isEmpty()) {
        m_colorButtons.first()->setChecked(true);
    }
    for (QToolButton *sizeButton : m_sizeButtons) {
        if (sizeButton->text() == tr("粗")) {
            sizeButton->setChecked(true);
            break;
        }
    }
    syncCanvasToolSettings();
}

void CaptureOverlayWidget::showAndActivate(const QString &mode)
{
    setDefaultAction(mode);
    refreshSnapshot();
    resetState();
    m_magnifierFrameTimer.invalidate();
    m_hasPendingMagnifierPos = false;
    if (m_magnifierUpdateTimer) {
        m_magnifierUpdateTimer->stop();
    }
    if (!m_virtualGeometry.isEmpty()) {
        setGeometry(m_virtualGeometry);
    }
    show();
#ifdef Q_OS_WIN
    if (!m_virtualGeometry.isEmpty()) {
        if (HWND hwnd = reinterpret_cast<HWND>(winId())) {
            SetWindowPos(hwnd,
                         HWND_TOPMOST,
                         m_virtualGeometry.x(),
                         m_virtualGeometry.y(),
                         m_virtualGeometry.width(),
                         m_virtualGeometry.height(),
                         SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
        }
    }
#endif
    raise();
    activateWindow();
    setFocus(Qt::ActiveWindowFocusReason);
    if (m_magnifier) {
        m_magnifier->hide();
    }
}

void CaptureOverlayWidget::toggleCapture(const QString &mode)
{
    if (isVisible()) {
        finishCapture();
        return;
    }
    showAndActivate(mode);
}

void CaptureOverlayWidget::setWidgetWindowBridge(WidgetWindowBridge *widgetWindowBridge)
{
    m_widgetWindowBridge = widgetWindowBridge;
}

void CaptureOverlayWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    const QRect dirtyRect = event ? event->rect() : rect();

    if (m_screenCapture) {
        const QPixmap snapshot = m_screenCapture->desktopSnapshot();
        const qreal dpr = qMax<qreal>(1.0, snapshot.devicePixelRatio());
        const QSize logicalSize(qRound(snapshot.width() / dpr), qRound(snapshot.height() / dpr));
        if (!snapshot.isNull() && logicalSize == size()) {
            painter.drawPixmap(dirtyRect, snapshot, dirtyRect);
        } else {
            painter.drawPixmap(rect(), snapshot);
        }
    } else {
        painter.fillRect(dirtyRect, Qt::black);
    }

    painter.setRenderHint(QPainter::Antialiasing, true);
    if (event) {
        painter.setClipRegion(event->region());
    }
    m_selection.paint(painter, rect());
}

void CaptureOverlayWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_canvas && m_canvas->isVisible()) {
        m_canvas->setGeometry(rect());
    }
}

void CaptureOverlayWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (m_gifRecordingMode) {
        event->accept();
        return;
    }

    if (m_canvas && m_canvas->drawingEnabled()) {
        event->ignore();
        return;
    }

    m_toolbar->hide();
    if (m_toolOptions) {
        m_toolOptions->hide();
    }
    hideTipBubble();
    cancelInlineText();
    const QRect previousSelection = currentSelectionRect();
    m_selection.beginInteraction(event->position().toPoint());
    setCursor(m_selection.cursorShape());
    updateSelectionVisualRegion(previousSelection, currentSelectionRect());
}

void CaptureOverlayWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint pos = event->position().toPoint();

    if (m_gifRecordingMode) {
        m_hasPendingMagnifierPos = false;
        if (m_magnifierUpdateTimer) {
            m_magnifierUpdateTimer->stop();
        }
        QWidget::mouseMoveEvent(event);
        return;
    }

    if (m_canvas && m_canvas->drawingEnabled()) {
        m_hasPendingMagnifierPos = false;
        if (m_magnifierUpdateTimer) {
            m_magnifierUpdateTimer->stop();
        }
        QWidget::mouseMoveEvent(event);
        return;
    }

    if (event->buttons() & Qt::LeftButton) {
        const QRect previousSelection = currentSelectionRect();
        m_selection.updateInteraction(pos, size());
        setCursor(m_selection.cursorShape());
        updateSelectionVisualRegion(previousSelection, currentSelectionRect());
        scheduleMagnifierUpdate(pos);
        return;
    }

    const QRect selectionBeforeHover = currentSelectionRect();
    m_selection.updateHover(pos);
    if (m_toolbar && !m_toolbar->isVisible()) {
        scheduleMagnifierUpdate(pos);
    } else if (m_magnifier) {
        m_hasPendingMagnifierPos = false;
        if (m_magnifierUpdateTimer) {
            m_magnifierUpdateTimer->stop();
        }
        m_magnifier->hide();
    }
    updateSelectionVisualRegion(selectionBeforeHover, currentSelectionRect());
    setCursor(m_selection.cursorShape());
    QWidget::mouseMoveEvent(event);
}

void CaptureOverlayWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_canvas && m_canvas->drawingEnabled()) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (m_gifRecordingMode) {
            event->accept();
            return;
        }
        const QRect previousSelection = currentSelectionRect();
        m_selection.endInteraction();
        m_selection.updateHover(event->position().toPoint());
        updateSelectionVisualRegion(previousSelection, currentSelectionRect());
        setCursor(m_selection.cursorShape());
        if (m_selection.hasSelection()) {
            refreshCanvasForSelection(false);
            if (m_defaultAction == CaptureAction::Sticky || m_defaultAction == CaptureAction::Save) {
                const CaptureAction action = m_defaultAction;
                if (m_magnifier) {
                    m_magnifier->hide();
                }
                update(selectionVisualRect(currentSelectionRect()));
                event->accept();
                QTimer::singleShot(0, this, [this, action]() {
                    if (isVisible() && m_selection.hasSelection()) {
                        performAction(action);
                    }
                });
                return;
            }
            updateToolbarGeometry();
            m_toolbar->show();
            m_toolbar->raise();
            updateToolOptionsPanel();
            updateToolOptionsGeometry();
        }
        if (m_magnifier) {
            m_magnifier->hide();
        }
        update();
    }
    QWidget::mouseReleaseEvent(event);
}

void CaptureOverlayWidget::keyPressEvent(QKeyEvent *event)
{
    if (event
        && event->matches(QKeySequence::Undo)
        && (!m_inlineTextPanel || !m_inlineTextPanel->isVisible())
        && m_canvas
        && m_canvas->hasAnnotations()) {
        m_canvas->undo();
        event->accept();
        return;
    }

    switch (event->key()) {
    case Qt::Key_Escape:
        if (m_gifRecordingMode && m_gifRecordViewModel) {
            m_gifRecordViewModel->cancelRecording();
        }
        finishCapture();
        event->accept();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_selection.hasSelection()) {
            performAction(m_defaultAction);
        }
        event->accept();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void CaptureOverlayWidget::closeEvent(QCloseEvent *event)
{
    if (m_screenCapture) {
        m_screenCapture->releaseDesktopSnapshot();
    }
    QWidget::closeEvent(event);
}

bool CaptureOverlayWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_inlineTextEditor && m_inlineTextEditor) {
        if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                cancelInlineText();
                return true;
            }
            if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
                && keyEvent->modifiers().testFlag(Qt::ControlModifier)) {
                submitInlineText();
                return true;
            }
            if (keyEvent->matches(QKeySequence::Undo)) {
                return QWidget::eventFilter(watched, event);
            }
        }
        if (event->type() == QEvent::Wheel) {
            auto *wheelEvent = static_cast<QWheelEvent *>(event);
            const int delta = wheelEvent->angleDelta().y();
            if (delta != 0) {
                m_currentPenSize = qBound(3, m_currentPenSize + (delta > 0 ? 1 : -1), 40);
                syncCanvasToolSettings();
                updateToolOptionsPanel();
                updateToolOptionsGeometry();
                wheelEvent->accept();
                return true;
            }
        }
        if (event->type() == QEvent::FocusOut && m_inlineTextPanel && m_inlineTextPanel->isVisible()) {
            submitInlineText();
        }
        return QWidget::eventFilter(watched, event);
    }

    if (event && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->matches(QKeySequence::Undo)
            && watched != m_inlineTextEditor
            && (!m_inlineTextPanel || !m_inlineTextPanel->isVisible())
            && m_canvas
            && m_canvas->hasAnnotations()) {
            m_canvas->undo();
            keyEvent->accept();
            return true;
        }
    }

    auto *button = qobject_cast<QToolButton *>(watched);
    if (!button) {
        return QWidget::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::Enter:
        showTipBubble(button);
        break;
    case QEvent::Leave:
    case QEvent::MouseButtonPress:
    case QEvent::Hide:
        hideTipBubble();
        break;
    default:
        break;
    }

    return QWidget::eventFilter(watched, event);
}

void CaptureOverlayWidget::showInlineTextEditor(const QPoint &point, const QString &initialText)
{
    m_pendingTextPoint = point;
    if (!m_inlineTextEditor || !m_inlineTextPanel || !m_canvas) {
        return;
    }

    m_annotationText = normalizeAnnotationDraft(initialText);
    if (m_inlineTextEditor->toPlainText() != m_annotationText) {
        m_inlineTextEditor->setPlainText(m_annotationText);
    }
    updateInlineTextEditorGeometry();
    m_inlineTextPanel->show();
    m_inlineTextPanel->raise();
    m_inlineTextEditor->setFocus();
    m_inlineTextEditor->moveCursor(QTextCursor::End);
}

void CaptureOverlayWidget::updateInlineTextEditorGeometry()
{
    if (!m_inlineTextEditor || !m_inlineTextPanel || !m_canvas) {
        return;
    }

    const QRect canvasRect = m_canvas->geometry();
    m_inlineTextEditor->document()->adjustSize();
    const QSize docSize = m_inlineTextEditor->document()->size().toSize();
    const QFontMetrics fm(m_inlineTextEditor->font());
    int panelW = qMax(fm.horizontalAdvance(QStringLiteral(" ")), docSize.width()) + 6;
    int panelH = qMax(fm.lineSpacing(), docSize.height()) + 6;
    panelW = qMin(panelW, qMax(1, canvasRect.width() - 12));
    panelW = qMin(panelW, qMax(1, width() - 12));
    panelH = qMin(panelH, qMax(36, height() - 12));
    const int padY = qMax(2, m_currentPenSize / 2);
    int x = canvasRect.x() + m_pendingTextPoint.x();
    int y = canvasRect.y() + m_pendingTextPoint.y() - fm.ascent() - padY;
    x = qMax(6, qMin(x, width() - panelW - 6));
    y = qMax(6, qMin(y, height() - panelH - 6));

    m_inlineTextPanel->setGeometry(x, y, panelW, panelH);
}

void CaptureOverlayWidget::submitInlineText()
{
    if (!m_inlineTextEditor) {
        return;
    }

    const QString text = finalizeAnnotationText(m_inlineTextEditor->toPlainText());
    if (m_canvas && !text.isEmpty()) {
        m_annotationText = text;
        m_canvas->addTextAnnotation(m_pendingTextPoint,
                                    text,
                                    m_currentPenColor,
                                    m_currentPenSize,
                                    m_inlineEditingExistingText ? m_inlineOriginalTextBackground : false);
    }

    cancelInlineText(false);
}

void CaptureOverlayWidget::cancelInlineText(bool restoreOriginal)
{
    if (restoreOriginal && m_inlineEditingExistingText && m_canvas && !m_inlineOriginalText.isEmpty()) {
        m_annotationText = m_inlineOriginalText;
        m_canvas->addTextAnnotation(m_pendingTextPoint,
                                    m_inlineOriginalText,
                                    m_inlineOriginalColor,
                                    m_inlineOriginalSize,
                                    m_inlineOriginalTextBackground);
    }

    if (m_inlineTextPanel) {
        m_inlineTextPanel->hide();
    }
    m_pendingTextPoint = QPoint();
    m_inlineEditingExistingText = false;
    m_inlineOriginalText.clear();
    m_inlineOriginalColor = QColor("#F43F5E");
    m_inlineOriginalSize = 6;
    m_inlineOriginalTextBackground = false;
}

void CaptureOverlayWidget::resetState()
{
    m_selection.reset();
    cancelInlineText();
    m_hasPendingMagnifierPos = false;
    if (m_magnifierUpdateTimer) {
        m_magnifierUpdateTimer->stop();
    }
    m_toolbar->hide();
    if (m_toolOptions) {
        m_toolOptions->hide();
    }
    hideTipBubble();
    if (m_canvas) {
        m_canvas->setDrawingEnabled(false);
        m_canvas->reset();
        m_canvas->hide();
    }
    const QVector<QToolButton*> drawButtons {
        m_pencilButton, m_rectButton, m_circleButton, m_arrowButton, m_mosaicButton, m_blurButton, m_textButton, m_numberButton
    };
    for (QToolButton *button : drawButtons) {
        if (button) {
            button->setChecked(false);
        }
    }
    if (m_numberLabel) {
        m_numberLabel->setText(QString::number(m_numberValue));
    }
    if (m_magnifier) {
        m_magnifier->hide();
    }
    if (m_gifOverlay) {
        m_gifOverlay->resetOverlay();
    }
    m_gifRecordingMode = false;
    m_gifGlobalCaptureRect = {};
    setCursor(Qt::CrossCursor);
    update(rect());
}

void CaptureOverlayWidget::refreshSnapshot()
{
    if (!m_screenCapture) {
        return;
    }
    hide();
    qApp->processEvents();
    m_screenCapture->captureDesktop();
    m_virtualGeometry = m_screenCapture->virtualGeometry();
    if (m_canvas) {
        m_canvas->setGeometry(QRect(QPoint(0, 0), size()));
        m_canvas->setBackgroundImage(m_screenCapture->desktopSnapshot().toImage());
    }
    if (m_magnifier) {
        m_magnifier->setSnapshot(m_screenCapture->desktopSnapshot(), m_virtualGeometry);
    }
}

QRect CaptureOverlayWidget::selectionInGlobal() const
{
    const QRect selection = m_selection.rect().normalized();
    if (selection.isEmpty()) {
        return {};
    }

#ifdef Q_OS_WIN
    if (m_screenCapture) {
        const QPoint logicalGlobalTopLeft = mapToGlobal(selection.topLeft());
        const QRect logicalGlobalRect(logicalGlobalTopLeft, selection.size());
        const QRect physicalGlobalRect =
            m_screenCapture->mapLogicalGlobalRectToPhysicalRect(logicalGlobalRect);
        if (!physicalGlobalRect.isEmpty()) {
            return physicalGlobalRect;
        }
    }
#endif

    return selection.translated(m_virtualGeometry.topLeft());
}

QRect CaptureOverlayWidget::currentSelectionRect() const
{
    return m_selection.rect().normalized();
}

QImage CaptureOverlayWidget::currentCaptureImage() const
{
    if (!m_screenCapture) {
        return {};
    }

    const QRect globalRect = selectionInGlobal();
    if (globalRect.isEmpty()) {
        return {};
    }
    return m_screenCapture->captureRectToImage(globalRect);
}

QImage CaptureOverlayWidget::currentOutputImage() const
{
    const QRect selection = currentSelectionRect();
    if (selection.isEmpty()) {
        return {};
    }

    if (m_canvas && m_canvas->hasAnnotations()) {
        const QImage composited = m_canvas->compositedImage(selection);
        if (!composited.isNull()) {
            return composited;
        }
    }
    return currentCaptureImage();
}

void CaptureOverlayWidget::refreshCanvasForSelection(bool clearAnnotations)
{
    if (!m_canvas) {
        return;
    }

    if (clearAnnotations) {
        m_canvas->reset();
    }

    if (!m_screenCapture) {
        m_canvas->hide();
        return;
    }

    const QPixmap snapshot = m_screenCapture->desktopSnapshot();
    if (snapshot.isNull()) {
        m_canvas->hide();
        return;
    }

    m_canvas->setGeometry(rect());
    m_canvas->setViewScale(1.0);
    m_canvas->setBackgroundImage(snapshot.toImage());
    syncCanvasToolSettings();
    m_canvas->show();
    m_canvas->raise();
}

void CaptureOverlayWidget::setActiveTool(Shapeype type, QToolButton *button)
{
    const bool active = button && button->isChecked();
    if (!m_canvas) {
        return;
    }

    cancelInlineText();
    m_canvas->setDrawingEnabled(active);
    if (active) {
        m_canvas->setActiveShapeType(type);
    }
    syncCanvasToolSettings();
    updateToolOptionsPanel();
    updateToolOptionsGeometry();
}

void CaptureOverlayWidget::syncCanvasToolSettings()
{
    if (!m_canvas) {
        return;
    }

    m_canvas->setPenColor(m_currentPenColor);
    m_canvas->setPenSize(m_currentPenSize);
    m_canvas->setNumberAutoIncrement(m_numberAutoIncrement);
    m_canvas->setNumberValue(m_numberValue);

    syncOptionButtonState(m_colorButtons, "annotationColor", m_currentPenColor);
    syncOptionButtonState(m_sizeButtons, "annotationSize", m_currentPenSize);
    syncInlineAnnotationEditorAppearance(m_inlineTextPanel, m_inlineTextEditor, m_currentPenColor, m_currentPenSize);
    if (m_inlineTextPanel && m_inlineTextPanel->isVisible()) {
        updateInlineTextEditorGeometry();
    }
}

void CaptureOverlayWidget::updateToolOptionsPanel()
{
    if (!m_toolOptions || !m_canvas) {
        return;
    }

    const bool visible = m_toolbar->isVisible() && m_canvas->drawingEnabled() && !m_gifRecordingMode;
    m_toolOptions->setVisible(visible);
    if (!visible) {
        cancelInlineText();
        return;
    }

    const Shapeype type = m_canvas->activeShapeType();
    const bool showNumberOptions = (type == NUMBER);
    const bool showColorOptions = (type != MOSAIC && type != BLUR);

    int x = 10;
    int y = 10;
    for (QToolButton *button : m_colorButtons) {
        button->setVisible(showColorOptions);
        if (showColorOptions) {
            button->setGeometry(x, y, 18, 18);
            x += 24;
        }
    }

    y = showColorOptions ? (y + 26) : 10;
    x = 10;
    for (QToolButton *button : m_sizeButtons) {
        button->setVisible(true);
        button->setGeometry(x, y, 48, 28);
        x += 54;
    }

    if (m_numberAutoIncrementCheck && m_numberMinusButton && m_numberLabel && m_numberPlusButton) {
        m_numberAutoIncrementCheck->setVisible(showNumberOptions);
        m_numberMinusButton->setVisible(showNumberOptions);
        m_numberLabel->setVisible(showNumberOptions);
        m_numberPlusButton->setVisible(showNumberOptions);
        m_numberAutoIncrementCheck->setGeometry(10, 68, 100, 24);
        m_numberMinusButton->setGeometry(118, 66, 28, 28);
        m_numberLabel->setGeometry(152, 66, 48, 28);
        m_numberPlusButton->setGeometry(206, 66, 28, 28);
    }
}

QToolButton *CaptureOverlayWidget::currentActiveToolButton() const
{
    const QVector<QToolButton*> drawButtons {
        m_pencilButton, m_rectButton, m_circleButton, m_arrowButton, m_mosaicButton, m_blurButton, m_textButton, m_numberButton
    };
    for (QToolButton *button : drawButtons) {
        if (button && button->isChecked()) {
            return button;
        }
    }
    return nullptr;
}

void CaptureOverlayWidget::updateToolbarGeometry()
{
    const QRect selection = m_selection.rect();
    if (selection.isEmpty()) {
        m_toolbar->hide();
        if (m_toolOptions) {
            m_toolOptions->hide();
        }
        return;
    }

    const QSize hint = m_toolbar->sizeHint();
    const int x = qBound(8, selection.center().x() - hint.width() / 2, width() - hint.width() - 8);
    int y = selection.bottom() + 12;
    if (y + hint.height() > height() - 8) {
        y = qMax(8, selection.top() - hint.height() - 12);
    }
    m_toolbar->setGeometry(x, y, hint.width(), hint.height());
    updateToolOptionsGeometry();
}

void CaptureOverlayWidget::updateMagnifier(const QPoint &localPos)
{
    if (!m_magnifier || !m_screenCapture) {
        return;
    }

    const QPoint globalPos = localPos + m_virtualGeometry.topLeft();
    const QColor pixelColor = m_screenCapture->getPixelColor(globalPos.x(), globalPos.y());
    m_magnifier->updateView(localPos, globalPos, pixelColor, size());
}

void CaptureOverlayWidget::scheduleMagnifierUpdate(const QPoint &localPos)
{
    m_pendingMagnifierPos = localPos;
    m_hasPendingMagnifierPos = true;

    constexpr int kMagnifierFrameIntervalMs = 16;
    if (!m_magnifierFrameTimer.isValid() || m_magnifierFrameTimer.elapsed() >= kMagnifierFrameIntervalMs) {
        m_hasPendingMagnifierPos = false;
        updateMagnifier(localPos);
        m_magnifierFrameTimer.restart();
        return;
    }

    if (m_magnifierUpdateTimer && !m_magnifierUpdateTimer->isActive()) {
        const int delay = kMagnifierFrameIntervalMs - int(m_magnifierFrameTimer.elapsed());
        m_magnifierUpdateTimer->start(qMax(1, delay));
    }
}

QRect CaptureOverlayWidget::selectionVisualRect(const QRect &selection) const
{
    if (selection.isEmpty()) {
        return {};
    }

    return selection.adjusted(-140, -48, 140, 28).intersected(rect());
}

void CaptureOverlayWidget::updateSelectionVisualRegion(const QRect &before, const QRect &after)
{
    const QRect dirty = selectionVisualRect(before).united(selectionVisualRect(after));
    if (!dirty.isEmpty()) {
        update(dirty);
    }
}

void CaptureOverlayWidget::updateToolOptionsGeometry()
{
    if (!m_toolOptions || !m_toolOptions->isVisible()) {
        return;
    }
    m_toolOptions->setGeometry(toolOptionsRect());
    m_toolOptions->raise();
}

QRect CaptureOverlayWidget::toolOptionsRect() const
{
    if (!m_toolOptions || !m_toolbar) {
        return {};
    }

    const int width = 260;
    const Shapeype type = m_canvas ? m_canvas->activeShapeType() : PEN;
    int height = 88;
    if (type == NUMBER) {
        height = 116;
    } else if (type == MOSAIC || type == BLUR) {
        height = 48;
    }
    int x = m_toolbar->geometry().center().x() - width / 2;
    if (QToolButton *anchor = currentActiveToolButton()) {
        const QPoint anchorTopLeft = anchor->mapTo(this, QPoint(0, 0));
        x = anchorTopLeft.x() + anchor->width() / 2 - width / 2;
    }
    x = qBound(8, x, qMax(8, this->width() - width - 8));
    int y = m_toolbar->geometry().bottom() + 8;
    if (y + height > this->height() - 8) {
        y = qMax(8, m_toolbar->geometry().top() - height - 8);
    }
    return QRect(x, y, width, height);
}

void CaptureOverlayWidget::showTipBubble(QToolButton *button)
{
    if (!button || !m_tipBubble || !m_tipLabel || !button->isVisible()) {
        return;
    }

    const QString tipText = button->toolTip();
    if (tipText.isEmpty()) {
        hideTipBubble();
        return;
    }

    m_tipLabel->setText(tipText);
    m_tipBubble->adjustSize();

    const QPoint topLeft = button->mapTo(this, QPoint(0, 0));
    const QRect buttonRect(topLeft, button->size());
    const QPoint center = buttonRect.center();
    int x = center.x() - m_tipBubble->width() / 2;
    x = qBound(8, x, width() - m_tipBubble->width() - 8);

    int y = buttonRect.bottom() + 8;
    if (y + m_tipBubble->height() > height() - 8) {
        y = buttonRect.top() - m_tipBubble->height() - 8;
    }

    m_tipBubble->move(x, y);
    m_tipBubble->show();
    m_tipBubble->raise();
}

void CaptureOverlayWidget::hideTipBubble()
{
    if (m_tipBubble) {
        m_tipBubble->hide();
    }
}

void CaptureOverlayWidget::setDefaultAction(const QString &mode)
{
    if (mode == QStringLiteral("save")) {
        m_defaultAction = CaptureAction::Save;
    } else if (mode == QStringLiteral("sticky")) {
        m_defaultAction = CaptureAction::Sticky;
    } else if (mode == QStringLiteral("ocr")) {
        m_defaultAction = CaptureAction::Ocr;
    } else {
        m_defaultAction = CaptureAction::Copy;
    }
}

void CaptureOverlayWidget::performAction(CaptureAction action)
{
    const QRect globalRect = selectionInGlobal();
    if (globalRect.isEmpty() || !m_screenCapture) {
        return;
    }

    switch (action) {
    case CaptureAction::Copy:
        m_screenCapture->copyImageToClipboard(currentOutputImage());
        break;
    case CaptureAction::Save: {
        const QImage image = currentOutputImage();
        if (!image.isNull()) {
            const QString filePath = QFileDialog::getSaveFileName(this,
                                                                  tr("保存截图"),
                                                                  defaultSaveFilePath(),
                                                                  tr("PNG 图片 (*.png)"));
            if (!filePath.isEmpty()) {
                image.save(filePath);
            }
        }
        break;
    }
    case CaptureAction::Sticky: {
        const QImage image = currentOutputImage();
        if (!image.isNull() && m_stickyViewModel) {
            m_stickyViewModel->requestStickyImage(image, globalRect);
        }
        break;
    }
    case CaptureAction::Ocr:
        if (m_ocrViewModel) {
            const QImage image = m_screenCapture->captureRectToImage(globalRect);
            if (!image.isNull()) {
                m_ocrViewModel->recognize(image);
            }
        }
        break;
    case CaptureAction::LongCapture:
        if (m_widgetWindowBridge) {
            m_widgetWindowBridge->requestLongCapture(globalRect);
        }
        finishCapture();
        return;
    case CaptureAction::Gif:
        if (!m_gifRecordViewModel) {
            break;
        }
        m_gifGlobalCaptureRect = globalRect;
        m_gifRecordingMode = true;
        m_toolbar->hide();
        if (m_toolOptions) {
            m_toolOptions->hide();
        }
        if (m_canvas) {
            m_canvas->setDrawingEnabled(false);
        }
        hideTipBubble();
        if (m_magnifier) {
            m_magnifier->hide();
        }
        if (m_gifOverlay) {
            m_gifOverlay->setCaptureRect(globalRect);
            m_gifOverlay->setElapsedSecs(0);
            m_gifOverlay->setFrameCount(0);
            m_gifOverlay->setRecordingState(true, false);
        }
        hide();
        qApp->processEvents();
        m_gifRecordViewModel->startRecording(globalRect.x(), globalRect.y(), globalRect.width(), globalRect.height());
        return;
    }

    finishCapture();
}

void CaptureOverlayWidget::finishCapture()
{
    hide();
    hideTipBubble();
    if (m_screenCapture) {
        m_screenCapture->releaseDesktopSnapshot();
    }
    resetState();
}

QString CaptureOverlayWidget::defaultSaveFilePath() const
{
    QString baseDir;
    if (m_storageViewModel) {
        baseDir = m_storageViewModel->savePath().toLocalFile();
    }
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath();
    }
    const QString fileName = QStringLiteral("TZshot_%1.png")
                                 .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    return QDir(baseDir).filePath(fileName);
}


