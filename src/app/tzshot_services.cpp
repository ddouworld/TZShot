#include "app/tzshot_services.h"

TZShotServices::TZShotServices(QObject *parent)
    : QObject(parent)
    , m_desktopSnapshot()
    , m_appSettings()
    , m_stickyImageStore(this)
    , m_screenshotViewModel(m_desktopSnapshot, m_stickyImageStore, this)
    , m_stickyViewModel(m_stickyImageStore, this)
    , m_aiViewModel(m_appSettings, m_stickyImageStore, this)
    , m_storageViewModel(m_appSettings, this)
    , m_languageManager(m_appSettings, this)
    , m_gifRecordViewModel(m_appSettings, this)
    , m_scrollCaptureViewModel(m_screenshotViewModel, m_appSettings, this)
    , m_ocrViewModel(this)
    , m_visionViewModel(m_appSettings, m_stickyImageStore, this)
    , m_globalShortcut(this)
{
    m_stickyViewModel.setAiViewModel(&m_aiViewModel);
    m_stickyViewModel.setVisionViewModel(&m_visionViewModel);
}
