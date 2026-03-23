#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "tray_icon_helper.h"
#include "paint_board/paint_board.h"
#include "shortcut_key/globalshortcut.h"
#include "magnifier_provider.h"
#include "sticky_image_store.h"
#include "sticky_image_provider_proxy.h"
#include "language_manager.h"
#include "instance_activation.h"

// Model
#include "model/desktop_snapshot.h"
#include "model/app_settings.h"

// ViewModel
#include "viewmodel/screenshot_view_model.h"
#include "viewmodel/sticky_view_model.h"
#include "viewmodel/ai_view_model.h"
#include "viewmodel/storage_view_model.h"
#include "viewmodel/gif_record_view_model.h"
#include "viewmodel/scroll_capture_view_model.h"
#include "viewmodel/ocr_view_model.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Single-instance activation: second launch notifies the first one and exits.
    InstanceActivation instanceActivation(QStringLiteral("TZshot.Instance"));
    if (!instanceActivation.initialize()) {
        return 0;
    }

    QCoreApplication::setOrganizationName("TZshot");
    QCoreApplication::setOrganizationDomain("tzshot.local");
    QCoreApplication::setApplicationName("TZshot");
    app.setQuitOnLastWindowClosed(false);


    // ── Model 层 ──────────────────────────────────────────
    DesktopSnapshot  desktopSnapshot;
    AppSettings      appSettings;
    StickyImageStore stickyImageStore;

    // ── ViewModel 层 ──────────────────────────────────────
    ScreenshotViewModel screenshotViewModel(desktopSnapshot, stickyImageStore);
    StickyViewModel     stickyViewModel(stickyImageStore);
    AIViewModel         aiViewModel(appSettings, stickyImageStore);
    StorageViewModel    storageViewModel(appSettings);
    LanguageManager     languageManager(appSettings);
    GifRecordViewModel  gifRecordViewModel(appSettings);
    ScrollCaptureViewModel scrollCaptureViewModel(screenshotViewModel, appSettings);
    OcrViewModel        ocrViewModel;


    // ── 其他服务（不属于 MVVM 任何层）───────────────────
    GlobalShortcut globalShortcut;

    QQmlApplicationEngine engine;


    // ── Qt 基础设施注册 ───────────────────────────────────
    engine.addImportPath("qrc:/qml");
    qmlRegisterType<PaintBoard>("CustomComponents", 1, 0, "PaintBoard");
    qmlRegisterType<TrayIconHelper>("com.Tz.tray", 1, 0, "TrayIconHelper");

    // MagnifierProvider 持有 ScreenshotViewModel 引用访问快照，engine 接管生命周期
    engine.addImageProvider("magnifier", new MagnifierProvider(&screenshotViewModel));
    // StickyImageProviderProxy 转发到 StickyImageStore，engine 接管生命周期
    engine.addImageProvider("sticky", new StickyImageProviderProxy(stickyImageStore));

    // ── 暴露给 QML 的 ViewModel ───────────────────────────
    engine.rootContext()->setContextProperty("O_ScreenCapture",      &screenshotViewModel);
    engine.rootContext()->setContextProperty("O_StickyViewModel",   &stickyViewModel);
    engine.rootContext()->setContextProperty("O_AIViewModel",        &aiViewModel);
    engine.rootContext()->setContextProperty("O_ImageSaver",         &storageViewModel);
    engine.rootContext()->setContextProperty("O_GlobalShortcut",     &globalShortcut);
    engine.rootContext()->setContextProperty("O_LanguageManager",    &languageManager);
    engine.rootContext()->setContextProperty("O_GifRecordVM",        &gifRecordViewModel);
    engine.rootContext()->setContextProperty("O_ScrollCaptureVM",    &scrollCaptureViewModel);
    engine.rootContext()->setContextProperty("O_OcrVM",              &ocrViewModel);
    engine.rootContext()->setContextProperty("O_InstanceBridge",     &instanceActivation);

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);
    return app.exec();
}
