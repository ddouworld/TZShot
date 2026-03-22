/*
 * win_grab.c — Windows 全局快捷键注册/注销（纯 C）
 *
 * 使用 RegisterHotKey / UnregisterHotKey 在当前线程（主线程）注册热键。
 * WM_HOTKEY 消息由 Qt 主消息循环接收，GlobalShortcut 通过
 * QAbstractNativeEventFilter::nativeEventFilter 拦截并分发。
 *
 * 注意：RegisterHotKey 必须在接收 WM_HOTKEY 的同一线程调用，
 * 因此所有操作均在主线程完成，无需额外线程或 pipe。
 */

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "win_grab.h"
#include <string.h>

/* ── 快捷键注册表 ─────────────────────────────────────── */
typedef struct {
    unsigned int vk;        /* 虚拟键码，0 表示该槽位空闲 */
    unsigned int mods;      /* MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN */
    uint8_t      event_id;  /* 对应动作 ID (1~4) */
} WinShortcutEntry;

static WinShortcutEntry s_shortcuts[WIN_GRAB_MAX_SHORTCUTS];
static int              s_count = 0;

/* ── 公共接口实现 ─────────────────────────────────────── */

int win_grab_init(void)
{
    memset(s_shortcuts, 0, sizeof(s_shortcuts));
    s_count = 0;
    return 1;
}

void win_grab_register(unsigned int vk, unsigned int mods, uint8_t event_id)
{
    int i;
    if (vk == 0 || event_id == 0) return;

    /* 如果该 event_id 已存在，先注销旧的 */
    for (i = 0; i < s_count; i++) {
        if (s_shortcuts[i].event_id == event_id) {
            UnregisterHotKey(NULL, (int)event_id);
            /* 从数组中移除 */
            s_shortcuts[i] = s_shortcuts[s_count - 1];
            memset(&s_shortcuts[s_count - 1], 0, sizeof(WinShortcutEntry));
            s_count--;
            break;
        }
    }

    /* 注册新快捷键，使用 event_id 作为 hotkey id（1~4，不与系统冲突） */
    /* MOD_NOREPEAT 避免按住时重复触发（Windows 7+ 支持） */
    if (s_count < WIN_GRAB_MAX_SHORTCUTS) {
        if (RegisterHotKey(NULL, (int)event_id, mods | MOD_NOREPEAT, vk)) {
            s_shortcuts[s_count].vk       = vk;
            s_shortcuts[s_count].mods     = mods;
            s_shortcuts[s_count].event_id = event_id;
            s_count++;
        }
    }
}

void win_grab_ungrab_all(void)
{
    int i;
    for (i = 0; i < s_count; i++) {
        if (s_shortcuts[i].event_id != 0) {
            UnregisterHotKey(NULL, (int)s_shortcuts[i].event_id);
        }
    }
    memset(s_shortcuts, 0, sizeof(s_shortcuts));
    s_count = 0;
}

void win_grab_cleanup(void)
{
    win_grab_ungrab_all();
}

unsigned int win_grab_get_vk(uint8_t event_id)
{
    int i;
    for (i = 0; i < s_count; i++) {
        if (s_shortcuts[i].event_id == event_id)
            return s_shortcuts[i].vk;
    }
    return 0;
}

unsigned int win_grab_get_mods(uint8_t event_id)
{
    int i;
    for (i = 0; i < s_count; i++) {
        if (s_shortcuts[i].event_id == event_id)
            return s_shortcuts[i].mods;
    }
    return 0;
}

#endif /* _WIN32 */
