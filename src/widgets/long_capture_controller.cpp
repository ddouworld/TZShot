#include "widgets/long_capture_controller.h"

#include "viewmodel/scroll_capture_view_model.h"

#include <QApplication>
#include <QBuffer>
#include <QEvent>
#include <QFile>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

QString loadLongCaptureStyleSheet()
{
    QFile file(QStringLiteral(":/resource/qss/long_capture_controller.qss"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

class LongCaptureFrameWidget : public QWidget
{
public:
    explicit LongCaptureFrameWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setFocusPolicy(Qt::NoFocus);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);
        QPen pen(QColor(QStringLiteral("#0EA5E9")));
        pen.setWidth(2);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
        painter.fillRect(QRect(0, 0, 6, 6), QColor(QStringLiteral("#0EA5E9")));
        painter.fillRect(QRect(width() - 6, 0, 6, 6), QColor(QStringLiteral("#0EA5E9")));
        painter.fillRect(QRect(0, height() - 6, 6, 6), QColor(QStringLiteral("#0EA5E9")));
        painter.fillRect(QRect(width() - 6, height() - 6, 6, 6), QColor(QStringLiteral("#0EA5E9")));
    }
};

void placeTopMost(QWidget *widget, const QRect &geometry)
{
    if (!widget) {
        return;
    }
    widget->setGeometry(geometry);
#ifdef Q_OS_WIN
    if (HWND hwnd = reinterpret_cast<HWND>(widget->winId())) {
        SetWindowPos(hwnd,
                     HWND_TOPMOST,
                     geometry.x(),
                     geometry.y(),
                     geometry.width(),
                     geometry.height(),
                     SWP_NOOWNERZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    }
#endif
}

QPixmap decodePreviewPixmap(const QString &previewUrl)
{
    if (!previewUrl.startsWith(QStringLiteral("data:image/png;base64,"))) {
        return {};
    }
    const QByteArray data = QByteArray::fromBase64(previewUrl.mid(22).toLatin1());
    QPixmap pixmap;
    pixmap.loadFromData(data, "PNG");
    return pixmap;
}

}

LongCaptureController::LongCaptureController(ScrollCaptureViewModel *scrollCaptureViewModel,
                                             QObject *parent)
    : QObject(parent)
    , m_scrollCaptureViewModel(scrollCaptureViewModel)
{
    if (!m_scrollCaptureViewModel) {
        return;
    }

    connect(m_scrollCaptureViewModel, &ScrollCaptureViewModel::captureStarted, this, [this]() {
        setRunning(true);
        updateStatusText(tr("滚动检测中，滑动内容会自动拼接"));
    });
    connect(m_scrollCaptureViewModel, &ScrollCaptureViewModel::captureSucceeded, this, [this](const QString &) {
        closeAll();
    });
    connect(m_scrollCaptureViewModel, &ScrollCaptureViewModel::captureFailed, this, [this](const QString &) {
        closeAll();
    });
    connect(m_scrollCaptureViewModel, &ScrollCaptureViewModel::capturedFramesChanged, this, [this]() {
        updateFrameCount(m_scrollCaptureViewModel->capturedFrames());
    });
    connect(m_scrollCaptureViewModel, &ScrollCaptureViewModel::statusTextChanged, this, [this]() {
        updateStatusText(m_scrollCaptureViewModel->statusText());
    });
    connect(m_scrollCaptureViewModel, &ScrollCaptureViewModel::previewImageUrlChanged, this, [this]() {
        updatePreviewImage(m_scrollCaptureViewModel->previewImageUrl());
    });
}

void LongCaptureController::beginCapture(const QRect &captureRect)
{
    m_captureRect = captureRect.normalized();
    if (m_captureRect.isEmpty()) {
        return;
    }

    ensureWidgets();
    setRunning(false);
    updateFrameCount(0);
    updateStatusText(tr("点击开始后，滑动内容自动拼接"));
    updatePreviewImage(QString());
    updateWidgetGeometry();

    if (m_barWidget) {
        m_barWidget->show();
        m_barWidget->raise();
    }
    if (m_previewWidget) {
        m_previewWidget->show();
        m_previewWidget->raise();
    }
    if (m_frameWidget) {
        m_frameWidget->show();
        m_frameWidget->raise();
    }
}

bool LongCaptureController::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            if (m_running && m_scrollCaptureViewModel) {
                m_scrollCaptureViewModel->stop();
            } else {
                closeAll();
            }
            return true;
        }
    }
    return QObject::eventFilter(watched, event);
}

void LongCaptureController::closeAll()
{
    setRunning(false);
    if (m_barWidget) {
        m_barWidget->hide();
    }
    if (m_previewWidget) {
        m_previewWidget->hide();
    }
    if (m_frameWidget) {
        m_frameWidget->hide();
    }
    m_captureRect = {};
}

void LongCaptureController::ensureWidgets()
{
    if (!m_barWidget) {
        m_barWidget = new QWidget(nullptr);
        m_barWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
        m_barWidget->setAttribute(Qt::WA_ShowWithoutActivating, true);
        m_barWidget->installEventFilter(this);
        m_barWidget->setObjectName(QStringLiteral("longCaptureBar"));
        m_barWidget->setStyleSheet(loadLongCaptureStyleSheet());
        m_barWidget->setFixedSize(500, 56);

        auto *layout = new QHBoxLayout(m_barWidget);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(10);

        auto *badge = new QLabel(QStringLiteral("长"), m_barWidget);
        badge->setObjectName(QStringLiteral("longCaptureBadge"));
        badge->setAlignment(Qt::AlignCenter);
        badge->setFixedSize(26, 26);
        layout->addWidget(badge);

        m_barMessageLabel = new QLabel(tr("点击开始后，滑动内容自动拼接"), m_barWidget);
        m_barMessageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        layout->addWidget(m_barMessageLabel, 1);

        m_barFrameLabel = new QLabel(tr("0 帧"), m_barWidget);
        m_barFrameLabel->setObjectName(QStringLiteral("longCaptureFrameLabel"));
        layout->addWidget(m_barFrameLabel);

        m_startButton = new QPushButton(tr("开始"), m_barWidget);
        m_startButton->setObjectName(QStringLiteral("longCapturePrimaryBtn"));
        m_startButton->setFixedSize(72, 30);
        layout->addWidget(m_startButton);

        m_stopButton = new QPushButton(tr("关闭"), m_barWidget);
        m_stopButton->setObjectName(QStringLiteral("longCaptureDangerBtn"));
        m_stopButton->setFixedSize(72, 30);
        layout->addWidget(m_stopButton);

        connect(m_startButton, &QPushButton::clicked, this, [this]() {
            if (m_captureRect.isEmpty() || !m_scrollCaptureViewModel) {
                return;
            }
            setRunning(true);
            updateStatusText(tr("滚动检测中，滑动内容会自动拼接"));
            m_scrollCaptureViewModel->start(m_captureRect);
        });
        connect(m_stopButton, &QPushButton::clicked, this, [this]() {
            if (m_running && m_scrollCaptureViewModel) {
                m_scrollCaptureViewModel->stop();
            } else {
                closeAll();
            }
        });
    }

    if (!m_previewWidget) {
        m_previewWidget = new QWidget(nullptr);
        m_previewWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
        m_previewWidget->setAttribute(Qt::WA_TranslucentBackground, true);
        m_previewWidget->setAttribute(Qt::WA_StyledBackground, true);
        m_previewWidget->setAttribute(Qt::WA_ShowWithoutActivating, true);
        m_previewWidget->installEventFilter(this);
        m_previewWidget->setObjectName(QStringLiteral("longCapturePreview"));
        m_previewWidget->setStyleSheet(loadLongCaptureStyleSheet());
        m_previewWidget->setFixedSize(260, 420);

        auto *layout = new QVBoxLayout(m_previewWidget);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(8);

        auto *headerLayout = new QHBoxLayout();
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setSpacing(8);
        auto *title = new QLabel(tr("长截图预览"), m_previewWidget);
        title->setObjectName(QStringLiteral("longCapturePreviewTitle"));
        headerLayout->addWidget(title);
        headerLayout->addStretch(1);
        m_previewFrameLabel = new QLabel(tr("0 帧"), m_previewWidget);
        m_previewFrameLabel->setObjectName(QStringLiteral("longCapturePreviewSub"));
        headerLayout->addWidget(m_previewFrameLabel);
        layout->addLayout(headerLayout);

        auto *imageBox = new QWidget(m_previewWidget);
        imageBox->setObjectName(QStringLiteral("longCapturePreviewImageBox"));
        imageBox->setFixedHeight(352);
        auto *imageLayout = new QVBoxLayout(imageBox);
        imageLayout->setContentsMargins(6, 6, 6, 6);
        imageLayout->setSpacing(0);
        m_previewImageLabel = new QLabel(imageBox);
        m_previewImageLabel->setObjectName(QStringLiteral("longCapturePreviewEmpty"));
        m_previewImageLabel->setAlignment(Qt::AlignCenter);
        m_previewImageLabel->setText(tr("暂无预览"));
        imageLayout->addWidget(m_previewImageLabel, 1);
        layout->addWidget(imageBox, 1);

        m_previewStatusLabel = new QLabel(tr("等待开始"), m_previewWidget);
        m_previewStatusLabel->setObjectName(QStringLiteral("longCapturePreviewStatus"));
        m_previewStatusLabel->setWordWrap(false);
        layout->addWidget(m_previewStatusLabel);
    }

    if (!m_frameWidget) {
        m_frameWidget = new LongCaptureFrameWidget(nullptr);
        m_frameWidget->setAttribute(Qt::WA_ShowWithoutActivating, true);
        m_frameWidget->installEventFilter(this);
    }
}

void LongCaptureController::updateWidgetGeometry()
{
    if (m_captureRect.isEmpty()) {
        return;
    }

    QScreen *screen = QGuiApplication::screenAt(m_captureRect.center());
    QRect workArea;
    if (screen) {
        workArea = screen->availableGeometry();
    } else if (QScreen *primary = QGuiApplication::primaryScreen()) {
        workArea = primary->availableGeometry();
    }
    if (workArea.isEmpty()) {
        workArea = QRect(0, 0, 1920, 1080);
    }

    if (m_barWidget) {
        const QSize size = m_barWidget->size();
        const int x = qBound(workArea.left(),
                             m_captureRect.x() + m_captureRect.width() - size.width(),
                             qMax(workArea.left(), workArea.right() - size.width() + 1));
        int y = m_captureRect.bottom() + 12;
        if (y + size.height() > workArea.bottom() + 1) {
            y = qMax(workArea.top(), m_captureRect.top() - size.height() - 12);
        }
        placeTopMost(m_barWidget, QRect(x, y, size.width(), size.height()));
    }

    if (m_previewWidget) {
        const QSize size = m_previewWidget->size();
        constexpr int kPreviewGap = 14;
        const int rightX = m_captureRect.right() + 1 + kPreviewGap;
        const int leftX = m_captureRect.left() - size.width() - kPreviewGap;

        int x = 0;
        if (rightX + size.width() <= workArea.right() + 1) {
            x = rightX;
        } else if (leftX >= workArea.left()) {
            x = leftX;
        } else {
            const int rightSpace = workArea.right() - m_captureRect.right();
            const int leftSpace = m_captureRect.left() - workArea.left();
            if (rightSpace >= leftSpace) {
                x = qMax(workArea.left(), workArea.right() - size.width() + 1);
            } else {
                x = workArea.left();
            }
        }

        const int maxY = qMax(workArea.top(), workArea.bottom() - size.height() + 1);
        const int y = qBound(workArea.top(),
                             m_captureRect.top(),
                             maxY);
        placeTopMost(m_previewWidget, QRect(x, y, size.width(), size.height()));
    }

    if (m_frameWidget) {
        // Keep the decorative frame fully outside the sampled area so
        // its corner markers never leak into the captured image.
        const QRect frameRect = m_captureRect.adjusted(-6, -6, 6, 6);
        placeTopMost(m_frameWidget, frameRect);
    }
}

void LongCaptureController::updateFrameCount(int value)
{
    if (m_barFrameLabel) {
        m_barFrameLabel->setText(tr("%1 帧").arg(value));
    }
    if (m_previewFrameLabel) {
        m_previewFrameLabel->setText(tr("%1 帧").arg(value));
    }
}

void LongCaptureController::updateStatusText(const QString &text)
{
    if (m_barMessageLabel) {
        m_barMessageLabel->setText(text);
    }
    if (m_previewStatusLabel) {
        m_previewStatusLabel->setText(text);
    }
}

void LongCaptureController::updatePreviewImage(const QString &previewUrl)
{
    if (!m_previewImageLabel) {
        return;
    }
    const QPixmap pixmap = decodePreviewPixmap(previewUrl);
    if (pixmap.isNull()) {
        m_previewImageLabel->setPixmap(QPixmap());
        m_previewImageLabel->setText(tr("暂无预览"));
        m_previewImageLabel->setObjectName(QStringLiteral("longCapturePreviewEmpty"));
        m_previewImageLabel->style()->unpolish(m_previewImageLabel);
        m_previewImageLabel->style()->polish(m_previewImageLabel);
        return;
    }

    m_previewImageLabel->setText(QString());
    m_previewImageLabel->setObjectName(QString());
    m_previewImageLabel->style()->unpolish(m_previewImageLabel);
    m_previewImageLabel->style()->polish(m_previewImageLabel);
    m_previewImageLabel->setPixmap(pixmap.scaled(236, 340, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void LongCaptureController::setRunning(bool running)
{
    m_running = running;
    if (m_startButton) {
        m_startButton->setVisible(!running);
    }
    if (m_stopButton) {
        m_stopButton->setText(running ? tr("完成") : tr("关闭"));
    }
}
