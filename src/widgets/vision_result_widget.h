#ifndef VISION_RESULT_WIDGET_H
#define VISION_RESULT_WIDGET_H

#include <QPointer>
#include <QWidget>

class QLabel;
class QTextEdit;
class QPushButton;
class QLineEdit;
class VisionViewModel;

class VisionResultWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VisionResultWidget(VisionViewModel *viewModel, QWidget *parent = nullptr);

    void showForImage(const QString &imageUrl,
                      const QImage &image,
                      const QString &prompt = QString());
    void showAndActivate();

protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void sendPrompt();
    void setLoadingState(bool loading, const QString &statusText = QString());
    void showCopyFeedback();

    QPointer<VisionViewModel> m_viewModel;
    QString m_imageUrl;
    QImage m_image;
    QWidget *m_card = nullptr;
    QWidget *m_titleBar = nullptr;
    QLabel *m_previewLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLineEdit *m_promptEdit = nullptr;
    QTextEdit *m_resultEdit = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_copyButton = nullptr;
    QLabel *m_copyFeedback = nullptr;
    QPoint m_dragOffset;
    QString m_resultMarkdown;
};

#endif // VISION_RESULT_WIDGET_H
