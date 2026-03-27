#ifndef TZSHOT_SERVICES_H
#define TZSHOT_SERVICES_H

#include <QObject>

#include "language_manager.h"
#include "model/app_settings.h"
#include "model/desktop_snapshot.h"
#include "shortcut_key/globalshortcut.h"
#include "sticky_image_store.h"
#include "viewmodel/ai_view_model.h"
#include "viewmodel/gif_record_view_model.h"
#include "viewmodel/ocr_view_model.h"
#include "viewmodel/screenshot_view_model.h"
#include "viewmodel/scroll_capture_view_model.h"
#include "viewmodel/sticky_view_model.h"
#include "viewmodel/storage_view_model.h"

class TZShotServices : public QObject
{
    Q_OBJECT

public:
    explicit TZShotServices(QObject *parent = nullptr);

    DesktopSnapshot& desktopSnapshot() { return m_desktopSnapshot; }
    AppSettings& appSettings() { return m_appSettings; }
    StickyImageStore& stickyImageStore() { return m_stickyImageStore; }
    ScreenshotViewModel& screenshotViewModel() { return m_screenshotViewModel; }
    StickyViewModel& stickyViewModel() { return m_stickyViewModel; }
    AIViewModel& aiViewModel() { return m_aiViewModel; }
    StorageViewModel& storageViewModel() { return m_storageViewModel; }
    LanguageManager& languageManager() { return m_languageManager; }
    GifRecordViewModel& gifRecordViewModel() { return m_gifRecordViewModel; }
    ScrollCaptureViewModel& scrollCaptureViewModel() { return m_scrollCaptureViewModel; }
    OcrViewModel& ocrViewModel() { return m_ocrViewModel; }
    GlobalShortcut& globalShortcut() { return m_globalShortcut; }

private:
    DesktopSnapshot m_desktopSnapshot;
    AppSettings m_appSettings;
    StickyImageStore m_stickyImageStore;
    ScreenshotViewModel m_screenshotViewModel;
    StickyViewModel m_stickyViewModel;
    AIViewModel m_aiViewModel;
    StorageViewModel m_storageViewModel;
    LanguageManager m_languageManager;
    GifRecordViewModel m_gifRecordViewModel;
    ScrollCaptureViewModel m_scrollCaptureViewModel;
    OcrViewModel m_ocrViewModel;
    GlobalShortcut m_globalShortcut;
};

#endif // TZSHOT_SERVICES_H
