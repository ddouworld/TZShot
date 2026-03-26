#include "widgets/ocr_result_widget.h"

#include "viewmodel/ocr_view_model.h"

#include <QCloseEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScreen>
#include <QTextEdit>
#include <QVBoxLayout>

OcrResultWidget::OcrResultWidget(OcrViewModel *viewModel, QWidget *parent)
    : QWidget(parent)
    , m_viewModel(viewModel)
{
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(420, 320);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(0);

    m_card = new QWidget(this);
    m_card->setObjectName(QStringLiteral("ocrResultCard"));
    m_card->setStyleSheet(QStringLiteral(
        "#ocrResultCard {"
        "background:#FFFFFF;"
        "border:1px solid #D1D5DB;"
        "border-radius:12px;"
        "}"));
    root->addWidget(m_card);

    auto *cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    m_titleBar = new QWidget(m_card);
    m_titleBar->setObjectName(QStringLiteral("ocrResultTitleBar"));
    m_titleBar->setFixedHeight(40);
    m_titleBar->setStyleSheet(QStringLiteral(
        "#ocrResultTitleBar {"
        "background:#F8FAFC;"
        "border-top-left-radius:12px;"
        "border-top-right-radius:12px;"
        "border-bottom-left-radius:0px;"
        "border-bottom-right-radius:0px;"
        "}"));
    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(14, 0, 8, 0);
    titleLayout->setSpacing(8);

    auto *title = new QLabel(tr("OCR 识别结果"), m_titleBar);
    title->setStyleSheet(QStringLiteral("color:#1E293B; font-size:14px; font-weight:600;"));
    titleLayout->addWidget(title);
    titleLayout->addStretch();

    m_closeButton = new QPushButton(QStringLiteral("✕"), m_titleBar);
    m_closeButton->setFixedSize(26, 26);
    m_closeButton->setStyleSheet(QStringLiteral(
        "QPushButton { border:none; background:transparent; color:#94A3B8; border-radius:5px; }"
        "QPushButton:hover { background:#FEE2E2; color:#DC2626; }"));
    connect(m_closeButton, &QPushButton::clicked, this, &QWidget::close);
    titleLayout->addWidget(m_closeButton);
    cardLayout->addWidget(m_titleBar);

    auto *divider = new QWidget(m_card);
    divider->setFixedHeight(1);
    divider->setStyleSheet(QStringLiteral("background:#E2E8F0;"));
    cardLayout->addWidget(divider);

    auto *content = new QWidget(m_card);
    content->setObjectName(QStringLiteral("ocrResultContent"));
    content->setStyleSheet(QStringLiteral("#ocrResultContent { background:#FFFFFF; }"));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(8, 8, 8, 8);
    contentLayout->setSpacing(8);

    m_loadingPane = new QWidget(content);
    m_loadingPane->setStyleSheet(QStringLiteral("background:#F8FAFC; border-radius:6px;"));
    auto *loadingLayout = new QVBoxLayout(m_loadingPane);
    loadingLayout->setContentsMargins(0, 20, 0, 20);
    loadingLayout->setSpacing(12);
    loadingLayout->addStretch();
    auto *loadingText = new QLabel(tr("正在识别中…"), m_loadingPane);
    loadingText->setAlignment(Qt::AlignCenter);
    loadingText->setStyleSheet(QStringLiteral("color:#64748B; font-size:13px;"));
    loadingLayout->addWidget(loadingText);
    loadingLayout->addStretch();
    contentLayout->addWidget(m_loadingPane, 1);

    m_resultEdit = new QTextEdit(content);
    m_resultEdit->setStyleSheet(QStringLiteral(
        "background:transparent;"
        "border:none;"
        "color:#1E293B;"
        "font-size:14px;"));
    contentLayout->addWidget(m_resultEdit, 1);
    cardLayout->addWidget(content, 1);

    auto *bottomBar = new QWidget(m_card);
    bottomBar->setObjectName(QStringLiteral("ocrResultBottomBar"));
    bottomBar->setFixedHeight(44);
    bottomBar->setStyleSheet(QStringLiteral(
        "#ocrResultBottomBar {"
        "background:#F8FAFC;"
        "border-bottom-left-radius:12px;"
        "border-bottom-right-radius:12px;"
        "border-top-left-radius:0px;"
        "border-top-right-radius:0px;"
        "}"));
    auto *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->addStretch();

    auto *bottomDivider = new QWidget(bottomBar);
    bottomDivider->setFixedHeight(1);
    bottomDivider->setStyleSheet(QStringLiteral("background:#E2E8F0;"));
    bottomDivider->setGeometry(0, 0, width(), 1);

    m_copyButton = new QPushButton(tr("复制全部"), bottomBar);
    m_copyButton->setFixedSize(100, 30);
    m_copyButton->setStyleSheet(QStringLiteral(
        "QPushButton { background:#EFF6FF; border:1px solid #3B82F6; border-radius:6px; color:#3B82F6; }"
        "QPushButton:hover { background:#DBEAFE; }"));
    connect(m_copyButton, &QPushButton::clicked, this, [this]() {
        if (m_viewModel) {
            m_viewModel->copyResultToClipboard();
            showCopyFeedback();
        }
    });
    bottomLayout->addWidget(m_copyButton);

    auto *closeAction = new QPushButton(tr("关闭"), bottomBar);
    closeAction->setFixedSize(70, 30);
    closeAction->setStyleSheet(QStringLiteral(
        "QPushButton { background:transparent; border:1px solid #CBD5E1; border-radius:6px; color:#64748B; }"
        "QPushButton:hover { background:#F1F5F9; }"));
    connect(closeAction, &QPushButton::clicked, this, &QWidget::close);
    bottomLayout->addSpacing(10);
    bottomLayout->addWidget(closeAction);
    bottomLayout->addStretch();
    cardLayout->addWidget(bottomBar);

    m_copyFeedback = new QLabel(tr("已复制！"), m_card);
    m_copyFeedback->setAlignment(Qt::AlignCenter);
    m_copyFeedback->setFixedSize(100, 28);
    m_copyFeedback->setStyleSheet(QStringLiteral(
        "background:#22C55E;"
        "color:white;"
        "border-radius:6px;"));
    m_copyFeedback->hide();

    if (m_viewModel) {
        connect(m_viewModel, &OcrViewModel::isRecognizingChanged, this, [this]() {
            updateUiState();
        });
        connect(m_viewModel, &OcrViewModel::resultTextChanged, this, [this]() {
            if (m_viewModel) {
                m_resultEdit->setPlainText(m_viewModel->resultText());
            }
            updateUiState();
        });
    }

    m_resultEdit->setPlaceholderText(tr("识别结果将显示在这里…"));
    updateUiState();
}

void OcrResultWidget::showAndActivate()
{
    if (!isVisible()) {
        if (QScreen *screen = QGuiApplication::primaryScreen()) {
            const QRect available = screen->availableGeometry();
            move(available.center() - QPoint(width() / 2, height() / 2));
        }
        show();
    }
    raise();
    activateWindow();
}

void OcrResultWidget::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void OcrResultWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_titleBar->geometry().contains(event->position().toPoint())) {
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(event);
}

void OcrResultWidget::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && !m_dragOffset.isNull()) {
        move(event->globalPosition().toPoint() - m_dragOffset);
    }
    QWidget::mouseMoveEvent(event);
}

void OcrResultWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QWidget::keyPressEvent(event);
}

void OcrResultWidget::updateUiState()
{
    const bool recognizing = m_viewModel && m_viewModel->isRecognizing();
    m_loadingPane->setVisible(recognizing);
    m_resultEdit->setVisible(!recognizing);
    m_copyButton->setEnabled(!recognizing);
    if (m_viewModel) {
        m_resultEdit->setPlainText(m_viewModel->resultText());
    }
}

void OcrResultWidget::showCopyFeedback()
{
    m_copyFeedback->move((m_card->width() - m_copyFeedback->width()) / 2,
                         m_card->height() - 44 - m_copyFeedback->height() - 6);
    m_copyFeedback->show();
    m_copyFeedback->raise();

    auto *anim = new QPropertyAnimation(m_copyFeedback, "windowOpacity", m_copyFeedback);
    anim->setDuration(1200);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    connect(anim, &QPropertyAnimation::finished, this, [this, anim]() {
        m_copyFeedback->hide();
        m_copyFeedback->setWindowOpacity(1.0);
        anim->deleteLater();
    });
    anim->start();
}
