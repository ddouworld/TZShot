#include "widgets/sticky_pin_widget.h"

#include "viewmodel/ai_view_model.h"
#include "viewmodel/vision_view_model.h"
#include "widgets/sticky_canvas_widget.h"
#include "widgets/vision_result_widget.h"
#include "sticky_image_store.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QFile>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QInputDialog>
#include <QPushButton>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QScreen>
#include <QSpinBox>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVector>
#include <QVBoxLayout>
#include <QWindow>
#include <QGuiApplication>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellscalingapi.h>
#endif

namespace {

QString loadStickyPinStyleSheet()
{
    QFile file(QStringLiteral(":/resource/qss/sticky_pin_widget.qss"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

qreal monitorScaleForPoint(const QPoint &physicalPoint)
{
#ifdef Q_OS_WIN
    const POINT nativePoint { physicalPoint.x(), physicalPoint.y() };
    if (HMONITOR monitor = MonitorFromPoint(nativePoint, MONITOR_DEFAULTTONEAREST)) {
        using GetDpiForMonitorFn = HRESULT (WINAPI *)(HMONITOR, MONITOR_DPI_TYPE, UINT *, UINT *);
        static const auto getDpiForMonitor = reinterpret_cast<GetDpiForMonitorFn>(
            GetProcAddress(LoadLibraryW(L"Shcore.dll"), "GetDpiForMonitor"));
        if (getDpiForMonitor) {
            UINT dpiX = 0;
            UINT dpiY = 0;
            if (SUCCEEDED(getDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) && dpiX > 0) {
                return qreal(dpiX) / 96.0;
            }
        }
    }
#endif
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        return screen->devicePixelRatio();
    }
    return 1.0;
}

QIcon makeAiLoadingIcon(int frame, const QSize &size)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPointF center(pixmap.width() / 2.0, pixmap.height() / 2.0);
    const qreal radius = qMin(pixmap.width(), pixmap.height()) / 2.0 - 2.0;
    const int spokeCount = 12;

    for (int i = 0; i < spokeCount; ++i) {
        const int index = (i + frame) % spokeCount;
        QColor color(QStringLiteral("#2563EB"));
        color.setAlphaF((index + 1.0) / spokeCount);

        painter.save();
        painter.translate(center);
        painter.rotate((360.0 / spokeCount) * i);
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawRoundedRect(QRectF(-1.5, -radius, 3.0, qMax<qreal>(4.0, radius * 0.45)), 1.5, 1.5);
        painter.restore();
    }

    return QIcon(pixmap);
}

class StickyAiDialog : public QDialog
{
public:
    explicit StickyAiDialog(const QImage &previewImage, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        setAttribute(Qt::WA_DeleteOnClose, false);
        setModal(true);
        setFixedSize(420, 380);
        setObjectName(QStringLiteral("stickyAiDialog"));
        setStyleSheet(QStringLiteral(
            "#stickyAiDialog { background:#FFFFFF; border:1px solid #E2E8F0; border-radius:10px; }"
            "#stickyAiTitle { color:#1E293B; font-size:14px; font-weight:600; }"
            "#stickyAiSubtle { color:#94A3B8; font-size:10px; }"
            "#stickyAiPreview { background:#F1F5F9; border:1px solid #E2E8F0; border-radius:6px; }"
            "#stickyAiInput { background:#F8FAFC; border:1px solid #E2E8F0; border-radius:8px; padding:8px; color:#1E293B; }"
            "#stickyAiCancel { background:#F8FAFC; border:1px solid #E2E8F0; border-radius:6px; color:#64748B; padding:6px 14px; }"
            "#stickyAiCancel:hover { background:#F1F5F9; }"
            "#stickyAiSend { background:#0088FF; border:none; border-radius:6px; color:white; padding:6px 14px; }"
            "#stickyAiSend:hover { background:#3399FF; }"
            "#stickyAiClose { background:transparent; border:none; color:#64748B; font-size:14px; }"
            "#stickyAiClose:hover { background:#FEE2E2; border-radius:6px; color:#B91C1C; }"));

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(12, 12, 12, 12);
        root->setSpacing(8);

        auto *titleBar = new QHBoxLayout;
        titleBar->setContentsMargins(0, 0, 0, 0);
        titleBar->setSpacing(8);

        auto *icon = new QLabel(this);
        icon->setPixmap(QIcon(QStringLiteral(":/resource/img/lc_chat.svg")).pixmap(20, 20));
        icon->setFixedSize(20, 20);
        titleBar->addWidget(icon);

        auto *title = new QLabel(tr("AI 图像编辑"), this);
        title->setObjectName(QStringLiteral("stickyAiTitle"));
        titleBar->addWidget(title);
        titleBar->addStretch();

        auto *closeButton = new QPushButton(QStringLiteral("×"), this);
        closeButton->setObjectName(QStringLiteral("stickyAiClose"));
        closeButton->setFixedSize(28, 28);
        connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
        titleBar->addWidget(closeButton);
        root->addLayout(titleBar);

        auto *previewBox = new QFrame(this);
        previewBox->setObjectName(QStringLiteral("stickyAiPreview"));
        previewBox->setFixedHeight(64);
        auto *previewLayout = new QHBoxLayout(previewBox);
        previewLayout->setContentsMargins(8, 8, 8, 8);
        previewLayout->setSpacing(10);

        auto *thumb = new QLabel(previewBox);
        thumb->setFixedSize(48, 48);
        thumb->setStyleSheet(QStringLiteral("background:#E2E8F0; border-radius:4px;"));
        if (!previewImage.isNull()) {
            thumb->setPixmap(QPixmap::fromImage(previewImage).scaled(48, 48, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
        previewLayout->addWidget(thumb);

        auto *metaCol = new QVBoxLayout;
        metaCol->setContentsMargins(0, 0, 0, 0);
        metaCol->setSpacing(3);
        auto *label1 = new QLabel(tr("当前贴图"), previewBox);
        label1->setStyleSheet(QStringLiteral("color:#94A3B8; font-size:11px;"));
        metaCol->addWidget(label1);
        auto *label2 = new QLabel(tr("发送后将对此图执行 AI 编辑"), previewBox);
        label2->setObjectName(QStringLiteral("stickyAiSubtle"));
        metaCol->addWidget(label2);
        metaCol->addStretch();
        previewLayout->addLayout(metaCol, 1);
        root->addWidget(previewBox);

        m_input = new QPlainTextEdit(this);
        m_input->setObjectName(QStringLiteral("stickyAiInput"));
        m_input->setPlaceholderText(tr("描述您想要的修改..."));
        root->addWidget(m_input, 1);

        auto *bottom = new QHBoxLayout;
        bottom->setContentsMargins(0, 0, 0, 0);
        bottom->addStretch();

        auto *cancelButton = new QPushButton(tr("取消"), this);
        cancelButton->setObjectName(QStringLiteral("stickyAiCancel"));
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        bottom->addWidget(cancelButton);

        auto *sendButton = new QPushButton(tr("发送"), this);
        sendButton->setObjectName(QStringLiteral("stickyAiSend"));
        connect(sendButton, &QPushButton::clicked, this, [this]() {
            if (!m_input->toPlainText().trimmed().isEmpty()) {
                accept();
            }
        });
        bottom->addWidget(sendButton);
        root->addLayout(bottom);
    }

    QString prompt() const
    {
        return m_input ? m_input->toPlainText().trimmed() : QString();
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        QDialog::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (event->buttons() & Qt::LeftButton) {
            move(event->globalPosition().toPoint() - m_dragOffset);
        }
        QDialog::mouseMoveEvent(event);
    }

private:
    QPlainTextEdit *m_input = nullptr;
    QPoint m_dragOffset;
};

}

StickyPinWidget::StickyPinWidget(const QString &imageUrl,
                                 const QRect &physicalRect,
                                 const QImage &image,
                                 StickyImageStore *store,
                                 AIViewModel *aiViewModel,
                                 VisionViewModel *visionViewModel,
                                 QWidget *parent)
    : QWidget(parent)
    , m_imageUrl(imageUrl)
    , m_physicalRect(physicalRect.normalized())
    , m_image(image)
    , m_store(store)
    , m_aiViewModel(aiViewModel)
    , m_visionViewModel(visionViewModel)
{
    m_annotationText = tr("文本");

    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setStyleSheet(loadStickyPinStyleSheet());

    const qreal dpr = screenScaleForRect(m_physicalRect);
    if (!m_image.isNull() && dpr > 0.0 && !qFuzzyCompare(m_image.devicePixelRatio(), dpr)) {
        m_image.setDevicePixelRatio(dpr);
    }

    m_imageDisplaySize = QSize(qMax(1, qRound(m_image.width() / qMax<qreal>(1.0, m_image.devicePixelRatio()))),
                               qMax(1, qRound(m_image.height() / qMax<qreal>(1.0, m_image.devicePixelRatio()))));
    m_baseImageDisplaySize = m_imageDisplaySize;

    m_canvas = new StickyCanvasWidget(this);
    m_canvas->setBackgroundImage(m_image);
    m_canvas->setViewScale(m_zoomFactor);
    m_canvas->setPenColor(m_currentPenColor);
    m_canvas->setPenSize(m_currentPenSize);
    m_canvas->setActiveShapeType(PEN);
    m_canvas->setAnnotationText(m_annotationText);
    m_canvas->setTextBackgroundEnabled(m_textBackgroundEnabled);
    m_canvas->setNumberAutoIncrement(m_numberAutoIncrement);
    m_canvas->setNumberValue(m_numberValue);
    connect(m_canvas, &StickyCanvasWidget::numberValueChanged, this, [this](int value) {
        m_numberValue = value;
        if (m_numberValueSpin && m_numberValueSpin->value() != value) {
            m_numberValueSpin->setValue(value);
        }
        if (m_numberLabel) {
            m_numberLabel->setText(QString::number(value));
        }
    });
    connect(m_canvas, &StickyCanvasWidget::textPlacementRequested, this, [this](const QPoint &point) {
        m_pendingTextPoint = point;
        if (!m_inlineTextEditor) {
            return;
        }
        m_inlineTextEditor->setText(m_annotationText);
        const QRect canvasRect = contentRect();
        const int editorW = 180;
        const int editorH = 32;
        int x = canvasRect.x() + point.x();
        int y = canvasRect.y() + point.y() + 6;
        x = qMax(canvasRect.x(), qMin(x, canvasRect.right() - editorW + 1));
        y = qMax(canvasRect.y(), qMin(y, canvasRect.bottom() - editorH + 1));
        m_inlineTextEditor->setGeometry(x, y, editorW, editorH);
        m_inlineTextEditor->show();
        m_inlineTextEditor->raise();
        m_inlineTextEditor->setFocus();
        m_inlineTextEditor->selectAll();
    });
    if (m_aiViewModel) {
        connect(m_aiViewModel, &AIViewModel::signalRequestComplete,
                this, &StickyPinWidget::applyAiImage);
        connect(m_aiViewModel, &AIViewModel::signalRequestFailed, this,
                [this](const QString &errorMsg) {
            this->setAiLoading(false);
            QMessageBox::warning(this, tr("AI 编辑失败"), errorMsg);
        });
        connect(m_aiViewModel, &AIViewModel::isLoadingChanged,
                this, &StickyPinWidget::setAiLoading);
    }
    if (m_visionViewModel) {
        m_visionResultWidget = new VisionResultWidget(m_visionViewModel, this);
    }

    m_toolOptions = new QWidget(this);
    m_toolOptions->setObjectName(QStringLiteral("stickyToolOptions"));

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
        connect(button, &QToolButton::clicked, this, [this, button, preset]() {
            for (QToolButton *other : m_sizeButtons) {
                if (other != button) {
                    other->setChecked(false);
                }
            }
            button->setChecked(true);
            m_currentPenSize = preset.value;
            syncCanvasToolSettings();
        });
        m_sizeButtons.append(button);
    }

    m_textInput = new QLineEdit(m_toolOptions);
    m_textInput->setPlaceholderText(tr("输入文字"));
    m_textInput->setText(m_annotationText);
    connect(m_textInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_annotationText = text;
        if (m_canvas) {
            m_canvas->setAnnotationText(m_annotationText);
        }
    });

    m_inlineTextEditor = new QLineEdit(this);
    m_inlineTextEditor->setObjectName(QStringLiteral("stickyInlineTextEditor"));
    m_inlineTextEditor->hide();
    m_inlineTextEditor->setPlaceholderText(tr("输入文字"));
    connect(m_inlineTextEditor, &QLineEdit::returnPressed, this, [this]() {
        const QString text = m_inlineTextEditor->text().trimmed();
        if (m_canvas && !text.isEmpty()) {
            m_annotationText = text;
            if (m_textInput) {
                m_textInput->setText(text);
            }
            m_canvas->setAnnotationText(text);
            m_canvas->addTextAnnotation(m_pendingTextPoint, text);
        }
        m_inlineTextEditor->hide();
    });
    connect(m_inlineTextEditor, &QLineEdit::editingFinished, this, [this]() {
        if (m_inlineTextEditor->isVisible()) {
            m_inlineTextEditor->hide();
        }
    });

    m_textBackgroundCheck = new QCheckBox(tr("背景"), m_toolOptions);
    m_textBackgroundCheck->setChecked(m_textBackgroundEnabled);
    connect(m_textBackgroundCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_textBackgroundEnabled = checked;
        if (m_canvas) {
            m_canvas->setTextBackgroundEnabled(checked);
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
        if (m_numberValueSpin) {
            m_numberValueSpin->setValue(nextValue);
        }
    });

    m_numberLabel = new QLabel(QString::number(m_numberValue), m_toolOptions);
    m_numberLabel->setObjectName(QStringLiteral("stickyNumberLabel"));
    m_numberLabel->setAlignment(Qt::AlignCenter);

    m_numberPlusButton = new QToolButton(m_toolOptions);
    m_numberPlusButton->setText(QStringLiteral("+"));
    m_numberPlusButton->setCursor(Qt::PointingHandCursor);
    connect(m_numberPlusButton, &QToolButton::clicked, this, [this]() {
        const int nextValue = qMin(9999, m_numberValue + 1);
        if (m_numberValueSpin) {
            m_numberValueSpin->setValue(nextValue);
        }
    });

    m_numberValueSpin = new QSpinBox(m_toolOptions);
    m_numberValueSpin->setRange(1, 9999);
    m_numberValueSpin->setValue(m_numberValue);
    connect(m_numberValueSpin, &QSpinBox::valueChanged, this, [this](int value) {
        m_numberValue = value;
        if (m_numberLabel) {
            m_numberLabel->setText(QString::number(value));
        }
        if (m_canvas) {
            m_canvas->setNumberValue(value);
        }
    });

    m_toolbar = new QWidget(this);
    m_toolbar->setObjectName(QStringLiteral("stickyToolbar"));

    m_pencilButton = new QToolButton(m_toolbar);
    m_rectButton = new QToolButton(m_toolbar);
    m_circleButton = new QToolButton(m_toolbar);
    m_arrowButton = new QToolButton(m_toolbar);
    m_highlightButton = new QToolButton(m_toolbar);
    m_mosaicButton = new QToolButton(m_toolbar);
    m_blurButton = new QToolButton(m_toolbar);
    m_textButton = new QToolButton(m_toolbar);
    m_numberButton = new QToolButton(m_toolbar);
    m_undoButton = new QToolButton(m_toolbar);
    m_aiButton = new QToolButton(m_toolbar);
    m_visionButton = new QToolButton(m_toolbar);
    m_copyButton = new QToolButton(m_toolbar);
    m_saveButton = new QToolButton(m_toolbar);
    m_zoomLabel = new QLabel(m_toolbar);
    m_toggleToolbarButton = new QToolButton(m_toolbar);
    m_closeButton = new QToolButton(m_toolbar);
    m_aiLoadingTimer = new QTimer(this);
    m_aiLoadingTimer->setInterval(90);
    connect(m_aiLoadingTimer, &QTimer::timeout, this, [this]() {
        m_aiLoadingFrame = (m_aiLoadingFrame + 1) % 12;
        updateAiLoadingIcon();
        update(contentRect());
    });

    auto setupButton = [](QToolButton *button, const QString &tooltip) {
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setAutoRaise(false);
        button->setIconSize(QSize(18, 18));
        button->setToolTip(tooltip);
        button->setCursor(Qt::PointingHandCursor);
        button->setContentsMargins(0, 0, 0, 0);
    };

    setupButton(m_pencilButton, tr("画笔"));
    setupButton(m_rectButton, tr("矩形"));
    setupButton(m_circleButton, tr("圆形"));
    setupButton(m_arrowButton, tr("箭头"));
    setupButton(m_highlightButton, tr("高亮"));
    setupButton(m_mosaicButton, tr("马赛克"));
    setupButton(m_blurButton, tr("高斯模糊"));
    setupButton(m_textButton, tr("文字"));
    setupButton(m_numberButton, tr("序号"));
    setupButton(m_undoButton, tr("撤销"));
    setupButton(m_aiButton, tr("AI 编辑"));
    setupButton(m_visionButton, tr("AI 理解"));
    setupButton(m_copyButton, tr("复制"));
    setupButton(m_saveButton, tr("保存"));
    setupButton(m_toggleToolbarButton, tr("隐藏工具栏"));
    setupButton(m_closeButton, tr("关闭"));

    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_zoomLabel->setMinimumWidth(52);
    m_zoomLabel->setText(QStringLiteral("100%"));
    m_zoomLabel->setStyleSheet(QStringLiteral(
        "color:#334155;"
        "background:transparent;"
        "border:none;"
        "font-size:12px;"
        "font-weight:500;"
    ));

    m_pencilButton->setCheckable(true);
    m_rectButton->setCheckable(true);
    m_circleButton->setCheckable(true);
    m_arrowButton->setCheckable(true);
    m_highlightButton->setCheckable(true);
    m_mosaicButton->setCheckable(true);
    m_blurButton->setCheckable(true);
    m_textButton->setCheckable(true);
    m_numberButton->setCheckable(true);
    m_pencilButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_pencil.svg")));
    m_rectButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_square.svg")));
    m_circleButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_circle.svg")));
    m_arrowButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_arrow.svg")));
    m_highlightButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_highlighter.svg")));
    m_mosaicButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_mosaic.svg")));
    m_blurButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_blur.svg")));
    m_textButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_text.svg")));
    m_numberButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_list_ordered.svg")));
    m_undoButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_undo.svg")));
    m_aiButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_chat.svg")));
    m_visionButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_ocr.svg")));
    QIcon copyIcon = QIcon::fromTheme(QStringLiteral("edit-copy"));
    if (copyIcon.isNull()) {
        copyIcon = style()->standardIcon(QStyle::SP_FileDialogDetailedView);
    }
    m_copyButton->setIcon(copyIcon);
    m_saveButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_save.svg")));
    m_toggleToolbarButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_close.svg")));
    m_closeButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_x.svg")));

    const QVector<QToolButton*> drawButtons {
        m_pencilButton, m_rectButton, m_circleButton, m_arrowButton,
        m_highlightButton, m_mosaicButton, m_blurButton, m_textButton, m_numberButton
    };
    auto setOnlyChecked = [drawButtons](QToolButton *activeButton) {
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
    connect(m_highlightButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_highlightButton); setActiveTool(HIGHLIGHT, m_highlightButton); });
    connect(m_mosaicButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_mosaicButton); setActiveTool(MOSAIC, m_mosaicButton); });
    connect(m_blurButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_blurButton); setActiveTool(BLUR, m_blurButton); });
    connect(m_textButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_textButton); setActiveTool(TEXT, m_textButton); });
    connect(m_numberButton, &QToolButton::clicked, this, [=]() { setOnlyChecked(m_numberButton); setActiveTool(NUMBER, m_numberButton); });
    connect(m_undoButton, &QToolButton::clicked, this, [this]() {
        if (m_canvas) {
            m_canvas->undo();
        }
    });
    connect(m_aiButton, &QToolButton::clicked, this, &StickyPinWidget::openAiEditor);
    connect(m_visionButton, &QToolButton::clicked, this, &StickyPinWidget::openVisionWindow);
    connect(m_copyButton, &QToolButton::clicked, this, [this]() {
        if (m_store) {
            m_store->copyImageToClipboard(m_imageUrl);
        }
    });
    connect(m_saveButton, &QToolButton::clicked, this, [this]() {
        saveCurrentImage();
    });
    connect(m_toggleToolbarButton, &QToolButton::clicked, this, [this]() {
        m_toolbarVisible = !m_toolbarVisible;
        updateLayoutAndSize();
        update();
    });
    connect(m_closeButton, &QToolButton::clicked, this, &QWidget::close);

    updateLayoutAndSize();
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
    show();
    raise();
    activateWindow();
    applyNativePosition(m_physicalRect);
    setAiLoading(m_aiViewModel && m_aiViewModel->isLoading());
}

StickyPinWidget::~StickyPinWidget()
{
    releaseStoredImage();
}

void StickyPinWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), Qt::transparent);

    const QRect imageRect = contentRect();
    if (m_shadowVisible) {
        for (int i = 10; i >= 1; --i) {
            const int alpha = 3 + i * 2;
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(47, 143, 255, alpha));
            painter.drawRoundedRect(imageRect.adjusted(-i, -i, i, i), 8, 8);
        }
    }

    painter.setPen(QPen(QColor(120, 182, 255, 80), 1));
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(imageRect, 4, 4);

    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(imageRect, 4, 4);

    if (m_aiLoading) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(15, 23, 42, 72));
        painter.drawRoundedRect(imageRect, 4, 4);

        const QSize overlaySize(156, 120);
        QRect overlayRect(QPoint(0, 0), overlaySize);
        overlayRect.moveCenter(imageRect.center());

        painter.setBrush(QColor(255, 255, 255, 238));
        painter.drawRoundedRect(overlayRect, 14, 14);

        painter.translate(overlayRect.center());
        const int spokeCount = 12;
        const qreal radius = 22.0;
        for (int i = 0; i < spokeCount; ++i) {
            const int index = (i + m_aiLoadingFrame) % spokeCount;
            QColor color(QStringLiteral("#2563EB"));
            color.setAlphaF((index + 1.0) / spokeCount);

            painter.save();
            painter.rotate((360.0 / spokeCount) * i);
            painter.setBrush(color);
            painter.drawRoundedRect(QRectF(-2.0, -radius, 4.0, 12.0), 2.0, 2.0);
            painter.restore();
        }
        painter.resetTransform();

        painter.setPen(QColor(QStringLiteral("#0F172A")));
        QFont labelFont = painter.font();
        labelFont.setPointSize(11);
        labelFont.setBold(true);
        painter.setFont(labelFont);
        painter.drawText(QRect(overlayRect.left() + 12,
                               overlayRect.bottom() - 44,
                               overlayRect.width() - 24,
                               20),
                         Qt::AlignHCenter | Qt::AlignVCenter,
                         tr("AI 正在处理中"));

        painter.setPen(QColor(QStringLiteral("#64748B")));
        QFont hintFont = painter.font();
        hintFont.setPointSize(9);
        hintFont.setBold(false);
        painter.setFont(hintFont);
        painter.drawText(QRect(overlayRect.left() + 12,
                               overlayRect.bottom() - 24,
                               overlayRect.width() - 24,
                               16),
                         Qt::AlignHCenter | Qt::AlignVCenter,
                         tr("请稍候…"));
        painter.restore();
    }
}

void StickyPinWidget::setActiveTool(Shapeype type, QToolButton *activeButton)
{
    const bool activate = activeButton && activeButton->isChecked();
    if (m_canvas) {
        m_canvas->setDrawingEnabled(activate);
        if (activate) {
            m_canvas->setActiveShapeType(type);
        }
    }
    syncCanvasToolSettings();
    updateToolOptionsPanel();
    updateLayoutAndSize();
    update();
}

void StickyPinWidget::syncCanvasToolSettings()
{
    if (!m_canvas) {
        return;
    }

    QColor penColor = m_currentPenColor;
    int penSize = m_currentPenSize;
    const Shapeype type = m_canvas->activeShapeType();
    if (type == HIGHLIGHT) {
        penColor.setAlpha(100);
        penSize = qMax(8, m_currentPenSize * 2);
    }

    m_canvas->setPenColor(penColor);
    m_canvas->setPenSize(penSize);
    m_canvas->setAnnotationText(m_annotationText);
    m_canvas->setTextBackgroundEnabled(m_textBackgroundEnabled);
    m_canvas->setNumberAutoIncrement(m_numberAutoIncrement);
    m_canvas->setNumberValue(m_numberValue);
}

void StickyPinWidget::updateToolOptionsPanel()
{
    const bool visible = m_toolbarVisible && m_canvas && m_canvas->drawingEnabled();
    if (!m_toolOptions) {
        return;
    }

    m_toolOptions->setVisible(visible);
    if (!visible) {
        return;
    }

    const Shapeype type = m_canvas->activeShapeType();
    const bool showTextOptions = (type == TEXT);
    const bool showNumberOptions = (type == NUMBER);
    const bool showColorOptions = (type != MOSAIC && type != BLUR);

    int x = 10;
    int y = 10;
    for (QToolButton *button : m_colorButtons) {
        button->setVisible(showColorOptions);
        button->setGeometry(x, y, 18, 18);
        x += 24;
    }

    y += 26;
    x = 10;
    for (QToolButton *button : m_sizeButtons) {
        button->setVisible(true);
        button->setGeometry(x, y, 48, 28);
        x += 54;
    }

    if (m_textInput && m_textBackgroundCheck) {
        m_textInput->setVisible(showTextOptions);
        m_textBackgroundCheck->setVisible(showTextOptions);
        m_textInput->setGeometry(10, 68, 160, 32);
        m_textBackgroundCheck->setGeometry(182, 72, 64, 24);
    }

    if (m_numberAutoIncrementCheck && m_numberMinusButton && m_numberLabel && m_numberPlusButton) {
        m_numberAutoIncrementCheck->setVisible(showNumberOptions);
        m_numberMinusButton->setVisible(showNumberOptions);
        m_numberLabel->setVisible(showNumberOptions);
        m_numberPlusButton->setVisible(showNumberOptions);
        m_numberValueSpin->setVisible(false);
        m_numberAutoIncrementCheck->setGeometry(10, 68, 100, 24);
        m_numberMinusButton->setGeometry(118, 66, 28, 28);
        m_numberLabel->setGeometry(152, 66, 48, 28);
        m_numberPlusButton->setGeometry(206, 66, 28, 28);
    }
}

void StickyPinWidget::closeEvent(QCloseEvent *event)
{
    releaseStoredImage();
    QWidget::closeEvent(event);
}

void StickyPinWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *copyAction = menu.addAction(tr("复制"));
    QAction *saveAction = menu.addAction(tr("保存原图"));
    QAction *aiAction = menu.addAction(tr("AI 编辑"));
    QAction *visionAction = menu.addAction(tr("AI 理解"));
    QAction *toggleAction = menu.addAction(m_toolbarVisible ? tr("隐藏工具栏") : tr("显示工具栏"));
    QAction *shadowAction = menu.addAction(m_shadowVisible ? tr("隐藏阴影") : tr("显示阴影"));
    menu.addSeparator();
    QAction *closeAction = menu.addAction(tr("关闭"));
    QAction *chosen = menu.exec(event->globalPos());
    if (chosen == copyAction) {
        if (m_store) {
            m_store->copyImageToClipboard(m_imageUrl);
        }
        return;
    }
    if (chosen == saveAction) {
        saveCurrentImage();
        return;
    }
    if (chosen == aiAction) {
        openAiEditor();
        return;
    }
    if (chosen == visionAction) {
        openVisionWindow();
        return;
    }
    if (chosen == toggleAction) {
        m_toolbarVisible = !m_toolbarVisible;
        updateLayoutAndSize();
        update();
        return;
    }
    if (chosen == shadowAction) {
        m_shadowVisible = !m_shadowVisible;
        update();
        return;
    }
    if (chosen == closeAction) {
        close();
    }
}

void StickyPinWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
#ifdef Q_OS_WIN
        if (HWND hwnd = reinterpret_cast<HWND>(winId())) {
            ReleaseCapture();
            SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            event->accept();
            return;
        }
#endif
    }
    QWidget::mousePressEvent(event);
}

void StickyPinWidget::wheelEvent(QWheelEvent *event)
{
    const QPoint angleDelta = event->angleDelta();
    if (angleDelta.y() == 0) {
        QWidget::wheelEvent(event);
        return;
    }

    const qreal step = angleDelta.y() > 0 ? 1.1 : 0.9;
    setZoomFactor(m_zoomFactor * step);
    event->accept();
}

void StickyPinWidget::setZoomFactor(qreal value)
{
    const qreal next = qBound<qreal>(0.3, value, 4.0);
    if (qFuzzyCompare(m_zoomFactor, next)) {
        return;
    }

    m_zoomFactor = next;
    m_imageDisplaySize = QSize(qMax(1, qRound(m_baseImageDisplaySize.width() * m_zoomFactor)),
                               qMax(1, qRound(m_baseImageDisplaySize.height() * m_zoomFactor)));
    if (m_canvas) {
        m_canvas->setViewScale(m_zoomFactor);
        m_canvas->setBackgroundImage(m_image);
    }
    updateLayoutAndSize();
    update();
}

void StickyPinWidget::openAiEditor()
{
    if (!m_aiViewModel) {
        QMessageBox::information(this, tr("AI 编辑"), tr("AI 功能当前不可用。"));
        return;
    }
    if (m_aiViewModel->isLoading()) {
        QMessageBox::information(this, tr("AI 编辑"), tr("当前有图像编辑任务正在处理中。"));
        return;
    }

    StickyAiDialog dialog(m_image, this);
    QPoint panelPos = frameGeometry().bottomLeft() + QPoint(0, 6);
    if (QScreen *screen = this->screen()) {
        const QRect available = screen->availableGeometry();
        if (panelPos.y() + dialog.height() > available.bottom() + 1) {
            panelPos.setY(frameGeometry().top() - dialog.height() - 6);
        }
        if (panelPos.x() + dialog.width() > available.right() + 1) {
            panelPos.setX(available.right() - dialog.width() + 1);
        }
        panelPos.setX(qMax(available.left(), panelPos.x()));
        panelPos.setY(qMax(available.top(), panelPos.y()));
    }
    dialog.move(panelPos);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString prompt = dialog.prompt();
    const QString trimmed = prompt.trimmed();
    if (trimmed.isEmpty()) {
        QMessageBox::information(this, tr("AI 编辑"), tr("提示词不能为空。"));
        return;
    }

    m_aiViewModel->sendPrompt(trimmed, m_imageUrl);
}

void StickyPinWidget::openVisionWindow()
{
    if (!m_visionViewModel || !m_visionResultWidget) {
        QMessageBox::information(this, tr("AI 理解"), tr("视觉理解功能当前不可用。"));
        return;
    }
    m_visionResultWidget->showForImage(m_imageUrl, m_image);
}

void StickyPinWidget::applyAiImage(const QString &oldImageUrl, const QString &newImageUrl)
{
    if (oldImageUrl != m_imageUrl || !m_store) {
        return;
    }

    const QImage newImage = m_store->getImageByUrl(newImageUrl);
    if (newImage.isNull()) {
        return;
    }

    const QString previousUrl = m_imageUrl;
    m_imageUrl = newImageUrl;
    m_image = newImage;
    m_baseImageDisplaySize = QSize(qMax(1, qRound(m_image.width() / qMax<qreal>(1.0, m_image.devicePixelRatio()))),
                                   qMax(1, qRound(m_image.height() / qMax<qreal>(1.0, m_image.devicePixelRatio()))));
    m_imageDisplaySize = QSize(qMax(1, qRound(m_baseImageDisplaySize.width() * m_zoomFactor)),
                               qMax(1, qRound(m_baseImageDisplaySize.height() * m_zoomFactor)));
    if (m_canvas) {
        m_canvas->setViewScale(m_zoomFactor);
        m_canvas->reset();
        m_canvas->setBackgroundImage(m_image);
        syncCanvasToolSettings();
    }
    updateLayoutAndSize();
    update();
    m_store->releaseImage(previousUrl);
    setAiLoading(false);
}

void StickyPinWidget::setAiLoading(bool loading)
{
    if (m_aiLoading == loading) {
        return;
    }

    m_aiLoading = loading;
    if (m_aiButton) {
        m_aiButton->setEnabled(!loading);
        m_aiButton->setToolTip(loading ? tr("AI 编辑处理中…") : tr("AI 编辑"));
    }

    if (loading) {
        m_aiLoadingFrame = 0;
        updateAiLoadingIcon();
        if (m_aiLoadingTimer) {
            m_aiLoadingTimer->start();
        }
        update(contentRect());
        return;
    }

    if (m_aiLoadingTimer) {
        m_aiLoadingTimer->stop();
    }
    if (m_aiButton) {
        m_aiButton->setIcon(QIcon(QStringLiteral(":/resource/img/lc_chat.svg")));
    }
    update(contentRect());
}

void StickyPinWidget::updateAiLoadingIcon()
{
    if (!m_aiButton || !m_aiLoading) {
        return;
    }
    m_aiButton->setIcon(makeAiLoadingIcon(m_aiLoadingFrame, m_aiButton->iconSize()));
}

void StickyPinWidget::releaseStoredImage()
{
    if (m_released || !m_store || m_imageUrl.isEmpty()) {
        return;
    }
    m_released = true;
    m_store->releaseImage(m_imageUrl);
}

bool StickyPinWidget::saveCurrentImage()
{
    if (m_image.isNull()) {
        return false;
    }

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("选择保存位置"),
        QString(),
        tr("PNG (*.png);;JPG (*.jpg);;BMP (*.bmp)"));

    if (filePath.isEmpty()) {
        return false;
    }

    return m_image.save(filePath);
}

qreal StickyPinWidget::screenScaleForRect(const QRect &physicalRect) const
{
    if (physicalRect.isEmpty()) {
        return 1.0;
    }
    return monitorScaleForPoint(physicalRect.center());
}

void StickyPinWidget::applyNativePosition(const QRect &physicalRect)
{
    const QPoint nativeTopLeft = physicalRect.topLeft() - contentRect().topLeft();
#ifdef Q_OS_WIN
    if (HWND hwnd = reinterpret_cast<HWND>(winId())) {
        SetWindowPos(hwnd,
                     HWND_TOPMOST,
                     nativeTopLeft.x(),
                     nativeTopLeft.y(),
                     0,
                     0,
                     SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        return;
    }
#endif
    move(nativeTopLeft);
}

void StickyPinWidget::updateLayoutAndSize()
{
    const QPoint contentTopLeft = currentContentTopLeftPhysical();
    const int totalWidth = qMax(m_imageDisplaySize.width(), toolbarWidth()) + shadowPadding() * 2;
    const int totalHeight = m_imageDisplaySize.height()
        + shadowPadding() * 2
        + (m_toolbarVisible ? (toolbarGap() + toolbarHeight()) : 0);
    const int optionsReserve = (m_toolOptions && m_toolOptions->isVisible())
        ? (toolOptionsGap() + toolOptionsHeight())
        : 0;
    const int finalHeight = totalHeight + optionsReserve;

    setFixedSize(totalWidth, finalHeight);

    if (m_toolbar) {
        const QRect barRect = toolbarRect();
        m_toolbar->setVisible(m_toolbarVisible);
        m_toolbar->setGeometry(barRect);
    }

    if (m_canvas) {
        m_canvas->setGeometry(contentRect());
    }

    if (m_toolOptions) {
        m_toolOptions->setGeometry(toolOptionsRect());
        updateToolOptionsPanel();
    }

    if (m_pencilButton && m_rectButton && m_circleButton && m_arrowButton
        && m_highlightButton && m_mosaicButton && m_blurButton
        && m_textButton && m_numberButton
        && m_undoButton && m_aiButton && m_visionButton && m_copyButton && m_saveButton && m_zoomLabel
        && m_toggleToolbarButton && m_closeButton) {
        const int buttonH = 32;
        const int buttonW = 32;
        const int zoomW = 52;
        const int buttonY = (toolbarHeight() - buttonH) / 2;
        const int gap = 2;
        int x = 6;
        m_pencilButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_rectButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_circleButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_arrowButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_highlightButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_mosaicButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_blurButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_textButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_numberButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_undoButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_aiButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_visionButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_copyButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_saveButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_zoomLabel->setGeometry(x, buttonY, zoomW, buttonH);
        m_zoomLabel->setText(QString::number(qRound(m_zoomFactor * 100)) + QStringLiteral("%"));
        x += zoomW + gap;
        m_toggleToolbarButton->setGeometry(x, buttonY, buttonW, buttonH);
        x += buttonW + gap;
        m_closeButton->setGeometry(x, buttonY, buttonW, buttonH);
        m_toggleToolbarButton->setToolTip(m_toolbarVisible ? tr("隐藏工具栏") : tr("显示工具栏"));
        const char *toggleIconPath = m_toolbarVisible
            ? ":/resource/img/lc_close.svg"
            : ":/resource/img/lc_check.svg";
        m_toggleToolbarButton->setIcon(QIcon(QString::fromLatin1(toggleIconPath)));
    }

    applyNativePosition(QRect(contentTopLeft, m_imageDisplaySize));
}

QPoint StickyPinWidget::currentContentTopLeftPhysical() const
{
#ifdef Q_OS_WIN
    if (HWND hwnd = reinterpret_cast<HWND>(const_cast<StickyPinWidget*>(this)->winId())) {
        RECT rc{};
        if (GetWindowRect(hwnd, &rc)) {
            return QPoint(rc.left, rc.top) + contentRect().topLeft();
        }
    }
#endif
    if (isVisible()) {
        return geometry().topLeft() + contentRect().topLeft();
    }
    return m_physicalRect.topLeft();
}

QRect StickyPinWidget::contentRect() const
{
    const int x = (width() - m_imageDisplaySize.width()) / 2;
    return QRect(x, shadowPadding(), m_imageDisplaySize.width(), m_imageDisplaySize.height());
}

QRect StickyPinWidget::toolOptionsRect() const
{
    const int w = toolOptionsWidth();
    int x = (width() - w) / 2;
    if (QToolButton *anchorButton = currentActiveToolButton()) {
        const int centerX = toolbarRect().x() + anchorButton->geometry().center().x();
        x = centerX - w / 2;
        x = qMax(shadowPadding(), qMin(x, width() - w - shadowPadding()));
    }
    const int y = toolbarRect().bottom() + 1 + toolOptionsGap();
    return QRect(x, y, w, toolOptionsHeight());
}

QRect StickyPinWidget::toolbarRect() const
{
    const int w = toolbarWidth();
    const int x = (width() - w) / 2;
    const int y = contentRect().bottom() + 1 + toolbarGap();
    return QRect(x, y, w, toolbarHeight());
}

int StickyPinWidget::shadowPadding() const
{
    return 12;
}

int StickyPinWidget::toolbarGap() const
{
    return 8;
}

int StickyPinWidget::toolOptionsGap() const
{
    return 8;
}

int StickyPinWidget::toolOptionsHeight() const
{
    if (m_canvas && m_canvas->drawingEnabled()) {
        const Shapeype type = m_canvas->activeShapeType();
        if (type == TEXT || type == NUMBER) {
            return 116;
        }
    }
    return 88;
}

int StickyPinWidget::toolOptionsWidth() const
{
    return 260;
}

QToolButton *StickyPinWidget::currentActiveToolButton() const
{
    const QVector<QToolButton*> drawButtons {
        m_pencilButton, m_rectButton, m_circleButton, m_arrowButton,
        m_highlightButton, m_mosaicButton, m_blurButton, m_textButton, m_numberButton
    };
    for (QToolButton *button : drawButtons) {
        if (button && button->isChecked()) {
            return button;
        }
    }
    return nullptr;
}

int StickyPinWidget::toolbarHeight() const
{
    return 44;
}

int StickyPinWidget::toolbarWidth() const
{
    return 32 * 16 + 52 + 2 * 16 + 12;
}
