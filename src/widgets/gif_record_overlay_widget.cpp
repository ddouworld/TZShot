#include "widgets/gif_record_overlay_widget.h"

#include <QColor>
#include <QGuiApplication>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QDebug>
#include <QScreen>
#include <QSize>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWindow>
#include <algorithm>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

constexpr auto kGifOverlayLogTag = "[GifOverlay v2]";
constexpr int kGifOverlayDisplayFps = 15;

struct ScreenSortLess
{
    bool operator()(const QRect &lhs, const QRect &rhs) const
    {
        if (lhs.left() != rhs.left()) {
            return lhs.left() < rhs.left();
        }
        return lhs.top() < rhs.top();
    }
};

struct ScreenEntry
{
    QScreen *screen = nullptr;
    QRect logicalGeometry;
    QRect physicalRect;
    qreal dpr = 1.0;
};

#ifdef Q_OS_WIN
QVector<QRect> enumerateMonitorRects()
{
    struct MonitorRects {
        QVector<QRect> rects;
    } monitors;

    EnumDisplayMonitors(nullptr,
                        nullptr,
                        [](HMONITOR monitor, HDC, LPRECT, LPARAM data) -> BOOL {
                            auto *rects = reinterpret_cast<MonitorRects *>(data);
                            MONITORINFOEXW info{};
                            info.cbSize = sizeof(info);
                            if (GetMonitorInfoW(monitor, &info)) {
                                const RECT &rc = info.rcMonitor;
                                rects->rects.append(QRect(rc.left,
                                                          rc.top,
                                                          rc.right - rc.left,
                                                          rc.bottom - rc.top));
                            }
                            return TRUE;
                        },
                        reinterpret_cast<LPARAM>(&monitors));

    std::sort(monitors.rects.begin(), monitors.rects.end(), ScreenSortLess{});
    return monitors.rects;
}
#endif

QVector<ScreenEntry> buildScreenEntries()
{
    QVector<ScreenEntry> entries;
    QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        return entries;
    }

    std::sort(screens.begin(), screens.end(), [](QScreen *lhs, QScreen *rhs) {
        if (!lhs || !rhs) {
            return lhs < rhs;
        }
        return ScreenSortLess{}(lhs->geometry(), rhs->geometry());
    });

#ifdef Q_OS_WIN
    const QVector<QRect> monitorRects = enumerateMonitorRects();
    const int pairCount = qMin(screens.size(), monitorRects.size());
    for (int i = 0; i < pairCount; ++i) {
        QScreen *screen = screens.at(i);
        if (!screen) {
            continue;
        }
        ScreenEntry entry;
        entry.screen = screen;
        entry.logicalGeometry = screen->geometry();
        entry.physicalRect = monitorRects.at(i);
        entry.dpr = screen->devicePixelRatio();
        if (entry.dpr <= 0.0) {
            entry.dpr = 1.0;
        }
        entries.append(entry);
    }

    for (int i = pairCount; i < screens.size(); ++i) {
        QScreen *screen = screens.at(i);
        if (!screen) {
            continue;
        }
        ScreenEntry entry;
        entry.screen = screen;
        entry.logicalGeometry = screen->geometry();
        entry.dpr = screen->devicePixelRatio();
        if (entry.dpr <= 0.0) {
            entry.dpr = 1.0;
        }
        entry.physicalRect = QRect(qRound(entry.logicalGeometry.x() * entry.dpr),
                                   qRound(entry.logicalGeometry.y() * entry.dpr),
                                   qRound(entry.logicalGeometry.width() * entry.dpr),
                                   qRound(entry.logicalGeometry.height() * entry.dpr));
        entries.append(entry);
    }
#else
    for (QScreen *screen : screens) {
        if (!screen) {
            continue;
        }
        ScreenEntry entry;
        entry.screen = screen;
        entry.logicalGeometry = screen->geometry();
        entry.physicalRect = entry.logicalGeometry;
        entry.dpr = screen->devicePixelRatio();
        if (entry.dpr <= 0.0) {
            entry.dpr = 1.0;
        }
        entries.append(entry);
    }
#endif

    return entries;
}

const ScreenEntry *screenEntryForPhysicalRect(const QRect &physicalRect, const QVector<ScreenEntry> &entries)
{
    if (entries.isEmpty()) {
        return nullptr;
    }

    const ScreenEntry *bestEntry = nullptr;
    int bestArea = -1;
    for (const ScreenEntry &entry : entries) {
        const QRect overlap = entry.physicalRect.intersected(physicalRect);
        const int area = overlap.width() * overlap.height();
        if (area > bestArea) {
            bestArea = area;
            bestEntry = &entry;
        }
    }

    if (bestEntry) {
        return bestEntry;
    }

    return &entries.first();
}

QRect mapPhysicalRectToLogical(const QRect &physicalRect, const ScreenEntry &entry)
{
    const QRect normalized = physicalRect.normalized();
    const QPoint localTopLeft = normalized.topLeft() - entry.physicalRect.topLeft();
    const QPoint logicalTopLeft(entry.logicalGeometry.x() + qRound(localTopLeft.x() / entry.dpr),
                                entry.logicalGeometry.y() + qRound(localTopLeft.y() / entry.dpr));
    return QRect(logicalTopLeft,
                 QSize(qMax(1, qRound(normalized.width() / entry.dpr)),
                       qMax(1, qRound(normalized.height() / entry.dpr))));
}

class GifBorderOverlayView : public QWidget
{
public:
    explicit GifBorderOverlayView(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);
        QPen pen(QColor(QStringLiteral("#EF4444")));
        pen.setWidth(2);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }
};

QToolButton *createOverlayButton(const QString &iconPath,
                                 const QString &toolTip,
                                 const QString &text,
                                 QWidget *parent)
{
    auto *button = new QToolButton(parent);
    button->setToolTip(toolTip);
    button->setAutoRaise(false);
    button->setText(text);
    if (!iconPath.isEmpty()) {
        button->setIcon(QIcon(iconPath));
        button->setIconSize(QSize(14, 14));
        if (text.trimmed().isEmpty()) {
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
            button->setFixedSize(30, 30);
        } else {
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            button->setFixedHeight(30);
            button->setMinimumWidth(78);
        }
    } else {
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setFixedHeight(30);
        button->setMinimumWidth(78);
    }
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

QRect layoutBoundsForRect(const QRect &globalRect)
{
    if (QScreen *screen = QGuiApplication::screenAt(globalRect.center())) {
        return screen->geometry();
    }
    return globalRect;
}

QScreen *targetScreenForRect(const QRect &globalRect)
{
    if (QScreen *screen = QGuiApplication::screenAt(globalRect.center())) {
        qDebug() << "[GifOverlay] screenAt center hit"
                 << "center=" << globalRect.center()
                 << "screenGeom=" << screen->geometry()
                 << "screenName=" << screen->name();
        return screen;
    }

    QScreen *bestScreen = nullptr;
    int bestArea = -1;
    for (QScreen *screen : QGuiApplication::screens()) {
        if (!screen) {
            continue;
        }
        const QRect overlap = screen->geometry().intersected(globalRect);
        const int area = overlap.width() * overlap.height();
        qDebug() << "[GifOverlay] fallback screen candidate"
                 << "screenGeom=" << screen->geometry()
                 << "screenName=" << screen->name()
                 << "overlap=" << overlap
                 << "area=" << area;
        if (area > bestArea) {
            bestArea = area;
            bestScreen = screen;
        }
    }
    if (bestScreen) {
        qDebug() << "[GifOverlay] fallback selected"
                 << "screenGeom=" << bestScreen->geometry()
                 << "screenName=" << bestScreen->name()
                 << "bestArea=" << bestArea;
    } else {
        qDebug() << "[GifOverlay] fallback failed, no screen selected";
    }
    return bestScreen;
}

}

GifRecordOverlayWidget::GifRecordOverlayWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("gifRecordOverlayHost"));
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    hide();

    m_borderOverlay = new GifBorderOverlayView(this);
    m_borderOverlay->hide();

    m_bar = new QWidget(this);
    m_bar->setObjectName(QStringLiteral("captureOverlayGifBar"));
    m_bar->setAttribute(Qt::WA_StyledBackground, true);
    m_bar->setStyleSheet(QStringLiteral(
        "#captureOverlayGifBar {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "                              stop:0 rgba(255,255,255,248),"
        "                              stop:1 rgba(248,250,252,244));"
        "  border: 1px solid rgba(148,163,184,72);"
        "  border-radius: 13px;"
        "}"
        "#captureOverlayGifInfoPanel {"
        "  background: rgba(15,23,42,10);"
        "  border: 1px solid rgba(148,163,184,58);"
        "  border-radius: 10px;"
        "}"
        "#captureOverlayGifTimeLabel {"
        "  color: #0F172A;"
        "  font-size: 15px;"
        "  font-weight: 700;"
        "  min-width: 52px;"
        "}"
        "#captureOverlayGifMetaLabel {"
        "  color: #64748B;"
        "  font-size: 10px;"
        "  font-weight: 600;"
        "  min-width: 82px;"
        "}"
        "#captureOverlayGifBar QToolButton {"
        "  background: rgba(255,255,255,224);"
        "  border: 1px solid rgba(203,213,225,230);"
        "  border-radius: 9px;"
        "  padding: 0px;"
        "  margin: 0px;"
        "  color: #0F172A;"
        "  font-size: 10px;"
        "  font-weight: 600;"
        "  min-width: 30px;"
        "  max-width: 30px;"
        "  min-height: 30px;"
        "  max-height: 30px;"
        "}"
        "#captureOverlayGifBar QToolButton:hover {"
        "  background: rgba(255,255,255,246);"
        "  border-color: rgba(148,163,184,255);"
        "}"
        "#captureOverlayGifBar QToolButton:pressed {"
        "  background: #E2E8F0;"
        "  border-color: #64748B;"
        "}"
        "#captureOverlayGifBar QToolButton:disabled {"
        "  background: rgba(226,232,240,210);"
        "  border-color: rgba(203,213,225,210);"
        "  color: #94A3B8;"
        "}"
        "#captureOverlayGifStopButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "                              stop:0 #FB7185,"
        "                              stop:1 #EF4444);"
        "  border: 1px solid #E11D48;"
        "  color: #FFFFFF;"
        "}"
        "#captureOverlayGifStopButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "                              stop:0 #F43F5E,"
        "                              stop:1 #DC2626);"
        "  border-color: #BE123C;"
        "}"
        "#captureOverlayGifStopButton:pressed {"
        "  background: #BE123C;"
        "  border-color: #9F1239;"
        "}"
        "#captureOverlayGifStopButton:disabled {"
        "  background: rgba(251,113,133,160);"
        "  border-color: rgba(225,29,72,120);"
        "  color: rgba(255,255,255,220);"
        "}"
        "#captureOverlayGifCancelButton {"
        "  background: rgba(255,255,255,232);"
        "  border-color: rgba(203,213,225,240);"
        "  color: #334155;"
        "}"
        "#captureOverlayGifCancelButton:hover {"
        "  background: #FFF1F2;"
        "  border-color: #FDA4AF;"
        "  color: #BE123C;"
        "}"
        "#captureOverlayGifCancelButton:pressed {"
        "  background: #FFE4E6;"
        "  border-color: #FB7185;"
        "  color: #9F1239;"
        "}"
    ));
    auto *shadow = new QGraphicsDropShadowEffect(m_bar);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(15, 23, 42, 42));
    m_bar->setGraphicsEffect(shadow);
    auto *gifLayout = new QHBoxLayout(m_bar);
    gifLayout->setContentsMargins(7, 7, 7, 7);
    gifLayout->setSpacing(7);
    gifLayout->setAlignment(Qt::AlignVCenter);

    m_infoPanel = new QWidget(m_bar);
    m_infoPanel->setObjectName(QStringLiteral("captureOverlayGifInfoPanel"));
    auto *infoLayout = new QHBoxLayout(m_infoPanel);
    infoLayout->setContentsMargins(9, 6, 10, 6);
    infoLayout->setSpacing(6);
    infoLayout->setAlignment(Qt::AlignVCenter);

    m_stateLabel = new QLabel(m_infoPanel);
    m_stateLabel->setAlignment(Qt::AlignCenter);
    m_stateLabel->setMinimumWidth(44);
    m_stateLabel->setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "                            stop:0 #FB7185,"
        "                            stop:1 #EF4444);"
        "color: #FFFFFF;"
        "font-size: 9px;"
        "font-weight: 700;"
        "padding: 3px 7px;"
        "border-radius: 8px;"));

    auto *textPanel = new QWidget(m_infoPanel);
    auto *textLayout = new QVBoxLayout(textPanel);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(1);

    m_timeLabel = new QLabel(textPanel);
    m_timeLabel->setObjectName(QStringLiteral("captureOverlayGifTimeLabel"));
    m_timeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_fpsLabel = new QLabel(textPanel);
    m_fpsLabel->setObjectName(QStringLiteral("captureOverlayGifMetaLabel"));
    m_fpsLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    textLayout->addWidget(m_timeLabel);
    textLayout->addWidget(m_fpsLabel);

    infoLayout->addWidget(m_stateLabel, 0, Qt::AlignVCenter);
    infoLayout->addWidget(textPanel, 1);

    m_stopButton = createOverlayButton(QStringLiteral(":/resource/img/lc_check.svg"),
                                       tr("完成录制"),
                                       QString(),
                                       m_bar);
    m_cancelButton = createOverlayButton(QStringLiteral(":/resource/img/lc_x.svg"),
                                         tr("取消录制"),
                                         QString(),
                                         m_bar);
    m_stopButton->setObjectName(QStringLiteral("captureOverlayGifStopButton"));
    m_cancelButton->setObjectName(QStringLiteral("captureOverlayGifCancelButton"));
    gifLayout->addWidget(m_infoPanel, 0, Qt::AlignVCenter);
    gifLayout->addStretch(1);
    gifLayout->addWidget(m_stopButton, 0, Qt::AlignVCenter);
    gifLayout->addWidget(m_cancelButton, 0, Qt::AlignVCenter);

    updateStatusText();
    m_bar->adjustSize();
    m_bar->setFixedSize(m_bar->sizeHint().expandedTo(QSize(286, 50)));

    connect(m_stopButton, &QToolButton::clicked, this, &GifRecordOverlayWidget::stopRequested);
    connect(m_cancelButton, &QToolButton::clicked, this, &GifRecordOverlayWidget::cancelRequested);
}

void GifRecordOverlayWidget::setCaptureRect(const QRect &globalRect)
{
    qDebug() << kGifOverlayLogTag << "setCaptureRect" << globalRect;
    if (m_captureRect == globalRect) {
        return;
    }
    m_captureRect = globalRect;
    updateLayout();
}

void GifRecordOverlayWidget::setRecordingState(bool recording, bool encoding)
{
    qDebug() << kGifOverlayLogTag << "setRecordingState"
             << "recording=" << recording
             << "encoding=" << encoding;
    if (m_recording == recording && m_encoding == encoding) {
        return;
    }
    m_recording = recording;
    m_encoding = encoding;
    updatePresentation();
}

void GifRecordOverlayWidget::setElapsedSecs(int elapsedSecs)
{
    if (m_elapsedSecs == elapsedSecs) {
        return;
    }
    m_elapsedSecs = elapsedSecs;
    updateStatusText();
}

void GifRecordOverlayWidget::setFrameCount(int frameCount)
{
    if (m_frameCount == frameCount) {
        return;
    }
    m_frameCount = frameCount;
    updateStatusText();
}

void GifRecordOverlayWidget::resetOverlay()
{
    qDebug() << kGifOverlayLogTag << "resetOverlay";
    m_captureRect = {};
    m_elapsedSecs = 0;
    m_frameCount = 0;
    m_recording = false;
    m_encoding = false;
    if (m_bar) {
        m_bar->hide();
    }
    if (m_borderOverlay) {
        m_borderOverlay->hide();
    }
    hide();
}

void GifRecordOverlayWidget::updateLayout()
{
    qDebug() << kGifOverlayLogTag << "updateLayout start"
             << "captureRect=" << m_captureRect
             << "recording=" << m_recording
             << "encoding=" << m_encoding;
    if (m_captureRect.isEmpty()) {
        if (m_bar) m_bar->hide();
        if (m_borderOverlay) m_borderOverlay->hide();
        hide();
        qDebug() << kGifOverlayLogTag << "updateLayout abort: empty capture rect";
        return;
    }

    const QVector<ScreenEntry> entries = buildScreenEntries();
    const ScreenEntry *entry = screenEntryForPhysicalRect(m_captureRect, entries);
    if (!entry || !entry->screen) {
        qDebug() << kGifOverlayLogTag << "updateLayout abort: no screen entry for physical rect" << m_captureRect;
        return;
    }

    const QRect logicalCaptureRect = mapPhysicalRectToLogical(m_captureRect, *entry);
    const QRect borderRect = logicalCaptureRect.adjusted(-3, -3, 3, 3);
    m_bar->adjustSize();
    const QSize hint = m_bar->sizeHint();
    const QRect screenRect = entry->screen->geometry();
    const QRect bounds = screenRect.adjusted(8, 8, -8, -8);
    const int preferredX = borderRect.x() + borderRect.width() - hint.width();
    const int x = qBound(bounds.left(),
                         preferredX,
                         qMax(bounds.left(), bounds.right() - hint.width() + 1));
    int y = borderRect.bottom() + 12;
    if (y + hint.height() > bounds.bottom() + 1) {
        y = qMax(bounds.top(), borderRect.top() - hint.height() - 12);
    }

    const QRect barRect(x, y, hint.width(), hint.height());
    qDebug() << kGifOverlayLogTag << "computed rects"
             << "screenPhysical=" << entry->physicalRect
             << "screenLogical=" << entry->logicalGeometry
             << "screenName=" << entry->screen->name()
             << "captureRect(physical)=" << m_captureRect
             << "captureRect(logical)=" << logicalCaptureRect
             << "borderRect=" << borderRect
             << "barHint=" << hint
             << "screenRect=" << screenRect
             << "bounds=" << bounds
             << "barRect=" << barRect;
    if (!isVisible()) {
        show();
        winId();
        qDebug() << kGifOverlayLogTag << "host shown first time"
                 << "hostVisible=" << isVisible()
                 << "winId=" << winId();
    }
    if (QWindow *handle = windowHandle()) {
        handle->setScreen(entry->screen);
        qDebug() << kGifOverlayLogTag << "host setScreen"
                 << "screenName=" << entry->screen->name()
                 << "screenGeom=" << entry->screen->geometry();
    }

    setGeometry(screenRect);
#ifdef Q_OS_WIN
    if (HWND hwnd = reinterpret_cast<HWND>(winId())) {
        SetWindowPos(hwnd,
                     HWND_TOPMOST,
                     entry->physicalRect.x(),
                     entry->physicalRect.y(),
                     entry->physicalRect.width(),
                     entry->physicalRect.height(),
                     SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

        RECT nativeRect{};
        if (GetWindowRect(hwnd, &nativeRect)) {
            qDebug() << kGifOverlayLogTag << "host native rect"
                     << QRect(nativeRect.left,
                              nativeRect.top,
                              nativeRect.right - nativeRect.left,
                              nativeRect.bottom - nativeRect.top);
        }
    }
#endif
    m_bar->setGeometry(barRect.translated(-screenRect.topLeft()));
    m_borderOverlay->setGeometry(borderRect.translated(-screenRect.topLeft()));
    qDebug() << kGifOverlayLogTag << "applied geometry"
             << "hostGeom=" << geometry()
             << "barGeom(local)=" << m_bar->geometry()
             << "borderGeom(local)=" << m_borderOverlay->geometry();

    if (!m_bar->isVisible()) m_bar->show();
    m_bar->raise();
    if (m_borderOverlay->isVisible()) m_borderOverlay->raise();
    qDebug() << kGifOverlayLogTag << "updateLayout end"
             << "hostVisible=" << isVisible()
             << "barVisible=" << m_bar->isVisible()
             << "borderVisible=" << m_borderOverlay->isVisible();
}

void GifRecordOverlayWidget::updateStatusText()
{
    if (!m_stateLabel || !m_timeLabel || !m_fpsLabel) {
        return;
    }

    if (m_encoding) {
        m_stateLabel->setText(tr("导出中"));
        m_stateLabel->setStyleSheet(QStringLiteral(
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            "                            stop:0 #F59E0B,"
            "                            stop:1 #D97706);"
            "color: #FFFFFF;"
            "font-size: 11px;"
            "font-weight: 700;"
            "padding: 5px 10px;"
            "border-radius: 10px;"));
        m_timeLabel->setText(tr("正在生成 GIF"));
        m_fpsLabel->setText(tr("已捕获 %1 帧").arg(qMax(0, m_frameCount)));
        return;
    }

    if (m_recording) {
        m_stateLabel->setText(tr("REC"));
        m_stateLabel->setStyleSheet(QStringLiteral(
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            "                            stop:0 #FB7185,"
            "                            stop:1 #EF4444);"
            "color: #FFFFFF;"
            "font-size: 11px;"
            "font-weight: 700;"
            "padding: 5px 10px;"
            "border-radius: 10px;"));
    } else {
        m_stateLabel->setText(tr("GIF"));
        m_stateLabel->setStyleSheet(QStringLiteral(
            "background: rgba(148,163,184,40);"
            "color: #475569;"
            "font-size: 11px;"
            "font-weight: 700;"
            "padding: 5px 10px;"
            "border-radius: 10px;"
            "border: 1px solid rgba(148,163,184,64);"));
    }

    const int totalSecs = qMax(0, m_elapsedSecs);
    const int hours = totalSecs / 3600;
    const int minutes = (totalSecs % 3600) / 60;
    const int seconds = totalSecs % 60;
    if (hours > 0) {
        m_timeLabel->setText(QStringLiteral("%1:%2:%3")
                                 .arg(hours, 2, 10, QLatin1Char('0'))
                                 .arg(minutes, 2, 10, QLatin1Char('0'))
                                 .arg(seconds, 2, 10, QLatin1Char('0')));
    } else {
        m_timeLabel->setText(QStringLiteral("%1:%2")
                                 .arg(minutes, 2, 10, QLatin1Char('0'))
                                 .arg(seconds, 2, 10, QLatin1Char('0')));
    }
    m_fpsLabel->setText(QStringLiteral("%1 FPS  |  %2 %3")
                            .arg(kGifOverlayDisplayFps)
                            .arg(qMax(0, m_frameCount))
                            .arg(tr("帧")));
}

void GifRecordOverlayWidget::updatePresentation()
{
    if (!m_stopButton || !m_cancelButton || !m_borderOverlay) {
        return;
    }

    if (m_encoding) {
        m_borderOverlay->hide();
        m_stopButton->setEnabled(false);
        m_cancelButton->setEnabled(false);
    } else if (m_recording) {
        m_borderOverlay->show();
        m_stopButton->setEnabled(true);
        m_cancelButton->setEnabled(true);
    } else {
        m_borderOverlay->hide();
        m_stopButton->setEnabled(true);
        m_cancelButton->setEnabled(true);
    }

    qDebug() << kGifOverlayLogTag << "updatePresentation"
             << "recording=" << m_recording
             << "encoding=" << m_encoding
             << "barVisible=" << (m_bar ? m_bar->isVisible() : false)
             << "borderVisible=" << m_borderOverlay->isVisible();

    updateStatusText();
    if (!m_captureRect.isEmpty()) {
        updateLayout();
    }
}
