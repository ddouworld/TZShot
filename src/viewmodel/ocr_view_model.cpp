#include "ocr_view_model.h"
#include "ocr/ocr_engine.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QClipboard>
#include <QGuiApplication>
#include <QDebug>

// ── 构造 ─────────────────────────────────────────────────
OcrViewModel::OcrViewModel(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFutureWatcher<QString>::finished,
            this,       &OcrViewModel::onRecognitionFinished);
}

// ── recognize ────────────────────────────────────────────
void OcrViewModel::recognize(const QImage &image)
{
    if (m_isRecognizing) {
        qWarning() << "[OcrViewModel] 正在识别中，忽略重复请求";
        return;
    }
    if (image.isNull()) {
        emit recognizeFailed("输入图像为空");
        return;
    }

    m_isRecognizing = true;
    emit isRecognizingChanged();

    // 在子线程中执行同步识别；OcrEngine 在 lambda 内部构造/销毁（线程安全）
    QImage imgCopy = image;   // 拷贝，避免主线程释放
    QFuture<QString> future = QtConcurrent::run([imgCopy]() -> QString {
        OcrEngine engine;
        if (!engine.isReady())
            return QString("__ERROR__:") + engine.lastError();
        QString result = engine.recognize(imgCopy);
        if (result.isEmpty() && !engine.lastError().isEmpty())
            return QString("__ERROR__:") + engine.lastError();
        return result;
    });

    m_watcher.setFuture(future);
}

// ── onRecognitionFinished ─────────────────────────────────
void OcrViewModel::onRecognitionFinished()
{
    m_isRecognizing = false;
    emit isRecognizingChanged();

    QString result = m_watcher.result();

    if (result.startsWith("__ERROR__:")) {
        QString err = result.mid(10);
        qWarning() << "[OcrViewModel] 识别失败：" << err;
        emit recognizeFailed(err);
        return;
    }

    m_resultText = result;
    emit resultTextChanged();
    emit resultReady(result);
}

// ── copyResultToClipboard ─────────────────────────────────
void OcrViewModel::copyResultToClipboard()
{
    if (m_resultText.isEmpty()) return;
    QClipboard *cb = QGuiApplication::clipboard();
    if (cb) {
        cb->setText(m_resultText, QClipboard::Clipboard);
#ifdef Q_OS_LINUX
        cb->setText(m_resultText, QClipboard::Selection);
#endif
    }
}
