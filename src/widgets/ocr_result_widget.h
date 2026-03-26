#ifndef OCR_RESULT_WIDGET_H
#define OCR_RESULT_WIDGET_H

#include <QPointer>
#include <QWidget>

class QLabel;
class QTextEdit;
class QPushButton;
class OcrViewModel;

class OcrResultWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OcrResultWidget(OcrViewModel *viewModel, QWidget *parent = nullptr);

    void showAndActivate();

protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void updateUiState();
    void showCopyFeedback();

    QPointer<OcrViewModel> m_viewModel;
    QWidget *m_card = nullptr;
    QWidget *m_titleBar = nullptr;
    QWidget *m_loadingPane = nullptr;
    QTextEdit *m_resultEdit = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_copyFeedback = nullptr;
    QPushButton *m_copyButton = nullptr;
    QPushButton *m_closeButton = nullptr;
    QPoint m_dragOffset;
};

#endif // OCR_RESULT_WIDGET_H
