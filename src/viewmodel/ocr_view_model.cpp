#include "ocr_view_model.h"
#include "ocr/ocr_engine.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QClipboard>
#include <QGuiApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

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
        emit recognizeFailed(tr("输入图像为空"));
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

QString OcrViewModel::runSelfCheck()
{
    const QStringList available = OcrEngine::availableTessdataDirs();
    const QStringList candidates = OcrEngine::tessdataSearchCandidates();

    QString report;
    if (!available.isEmpty()) {
        report += tr("OCR 环境检查：通过\n");
        report += tr("可用 tessdata：\n");
        for (const QString &dir : available) {
            report += QStringLiteral(" - %1\n").arg(QDir::toNativeSeparators(dir));
        }
    } else {
        report += tr("OCR 环境检查：未通过（缺少 chi_sim/eng 语言包）\n");
        report += tr("建议把以下文件放到目录中：\n");
        report += QStringLiteral(" - chi_sim.traineddata\n");
        report += QStringLiteral(" - eng.traineddata\n");
        report += tr("候选 tessdata 目录：\n");
        for (const QString &dir : candidates) {
            report += QStringLiteral(" - %1\n").arg(QDir::toNativeSeparators(dir));
        }
    }

    OcrEngine engine;
    if (engine.isReady()) {
        report += tr("引擎初始化：成功");
    } else {
        report += tr("引擎初始化：失败\n");
        report += tr("错误信息：%1").arg(engine.lastError());
    }

    m_selfCheckText = report.trimmed();
    emit selfCheckTextChanged();
    return m_selfCheckText;
}

bool OcrViewModel::openTessdataFolder()
{
    QString dirPath = OcrEngine::suggestedTessdataDir();
    if (dirPath.isEmpty()) {
        return false;
    }

    QDir dir(dirPath);
    if (!dir.exists()) {
        if (!QDir().mkpath(dirPath)) {
            return false;
        }
    }
    return QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(dirPath).absoluteFilePath()));
}
