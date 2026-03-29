#include "ocr_engine.h"

#ifdef ENABLE_OCR
#include <tesseract/baseapi.h>
#endif

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryDir>

namespace {
bool hasRequiredLanguages(const QString &tessdataDir)
{
    return QFileInfo::exists(tessdataDir + "/eng.traineddata")
           && QFileInfo::exists(tessdataDir + "/chi_sim.traineddata");
}

QStringList resolveTessdataDirs()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
#ifdef OCR_TESSDATA_DIR
        QStringLiteral(OCR_TESSDATA_DIR),
#endif
        appDir + "/thirdpart/ocr-install/share/tessdata",
        appDir + "/../thirdpart/ocr-install/share/tessdata",
        appDir + "/../../thirdpart/ocr-install/share/tessdata",
        appDir + "/thirdpart/ocr-win/share/tessdata",
        appDir + "/../thirdpart/ocr-win/share/tessdata",
        appDir + "/../../thirdpart/ocr-win/share/tessdata",
        appDir + "/thirdpart/ocr/share/tessdata",
        appDir + "/../thirdpart/ocr/share/tessdata",
        appDir + "/../../thirdpart/ocr/share/tessdata",
        QStringLiteral("/usr/share/tesseract-ocr/5/tessdata"),
        QStringLiteral("/usr/share/tesseract-ocr/4.00/tessdata")
    };

    const QByteArray envPrefix = qgetenv("TESSDATA_PREFIX");
    if (!envPrefix.isEmpty()) {
        const QString envPath = QString::fromUtf8(envPrefix);
        if (QFileInfo(envPath).fileName() == QStringLiteral("tessdata")) {
            candidates.prepend(envPath);
        } else {
            candidates.prepend(envPath + "/tessdata");
        }
    }

    QStringList validDirs;
    for (const QString &tessdataDir : candidates) {
        const QString normalized = QDir(tessdataDir).absolutePath();
        if (!validDirs.contains(normalized) && hasRequiredLanguages(normalized)) {
            validDirs << normalized;
        }
    }
    return validDirs;
}

QStringList resolveTessdataCandidates()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
#ifdef OCR_TESSDATA_DIR
        QStringLiteral(OCR_TESSDATA_DIR),
#endif
        appDir + "/thirdpart/ocr-install/share/tessdata",
        appDir + "/../thirdpart/ocr-install/share/tessdata",
        appDir + "/../../thirdpart/ocr-install/share/tessdata",
        appDir + "/thirdpart/ocr-win/share/tessdata",
        appDir + "/../thirdpart/ocr-win/share/tessdata",
        appDir + "/../../thirdpart/ocr-win/share/tessdata",
        appDir + "/thirdpart/ocr/share/tessdata",
        appDir + "/../thirdpart/ocr/share/tessdata",
        appDir + "/../../thirdpart/ocr/share/tessdata",
        QStringLiteral("/usr/share/tesseract-ocr/5/tessdata"),
        QStringLiteral("/usr/share/tesseract-ocr/4.00/tessdata")
    };

    const QByteArray envPrefix = qgetenv("TESSDATA_PREFIX");
    if (!envPrefix.isEmpty()) {
        const QString envPath = QString::fromUtf8(envPrefix);
        if (QFileInfo(envPath).fileName() == QStringLiteral("tessdata")) {
            candidates.prepend(envPath);
        } else {
            candidates.prepend(envPath + "/tessdata");
        }
    }

    QStringList normalized;
    for (const QString &candidate : candidates) {
        const QString path = QDir(candidate).absolutePath();
        if (!normalized.contains(path)) {
            normalized << path;
        }
    }
    return normalized;
}

QString resolveTesseractProgram()
{
    const QString appDir = QCoreApplication::applicationDirPath();

    const QByteArray envProgram = qgetenv("TESSERACT_PATH");
    if (!envProgram.isEmpty()) {
        const QString envPath = QDir::fromNativeSeparators(QString::fromUtf8(envProgram));
        if (QFileInfo::exists(envPath)) {
            return QDir(envPath).absolutePath();
        }
    }

    QStringList candidatePaths = {
        appDir + "/thirdpart/ocr-win/bin",
        appDir + "/../thirdpart/ocr-win/bin",
        appDir + "/../../thirdpart/ocr-win/bin",
        appDir + "/thirdpart/ocr/bin",
        appDir + "/../thirdpart/ocr/bin",
        appDir + "/../../thirdpart/ocr/bin"
    };

#ifdef Q_OS_WIN
    candidatePaths << QStringLiteral("C:/Program Files/Tesseract-OCR")
                   << QStringLiteral("C:/Program Files (x86)/Tesseract-OCR");

    const QString localAppData = QDir::fromNativeSeparators(
        QString::fromUtf8(qgetenv("LOCALAPPDATA"))
    );
    if (!localAppData.isEmpty()) {
        candidatePaths << localAppData + "/Programs/Tesseract-OCR";
    }
#endif

    const QStringList executableNames = {
#ifdef Q_OS_WIN
        QStringLiteral("tesseract.exe"),
#endif
        QStringLiteral("tesseract")
    };

    for (const QString &name : executableNames) {
        const QString resolved = QStandardPaths::findExecutable(name, candidatePaths);
        if (!resolved.isEmpty()) {
            return QDir::fromNativeSeparators(resolved);
        }

        const QString fromPath = QStandardPaths::findExecutable(name);
        if (!fromPath.isEmpty()) {
            return QDir::fromNativeSeparators(fromPath);
        }
    }

    return {};
}
} // namespace

QStringList OcrEngine::tessdataSearchCandidates()
{
    return resolveTessdataCandidates();
}

QStringList OcrEngine::availableTessdataDirs()
{
    return resolveTessdataDirs();
}

QString OcrEngine::suggestedTessdataDir()
{
    const QStringList available = availableTessdataDirs();
    if (!available.isEmpty()) {
        return available.first();
    }
    const QStringList candidates = tessdataSearchCandidates();
    if (!candidates.isEmpty()) {
        return candidates.first();
    }
    return {};
}

OcrEngine::OcrEngine()
{
#ifdef ENABLE_OCR
    m_api = new tesseract::TessBaseAPI();

#ifdef Q_OS_WIN
    m_api->SetVariable("debug_file", "NUL");
#else
    m_api->SetVariable("debug_file", "/dev/null");
#endif

    const QStringList tessdataDirs = resolveTessdataDirs();
    bool initOk = false;
    QString selectedTessdataDir;

    for (const QString &tessdataDir : tessdataDirs) {
        m_api->End();
        qputenv("TESSDATA_PREFIX", tessdataDir.toUtf8());

        if (m_api->Init(nullptr, "chi_sim+eng") == 0) {
            initOk = true;
            selectedTessdataDir = tessdataDir;
            break;
        }
    }

    if (!initOk) {
        if (tessdataDirs.isEmpty()) {
            m_lastError = QCoreApplication::translate(
                "OcrEngine",
                "Tesseract Init 失败（chi_sim+eng）：未找到可用 tessdata");
        } else {
            m_lastError = QCoreApplication::translate(
                "OcrEngine",
                "Tesseract Init 失败（chi_sim+eng），候选目录=%1")
                              .arg(tessdataDirs.join(QStringLiteral("; ")));
        }
        qWarning() << "[OcrEngine]" << m_lastError;
        delete m_api;
        m_api = nullptr;
        return;
    }

    qInfo() << "[OcrEngine] Tesseract Init success, tessdata=" << selectedTessdataDir;
    m_api->SetPageSegMode(tesseract::PSM_AUTO);
    m_lastError.clear();
    m_ready = true;
#else
    m_tesseractProgram = resolveTesseractProgram();
    if (m_tesseractProgram.isEmpty()) {
        m_lastError = QCoreApplication::translate(
            "OcrEngine",
            "OCR 未启用：未找到 tesseract 可执行文件。请安装 Tesseract，或设置 TESSERACT_PATH 指向可执行文件");
        m_ready = false;
        return;
    }

    const QStringList tessdataDirs = resolveTessdataDirs();
    if (!tessdataDirs.isEmpty()) {
        m_tessdataDir = tessdataDirs.first();
    }

    qInfo() << "[OcrEngine] CLI fallback enabled, tesseract=" << m_tesseractProgram
            << "tessdata=" << (m_tessdataDir.isEmpty() ? QStringLiteral("<system default>") : m_tessdataDir);
    m_lastError.clear();
    m_ready = true;
#endif
}

OcrEngine::~OcrEngine()
{
#ifdef ENABLE_OCR
    if (m_api) {
        m_api->End();
        delete m_api;
        m_api = nullptr;
    }
#endif
}

QString OcrEngine::recognize(const QImage &image)
{
#ifdef ENABLE_OCR
    if (!m_ready || !m_api) {
        m_lastError = QCoreApplication::translate("OcrEngine", "OCR 引擎未就绪");
        return {};
    }

    if (image.isNull()) {
        m_lastError = QCoreApplication::translate("OcrEngine", "输入图像为空");
        return {};
    }

    QImage rgb = image.convertToFormat(QImage::Format_RGB888);
    if (rgb.width() < 2 || rgb.height() < 2) {
        m_lastError = QCoreApplication::translate("OcrEngine", "输入图像尺寸过小");
        return {};
    }

    // 为图像加白边，降低文本贴边时 Leptonica 产生无效 box 警告的概率。
    constexpr int kPad = 10;
    QImage padded(rgb.width() + kPad * 2, rgb.height() + kPad * 2, QImage::Format_RGB888);
    padded.fill(Qt::white);
    {
        QPainter painter(&padded);
        painter.drawImage(kPad, kPad, rgb);
    }

    m_api->SetSourceResolution(300);
    m_api->SetImage(
        padded.constBits(),
        padded.width(),
        padded.height(),
        3,
        padded.bytesPerLine());

    char *rawText = m_api->GetUTF8Text();
    if (!rawText) {
        m_lastError = QCoreApplication::translate("OcrEngine", "Tesseract GetUTF8Text 返回 null");
        return {};
    }

    const QString result = QString::fromUtf8(rawText).trimmed();
    delete[] rawText;

    m_api->Clear();
    m_lastError.clear();
    return result;
#else
    if (!m_ready || m_tesseractProgram.isEmpty()) {
        if (m_lastError.isEmpty()) {
            m_lastError = QCoreApplication::translate(
                "OcrEngine",
                "OCR 未就绪：未找到 tesseract 可执行文件");
        }
        return {};
    }

    if (image.isNull()) {
        m_lastError = QCoreApplication::translate("OcrEngine", "输入图像为空");
        return {};
    }

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        m_lastError = QCoreApplication::translate("OcrEngine", "OCR 临时目录创建失败");
        return {};
    }

    const QString inputPath = tempDir.path() + QStringLiteral("/ocr_input.png");
    if (!image.save(inputPath, "PNG")) {
        m_lastError = QCoreApplication::translate("OcrEngine", "OCR 临时图片保存失败");
        return {};
    }

    QStringList args;
    args << inputPath << QStringLiteral("stdout") << QStringLiteral("-l") << QStringLiteral("chi_sim+eng");
    if (!m_tessdataDir.isEmpty()) {
        args << QStringLiteral("--tessdata-dir") << m_tessdataDir;
    }

    QProcess process;
    process.start(m_tesseractProgram, args);
    if (!process.waitForStarted(5000)) {
        m_lastError = QCoreApplication::translate("OcrEngine", "OCR 启动失败：无法启动 tesseract 进程");
        return {};
    }

    if (!process.waitForFinished(30000)) {
        process.kill();
        m_lastError = QCoreApplication::translate("OcrEngine", "OCR 超时：tesseract 执行超过 30 秒");
        return {};
    }

    const QByteArray stdOut = process.readAllStandardOutput();
    const QByteArray stdErr = process.readAllStandardError();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        const QString errText = QString::fromLocal8Bit(stdErr).trimmed();
        m_lastError = errText.isEmpty()
            ? QCoreApplication::translate("OcrEngine", "OCR 失败：tesseract 返回非零退出码")
            : QCoreApplication::translate("OcrEngine", "OCR 失败：%1").arg(errText);
        return {};
    }

    const QString result = QString::fromUtf8(stdOut).trimmed();
    if (result.isEmpty()) {
        const QString errText = QString::fromLocal8Bit(stdErr).trimmed();
        m_lastError = errText.isEmpty()
            ? QCoreApplication::translate("OcrEngine", "OCR 未识别到文本")
            : QCoreApplication::translate("OcrEngine", "OCR 未识别到文本：%1").arg(errText);
        return {};
    }

    m_lastError.clear();
    return result;
#endif
}
