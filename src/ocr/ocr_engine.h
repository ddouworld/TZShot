#ifndef OCR_ENGINE_H
#define OCR_ENGINE_H

#include <QImage>
#include <QString>

namespace tesseract {
class TessBaseAPI;
}

class OcrEngine
{
public:
    OcrEngine();
    ~OcrEngine();

    // Synchronous OCR recognition, called from a worker thread.
    // Returns trimmed text on success; empty string on failure.
    QString recognize(const QImage &image);

    QString lastError() const { return m_lastError; }
    bool isReady() const { return m_ready; }

    // Diagnostics helpers for UI self-check.
    static QStringList tessdataSearchCandidates();
    static QStringList availableTessdataDirs();
    static QString suggestedTessdataDir();

private:
    tesseract::TessBaseAPI *m_api = nullptr;
    bool m_ready = false;
    QString m_lastError;
    QString m_tesseractProgram;
    QString m_tessdataDir;
};

#endif // OCR_ENGINE_H
