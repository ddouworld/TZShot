#ifndef GLOBALSHORTCUT_H
#define GLOBALSHORTCUT_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QString>
#include <QMap>

#ifdef Q_OS_UNIX
QT_FORWARD_DECLARE_CLASS(QSocketNotifier)
#endif

/*
 * GlobalShortcut — 全局快捷键管理器（跨平台）
 *
 * 支持 4 个功能动作的快捷键注册、动态修改与 QSettings 持久化。
 * 快捷键格式：标准 Qt 序列字符串，如 "Alt+A"、"Ctrl+Shift+1"。
 *
 * 动作 ID：
 *   1 = 截图
 *   2 = 截图并保存
 *   3 = 贴图到桌面
 *   4 = 显示/隐藏窗口
 *
 * 平台实现：
 *   Linux/Unix — XCB xcb_grab_key + pipe + QSocketNotifier
 *   Windows    — RegisterHotKey + QAbstractNativeEventFilter (WM_HOTKEY)
 */
class GlobalShortcut : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

    Q_PROPERTY(QString shortcutScreenshot     READ shortcutScreenshot     NOTIFY shortcutScreenshotChanged)
    Q_PROPERTY(QString shortcutScreenshotSave READ shortcutScreenshotSave NOTIFY shortcutScreenshotSaveChanged)
    Q_PROPERTY(QString shortcutSticky         READ shortcutSticky         NOTIFY shortcutStickyChanged)
    Q_PROPERTY(QString shortcutToggle         READ shortcutToggle         NOTIFY shortcutToggleChanged)

public:
    explicit GlobalShortcut(QObject *parent = nullptr);
    ~GlobalShortcut();

    /* 读取各动作当前快捷键字符串（供 QML 属性绑定） */
    QString shortcutScreenshot()     const;
    QString shortcutScreenshotSave() const;
    QString shortcutSticky()         const;
    QString shortcutToggle()         const;

    /*
     * 更新某个动作的快捷键（供 QML 调用）
     * actionId : 1~4
     * keySeq   : 如 "Alt+A"、"Ctrl+Shift+1"、"F1"
     * 返回 true 表示解析并注册成功
     */
    Q_INVOKABLE bool updateShortcut(int actionId, const QString &keySeq);

    /* 检查某个快捷键字符串是否已被其他动作占用（0 = 未占用，否则返回占用的 actionId） */
    Q_INVOKABLE int checkConflict(const QString &keySeq) const;

    /* 将 Qt 按键事件中的 key + modifiers 转换为快捷键字符串（供 QML 录制用） */
    Q_INVOKABLE static QString buildKeySequence(int key, int modifiers);

signals:
    /* 4 个功能信号，由 main.qml 连接到对应操作 */
    void screenshotActivated();
    void screenshotSaveActivated();
    void stickyActivated();
    void toggleActivated();

    /* 快捷键变更通知，供 QML 属性绑定刷新 */
    void shortcutScreenshotChanged();
    void shortcutScreenshotSaveChanged();
    void shortcutStickyChanged();
    void shortcutToggleChanged();

protected:
    bool nativeEventFilter(const QByteArray &eventType,
                           void *message, qintptr *result) override;

private slots:
#ifdef Q_OS_UNIX
    void onPipeReadable();
#endif

private:
    /* 解析 keySeq，注册到平台后端，写入 QSettings */
    bool applyShortcut(int actionId, const QString &keySeq);

    /* 读取 QSettings，缺失时使用 defaultSeq */
    QString loadShortcut(int actionId, const QString &defaultSeq) const;

    /* 写入 QSettings */
    void saveShortcut(int actionId, const QString &keySeq) const;

    /* 当前各动作的快捷键字符串，key = actionId (1~4) */
    QMap<int, QString> m_shortcuts;

    /* 默认快捷键 */
    static const char* kDefaults[5]; /* index 1..4 */

#ifdef Q_OS_UNIX
    /* ── Unix 专用：pipe + QSocketNotifier ─────────────── */
    static quint16 modifierToXcbMask(const QString &mod);

    QSocketNotifier *m_notifier = nullptr;
    int  m_readFd  = -1;
    int  m_writeFd = -1;
#endif

#ifdef Q_OS_WIN
    /* ── Windows 专用：VK 键码转换 ─────────────────────── */
    static unsigned int keyNameToVk(const QString &key);
    static unsigned int modifierToWinMask(const QString &mod);
#endif
};

#endif // GLOBALSHORTCUT_H
