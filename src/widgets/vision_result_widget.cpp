#include "widgets/vision_result_widget.h"

#include "viewmodel/vision_view_model.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScreen>
#include <QTextEdit>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QLineEdit>

VisionResultWidget::VisionResultWidget(VisionViewModel *viewModel, QWidget *parent)
    : QWidget(parent)
    , m_viewModel(viewModel)
{
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(760, 480);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(0);

    m_card = new QWidget(this);
    m_card->setObjectName(QStringLiteral("visionResultCard"));
    m_card->setStyleSheet(QStringLiteral(
        "#visionResultCard { background:#FFFFFF; border:1px solid #D1D5DB; border-radius:12px; }"
        "#visionResultTitleBar { background:#F8FAFC; border-top-left-radius:12px; border-top-right-radius:12px; }"
        "#visionPreviewBox { background:#F8FAFC; border:1px solid #E2E8F0; border-radius:10px; }"
    ));
    root->addWidget(m_card);

    auto *cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    m_titleBar = new QWidget(m_card);
    m_titleBar->setObjectName(QStringLiteral("visionResultTitleBar"));
    m_titleBar->setFixedHeight(40);
    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(14, 0, 8, 0);
    titleLayout->setSpacing(8);
    auto *title = new QLabel(tr("AI 视觉理解"), m_titleBar);
    title->setStyleSheet(QStringLiteral("color:#1E293B; font-size:14px; font-weight:600;"));
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    auto *closeButton = new QPushButton(QStringLiteral("✕"), m_titleBar);
    closeButton->setFixedSize(26, 26);
    closeButton->setStyleSheet(QStringLiteral(
        "QPushButton { border:none; background:transparent; color:#94A3B8; border-radius:5px; }"
        "QPushButton:hover { background:#FEE2E2; color:#DC2626; }"));
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
    titleLayout->addWidget(closeButton);
    cardLayout->addWidget(m_titleBar);

    auto *divider = new QWidget(m_card);
    divider->setFixedHeight(1);
    divider->setStyleSheet(QStringLiteral("background:#E2E8F0;"));
    cardLayout->addWidget(divider);

    auto *content = new QWidget(m_card);
    auto *contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(14, 14, 14, 14);
    contentLayout->setSpacing(14);

    auto *leftPane = new QWidget(content);
    auto *leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);

    auto *previewBox = new QWidget(leftPane);
    previewBox->setObjectName(QStringLiteral("visionPreviewBox"));
    previewBox->setMinimumSize(220, 280);
    auto *previewLayout = new QVBoxLayout(previewBox);
    previewLayout->setContentsMargins(10, 10, 10, 10);
    previewLayout->setSpacing(8);
    auto *previewTitle = new QLabel(tr("图片预览"), previewBox);
    previewTitle->setStyleSheet(QStringLiteral("color:#334155; font-size:12px; font-weight:600;"));
    previewLayout->addWidget(previewTitle);
    m_previewLabel = new QLabel(previewBox);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumSize(180, 220);
    m_previewLabel->setStyleSheet(QStringLiteral("background:#FFFFFF; border:1px solid #E2E8F0; border-radius:8px;"));
    previewLayout->addWidget(m_previewLabel, 1);
    leftLayout->addWidget(previewBox, 1);

    m_statusLabel = new QLabel(tr("输入提示词后点击发送。"), leftPane);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color:#64748B; font-size:12px;"));
    leftLayout->addWidget(m_statusLabel);

    contentLayout->addWidget(leftPane, 0);

    auto *rightPane = new QWidget(content);
    auto *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    auto *promptTitle = new QLabel(tr("提示词"), rightPane);
    promptTitle->setStyleSheet(QStringLiteral("color:#334155; font-size:12px; font-weight:600;"));
    rightLayout->addWidget(promptTitle);

    auto *promptRow = new QHBoxLayout;
    promptRow->setContentsMargins(0, 0, 0, 0);
    promptRow->setSpacing(8);
    m_promptEdit = new QLineEdit(rightPane);
    m_promptEdit->setPlaceholderText(tr("例如：请总结这张截图里的重点信息"));
    m_promptEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { background:#F8FAFC; border:1px solid #E2E8F0; border-radius:8px; padding:8px 10px; color:#1E293B; }"
        "QLineEdit:focus { border-color:#3B82F6; }"));
    connect(m_promptEdit, &QLineEdit::returnPressed, this, &VisionResultWidget::sendPrompt);
    promptRow->addWidget(m_promptEdit, 1);
    m_sendButton = new QPushButton(tr("发送"), rightPane);
    m_sendButton->setFixedSize(72, 34);
    m_sendButton->setStyleSheet(QStringLiteral(
        "QPushButton { background:#2563EB; border:none; border-radius:8px; color:white; font-weight:600; }"
        "QPushButton:hover { background:#1D4ED8; }"
        "QPushButton:disabled { background:#94A3B8; }"));
    connect(m_sendButton, &QPushButton::clicked, this, &VisionResultWidget::sendPrompt);
    promptRow->addWidget(m_sendButton);
    rightLayout->addLayout(promptRow);

    auto *resultTitle = new QLabel(tr("分析结果"), rightPane);
    resultTitle->setStyleSheet(QStringLiteral("color:#334155; font-size:12px; font-weight:600;"));
    rightLayout->addWidget(resultTitle);

    m_resultEdit = new QTextEdit(rightPane);
    m_resultEdit->setReadOnly(true);
    m_resultEdit->setPlaceholderText(tr("分析结果会显示在这里…"));
    m_resultEdit->setStyleSheet(QStringLiteral(
        "QTextEdit { background:#FFFFFF; border:1px solid #E2E8F0; border-radius:10px; padding:8px; color:#1E293B; font-size:14px; }"));
    rightLayout->addWidget(m_resultEdit, 1);

    auto *bottomRow = new QHBoxLayout;
    bottomRow->setContentsMargins(0, 0, 0, 0);
    bottomRow->addStretch();
    m_copyButton = new QPushButton(tr("复制结果"), rightPane);
    m_copyButton->setFixedSize(92, 32);
    m_copyButton->setStyleSheet(QStringLiteral(
        "QPushButton { background:#EFF6FF; border:1px solid #3B82F6; border-radius:8px; color:#2563EB; }"
        "QPushButton:hover { background:#DBEAFE; }"));
    connect(m_copyButton, &QPushButton::clicked, this, [this]() {
        if (!m_resultMarkdown.trimmed().isEmpty()) {
            qApp->clipboard()->setText(m_resultMarkdown);
            showCopyFeedback();
        }
    });
    bottomRow->addWidget(m_copyButton);
    rightLayout->addLayout(bottomRow);
    contentLayout->addWidget(rightPane, 1);

    cardLayout->addWidget(content, 1);

    m_copyFeedback = new QLabel(tr("已复制！"), m_card);
    m_copyFeedback->setAlignment(Qt::AlignCenter);
    m_copyFeedback->setFixedSize(100, 28);
    m_copyFeedback->setStyleSheet(QStringLiteral(
        "background:#22C55E; color:white; border-radius:6px;"));
    m_copyFeedback->hide();

    if (m_viewModel) {
        connect(m_viewModel, &VisionViewModel::analysisStarted, this,
                [this](const QString &imageUrl, const QString &prompt, const QImage &image) {
            if (imageUrl != m_imageUrl) {
                return;
            }
            m_image = image;
            m_promptEdit->setText(prompt);
            if (!image.isNull()) {
                m_previewLabel->setText(QString());
                m_previewLabel->setPixmap(QPixmap::fromImage(image).scaled(m_previewLabel->size(),
                                                                           Qt::KeepAspectRatio,
                                                                           Qt::SmoothTransformation));
            }
            m_resultMarkdown.clear();
            m_resultEdit->clear();
            setLoadingState(true, tr("正在分析图片内容…"));
        });
        connect(m_viewModel, &VisionViewModel::analysisDelta, this,
                [this](const QString &imageUrl, const QString &, const QString &delta) {
            if (imageUrl != m_imageUrl || delta.isEmpty()) {
                return;
            }
            m_resultMarkdown += delta;
            m_resultEdit->setPlainText(m_resultMarkdown);
            m_resultEdit->moveCursor(QTextCursor::End);
            m_statusLabel->setText(tr("正在生成回答…"));
            m_copyButton->setEnabled(!m_resultMarkdown.trimmed().isEmpty());
        });
        connect(m_viewModel, &VisionViewModel::analysisSucceeded, this,
                [this](const QString &imageUrl, const QString &, const QString &result, const QImage &image) {
            if (imageUrl != m_imageUrl) {
                return;
            }
            m_image = image;
            if (!result.trimmed().isEmpty()) {
                m_resultMarkdown = result;
            }
            m_resultEdit->setMarkdown(m_resultMarkdown);
            setLoadingState(false, tr("分析完成。"));
            m_copyButton->setEnabled(!m_resultMarkdown.trimmed().isEmpty());
            showAndActivate();
        });
        connect(m_viewModel, &VisionViewModel::analysisFailed, this,
                [this](const QString &imageUrl, const QString &, const QString &errorMsg) {
            if (imageUrl != m_imageUrl) {
                return;
            }
            setLoadingState(false, errorMsg);
            if (m_resultMarkdown.trimmed().isEmpty()) {
                m_resultEdit->setPlainText(errorMsg);
            }
            m_copyButton->setEnabled(!m_resultMarkdown.trimmed().isEmpty());
            showAndActivate();
        });
    }

    setLoadingState(false);
}

void VisionResultWidget::showForImage(const QString &imageUrl,
                                      const QImage &image,
                                      const QString &prompt)
{
    m_imageUrl = imageUrl;
    m_image = image;
    const QString initialPrompt = prompt.isEmpty()
        ? tr("请总结这张截图里的重点信息")
        : prompt;
    m_promptEdit->setText(initialPrompt);
    m_resultMarkdown.clear();
    m_resultEdit->clear();
    setLoadingState(false, tr("输入提示词后点击发送。"));
    if (!image.isNull()) {
        m_previewLabel->setText(QString());
        m_previewLabel->setPixmap(QPixmap::fromImage(image).scaled(m_previewLabel->size(),
                                                                   Qt::KeepAspectRatio,
                                                                   Qt::SmoothTransformation));
    } else {
        m_previewLabel->setPixmap(QPixmap());
        m_previewLabel->setText(tr("暂无预览"));
    }
    showAndActivate();
}

void VisionResultWidget::showAndActivate()
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

void VisionResultWidget::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void VisionResultWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_titleBar->geometry().contains(event->position().toPoint())) {
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(event);
}

void VisionResultWidget::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && !m_dragOffset.isNull()) {
        move(event->globalPosition().toPoint() - m_dragOffset);
    }
    QWidget::mouseMoveEvent(event);
}

void VisionResultWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QWidget::keyPressEvent(event);
}

void VisionResultWidget::sendPrompt()
{
    if (!m_viewModel) {
        setLoadingState(false, tr("视觉功能当前不可用。"));
        return;
    }
    if (m_imageUrl.isEmpty()) {
        setLoadingState(false, tr("当前没有可分析的图片。"));
        return;
    }
    const QString prompt = m_promptEdit->text().trimmed();
    if (prompt.isEmpty()) {
        setLoadingState(false, tr("提示词不能为空。"));
        return;
    }
    m_viewModel->analyzeStickyImage(prompt, m_imageUrl);
}

void VisionResultWidget::setLoadingState(bool loading, const QString &statusText)
{
    m_sendButton->setEnabled(!loading);
    m_copyButton->setEnabled(!loading && !m_resultMarkdown.trimmed().isEmpty());
    m_promptEdit->setEnabled(!loading);
    if (!statusText.isEmpty()) {
        m_statusLabel->setText(statusText);
    }
}

void VisionResultWidget::showCopyFeedback()
{
    m_copyFeedback->move(m_card->width() - m_copyFeedback->width() - 16,
                         m_card->height() - m_copyFeedback->height() - 16);
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
