#ifndef OCR_VIEW_MODEL_H
#define OCR_VIEW_MODEL_H

#include <QObject>
#include <QImage>
#include <QFuture>
#include <QFutureWatcher>

// OcrViewModel
// ------------
// 暴露给 QML 的 OCR 状态机 ViewModel（QML 上下文名：O_OcrVM）。
// recognize() 在 QtConcurrent 子线程中调用 OcrEngine，不阻塞 UI 线程。

class OcrViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool    isRecognizing READ isRecognizing NOTIFY isRecognizingChanged)
    Q_PROPERTY(QString resultText    READ resultText    NOTIFY resultTextChanged)
    Q_PROPERTY(QString selfCheckText READ selfCheckText NOTIFY selfCheckTextChanged)

public:
    explicit OcrViewModel(QObject *parent = nullptr);
    ~OcrViewModel() override = default;

    bool    isRecognizing() const { return m_isRecognizing; }
    QString resultText()    const { return m_resultText;    }
    QString selfCheckText() const { return m_selfCheckText; }

    // 提交识别任务（供 QML 调用）
    Q_INVOKABLE void recognize(const QImage &image);

    // 将结果文本复制到系统剪贴板
    Q_INVOKABLE void copyResultToClipboard();

    // OCR 环境一键自检（返回文本可直接展示到 UI）
    Q_INVOKABLE QString runSelfCheck();

    // 打开 tessdata 目录（若目录不存在会尝试创建）
    Q_INVOKABLE bool openTessdataFolder();

signals:
    void isRecognizingChanged();
    void resultTextChanged();
    void selfCheckTextChanged();
    void resultReady(const QString &text);
    void recognizeFailed(const QString &errorMessage);

private slots:
    void onRecognitionFinished();

private:
    bool    m_isRecognizing = false;
    QString m_resultText;
    QString m_selfCheckText;

    QFutureWatcher<QString> m_watcher;
};

#endif // OCR_VIEW_MODEL_H
