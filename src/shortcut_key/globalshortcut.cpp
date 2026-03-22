#include "globalshortcut.h"

#include <QGuiApplication>
#include <QDebug>
#include <QSettings>
#include <QKeySequence>

/* ── 默认快捷键 ─────────────────────────────────────── */
const char* GlobalShortcut::kDefaults[5] = {
    "",           /* index 0 未使用 */
    "Alt+A",      /* 1: 截图           */
    "Alt+S",      /* 2: 截图并保存     */
    "Alt+P",      /* 3: 贴图到桌面     */
    "Alt+Q",      /* 4: 显示/隐藏窗口  */
};

/* ── QSettings 键名 ─────────────────────────────────── */
static QString settingsKey(int actionId)
{
    switch (actionId) {
    case 1: return QStringLiteral("Shortcuts/screenshot");
    case 2: return QStringLiteral("Shortcuts/screenshotSave");
    case 3: return QStringLiteral("Shortcuts/sticky");
    case 4: return QStringLiteral("Shortcuts/toggle");
    }
    return QString();
}

/* ── QSettings 读写 ─────────────────────────────────── */
QString GlobalShortcut::loadShortcut(int actionId, const QString &defaultSeq) const
{
    QSettings s;
    return s.value(settingsKey(actionId), defaultSeq).toString();
}

void GlobalShortcut::saveShortcut(int actionId, const QString &keySeq) const
{
    QSettings s;
    s.setValue(settingsKey(actionId), keySeq);
}

/* ── QML 属性读取 ────────────────────────────────────── */
QString GlobalShortcut::shortcutScreenshot()     const { return m_shortcuts.value(1); }
QString GlobalShortcut::shortcutScreenshotSave() const { return m_shortcuts.value(2); }
QString GlobalShortcut::shortcutSticky()         const { return m_shortcuts.value(3); }
QString GlobalShortcut::shortcutToggle()         const { return m_shortcuts.value(4); }

/* ── buildKeySequence（QML 调用，跨平台通用） ───────── */
QString GlobalShortcut::buildKeySequence(int key, int modifiers)
{
    QStringList parts;
    if (modifiers & Qt::ControlModifier) parts << "Ctrl";
    if (modifiers & Qt::ShiftModifier)   parts << "Shift";
    if (modifiers & Qt::AltModifier)     parts << "Alt";
    if (modifiers & Qt::MetaModifier)    parts << "Meta";

    QString keyName = QKeySequence(key).toString();
    if (keyName.isEmpty()) return QString();

    /* 过滤掉单独按修饰键的情况 */
    if (keyName == "Ctrl" || keyName == "Shift" ||
        keyName == "Alt"  || keyName == "Meta")
        return QString();

    parts << keyName;
    return parts.join('+');
}

/* ── checkConflict（QML 调用） ───────────────────────── */
int GlobalShortcut::checkConflict(const QString &keySeq) const
{
    for (auto it = m_shortcuts.constBegin(); it != m_shortcuts.constEnd(); ++it) {
        if (it.value().compare(keySeq, Qt::CaseInsensitive) == 0)
            return it.key();
    }
    return 0;
}

/* ── updateShortcut（QML 调用） ──────────────────────── */
bool GlobalShortcut::updateShortcut(int actionId, const QString &keySeq)
{
    if (actionId < 1 || actionId > 4 || keySeq.isEmpty()) return false;

    if (!applyShortcut(actionId, keySeq)) return false;

    m_shortcuts[actionId] = keySeq;
    saveShortcut(actionId, keySeq);

    switch (actionId) {
    case 1: emit shortcutScreenshotChanged();     break;
    case 2: emit shortcutScreenshotSaveChanged(); break;
    case 3: emit shortcutStickyChanged();         break;
    case 4: emit shortcutToggleChanged();         break;
    }

    qInfo() << "[GlobalShortcut] 更新快捷键 actionId=" << actionId << "->" << keySeq;
    return true;
}

/* ══════════════════════════════════════════════════════════
 *  以下部分按平台分别实现
 * ══════════════════════════════════════════════════════════ */

/* ────────────────────────────────────────────────────────
 *  UNIX / Linux 实现（XCB + pipe + QSocketNotifier）
 * ──────────────────────────────────────────────────────── */
#ifdef Q_OS_UNIX

#include "linux_grab.h"
#include <QSocketNotifier>
#include <unistd.h>
#include <xcb/xcb.h>

/* XK_ keysym 映射（常用键） */
static unsigned long keyNameToKeySym(const QString &key)
{
    if (key.length() == 1) {
        QChar c = key.at(0).toLower();
        if (c >= 'a' && c <= 'z')
            return (unsigned long)(0x0061 + (c.toLatin1() - 'a'));
        if (c >= '0' && c <= '9')
            return (unsigned long)(0x0030 + (c.toLatin1() - '0'));
    }
    if (key.startsWith("F", Qt::CaseInsensitive) && key.length() <= 3) {
        bool ok = false;
        int n = key.mid(1).toInt(&ok);
        if (ok && n >= 1 && n <= 12)
            return (unsigned long)(0xFFBE + n - 1);
    }
    if (key == "Return" || key == "Enter") return 0xFF0D;
    if (key == "Escape" || key == "Esc")   return 0xFF1B;
    if (key == "Space")                    return 0x0020;
    if (key == "Tab")                      return 0xFF09;
    if (key == "Backspace")                return 0xFF08;
    if (key == "Delete" || key == "Del")   return 0xFFFF;
    if (key == "Insert"  || key == "Ins")  return 0xFF63;
    if (key == "Home")                     return 0xFF50;
    if (key == "End")                      return 0xFF57;
    if (key == "PageUp")                   return 0xFF55;
    if (key == "PageDown")                 return 0xFF56;
    if (key == "Left")                     return 0xFF51;
    if (key == "Right")                    return 0xFF53;
    if (key == "Up")                       return 0xFF52;
    if (key == "Down")                     return 0xFF54;
    return 0;
}

quint16 GlobalShortcut::modifierToXcbMask(const QString &mod)
{
    if (mod == "Ctrl"  || mod == "Control") return XCB_MOD_MASK_CONTROL;
    if (mod == "Shift")                     return XCB_MOD_MASK_SHIFT;
    if (mod == "Alt"   || mod == "Meta")    return XCB_MOD_MASK_1;
    if (mod == "Super" || mod == "Win")     return XCB_MOD_MASK_4;
    return 0;
}

bool GlobalShortcut::applyShortcut(int actionId, const QString &keySeq)
{
    if (keySeq.isEmpty()) return false;

    QStringList parts = keySeq.split('+', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return false;

    QString mainKey = parts.last();
    quint16 mods    = 0;
    for (int i = 0; i < parts.size() - 1; i++) {
        mods = (quint16)(mods | modifierToXcbMask(parts.at(i)));
    }

    unsigned long keysym = keyNameToKeySym(mainKey);
    if (keysym == 0) {
        qWarning() << "[GlobalShortcut] 无法识别主键：" << mainKey;
        return false;
    }

    unsigned int keycode = linux_grab_keysym_to_keycode(keysym);
    if (keycode == 0) {
        qWarning() << "[GlobalShortcut] keycode 查询失败：" << mainKey;
        return false;
    }

    linux_grab_register(keycode, mods, (uint8_t)actionId);
    qInfo() << "[GlobalShortcut] 注册快捷键" << keySeq
            << "→ actionId=" << actionId
            << "keycode=" << keycode << "mods=" << mods;
    return true;
}

GlobalShortcut::GlobalShortcut(QObject *parent)
    : QObject(parent)
{
    int fds[2];
    if (pipe(fds) != 0) {
        qCritical() << "[GlobalShortcut] pipe 创建失败";
        return;
    }
    m_readFd  = fds[0];
    m_writeFd = fds[1];

    if (!linux_grab_init(m_writeFd)) {
        qCritical() << "[GlobalShortcut] linux_grab_init 失败";
        ::close(m_readFd);
        ::close(m_writeFd);
        m_readFd = m_writeFd = -1;
        return;
    }

    for (int id = 1; id <= 4; id++) {
        QString seq = loadShortcut(id, QString::fromLatin1(kDefaults[id]));
        m_shortcuts[id] = seq;
        applyShortcut(id, seq);
    }

    m_notifier = new QSocketNotifier(m_readFd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated,
            this, &GlobalShortcut::onPipeReadable);

    qInfo() << "[GlobalShortcut] Unix 初始化完成";
}

GlobalShortcut::~GlobalShortcut()
{
    linux_grab_cleanup();
    if (m_readFd  >= 0) { ::close(m_readFd);  m_readFd  = -1; }
    if (m_writeFd >= 0) { ::close(m_writeFd); m_writeFd = -1; }
}

void GlobalShortcut::onPipeReadable()
{
    uint8_t evt = 0;
    if (::read(m_readFd, &evt, 1) != 1) return;

    qDebug() << "[GlobalShortcut] 触发事件 id=" << evt;

    switch (evt) {
    case LINUX_GRAB_EVENT_SCREENSHOT:      emit screenshotActivated();      break;
    case LINUX_GRAB_EVENT_SCREENSHOT_SAVE: emit screenshotSaveActivated();  break;
    case LINUX_GRAB_EVENT_STICKY:          emit stickyActivated();          break;
    case LINUX_GRAB_EVENT_TOGGLE:          emit toggleActivated();          break;
    default: break;
    }
}

bool GlobalShortcut::nativeEventFilter(const QByteArray &, void *, qintptr *)
{
    return false; /* Unix 通过 pipe 分发，此处不处理 */
}

#endif /* Q_OS_UNIX */

/* ────────────────────────────────────────────────────────
 *  Windows 实现（RegisterHotKey + WM_HOTKEY）
 * ──────────────────────────────────────────────────────── */
#ifdef Q_OS_WIN

#include "win_grab.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* 键名 → Windows 虚拟键码 */
unsigned int GlobalShortcut::keyNameToVk(const QString &key)
{
    /* 字母 A-Z */
    if (key.length() == 1) {
        QChar c = key.at(0).toUpper();
        if (c >= 'A' && c <= 'Z') return (unsigned int)c.toLatin1(); /* VK_A = 'A' */
        if (c >= '0' && c <= '9') return (unsigned int)c.toLatin1(); /* VK_0 = '0' */
    }
    /* F1-F12 */
    if (key.startsWith("F", Qt::CaseInsensitive) && key.length() <= 3) {
        bool ok = false;
        int n = key.mid(1).toInt(&ok);
        if (ok && n >= 1 && n <= 12) return (unsigned int)(VK_F1 + n - 1);
    }
    /* 特殊键 */
    if (key == "Return"   || key == "Enter")    return VK_RETURN;
    if (key == "Escape"   || key == "Esc")      return VK_ESCAPE;
    if (key == "Space")                         return VK_SPACE;
    if (key == "Tab")                           return VK_TAB;
    if (key == "Backspace")                     return VK_BACK;
    if (key == "Delete"   || key == "Del")      return VK_DELETE;
    if (key == "Insert"   || key == "Ins")      return VK_INSERT;
    if (key == "Home")                          return VK_HOME;
    if (key == "End")                           return VK_END;
    if (key == "PageUp"   || key == "PgUp")     return VK_PRIOR;
    if (key == "PageDown" || key == "PgDown")   return VK_NEXT;
    if (key == "Left")                          return VK_LEFT;
    if (key == "Right")                         return VK_RIGHT;
    if (key == "Up")                            return VK_UP;
    if (key == "Down")                          return VK_DOWN;
    if (key == "PrintScreen")                   return VK_SNAPSHOT;
    if (key == "Pause")                         return VK_PAUSE;
    if (key == "CapsLock")                      return VK_CAPITAL;
    if (key == "NumLock")                       return VK_NUMLOCK;
    if (key == "ScrollLock")                    return VK_SCROLL;
    return 0;
}

/* 修饰键字符串 → Windows MOD_* 掩码 */
unsigned int GlobalShortcut::modifierToWinMask(const QString &mod)
{
    if (mod == "Ctrl"  || mod == "Control") return MOD_CONTROL;
    if (mod == "Shift")                     return MOD_SHIFT;
    if (mod == "Alt"   || mod == "Meta")    return MOD_ALT;
    if (mod == "Win"   || mod == "Super")   return MOD_WIN;
    return 0;
}

bool GlobalShortcut::applyShortcut(int actionId, const QString &keySeq)
{
    if (keySeq.isEmpty()) return false;

    QStringList parts = keySeq.split('+', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return false;

    QString mainKey  = parts.last();
    unsigned int mods = 0;
    for (int i = 0; i < parts.size() - 1; i++) {
        mods |= modifierToWinMask(parts.at(i));
    }

    unsigned int vk = keyNameToVk(mainKey);
    if (vk == 0) {
        qWarning() << "[GlobalShortcut] 无法识别主键：" << mainKey;
        return false;
    }

    win_grab_register(vk, mods, (uint8_t)actionId);
    qInfo() << "[GlobalShortcut] 注册快捷键" << keySeq
            << "→ actionId=" << actionId
            << "vk=0x" << Qt::hex << vk << "mods=0x" << mods;
    return true;
}

GlobalShortcut::GlobalShortcut(QObject *parent)
    : QObject(parent)
{
    win_grab_init();

    for (int id = 1; id <= 4; id++) {
        QString seq = loadShortcut(id, QString::fromLatin1(kDefaults[id]));
        m_shortcuts[id] = seq;
        applyShortcut(id, seq);
    }

    /* 将自身注册为 Qt 原生事件过滤器，以便接收 WM_HOTKEY */
    qApp->installNativeEventFilter(this);

    qInfo() << "[GlobalShortcut] Windows 初始化完成";
}

GlobalShortcut::~GlobalShortcut()
{
    qApp->removeNativeEventFilter(this);
    win_grab_cleanup();
}

bool GlobalShortcut::nativeEventFilter(const QByteArray &eventType,
                                       void *message, qintptr *result)
{
    Q_UNUSED(result)
    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
        return false;

    MSG *msg = static_cast<MSG*>(message);
    if (!msg || msg->message != WM_HOTKEY)
        return false;

    uint8_t evt = (uint8_t)msg->wParam; /* wParam = hotkey id = actionId */

    qDebug() << "[GlobalShortcut] WM_HOTKEY 触发事件 id=" << evt;

    switch (evt) {
    case WIN_GRAB_EVENT_SCREENSHOT:      emit screenshotActivated();      break;
    case WIN_GRAB_EVENT_SCREENSHOT_SAVE: emit screenshotSaveActivated();  break;
    case WIN_GRAB_EVENT_STICKY:          emit stickyActivated();          break;
    case WIN_GRAB_EVENT_TOGGLE:          emit toggleActivated();          break;
    default: return false;
    }
    return true; /* 已处理，不再传递 */
}

#endif /* Q_OS_WIN */
