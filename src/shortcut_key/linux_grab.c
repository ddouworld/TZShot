/* 纯 C 翻译单元：独立 XCB 连接 + 线程监听按键，通过 pipe 通知 Qt */
#include "linux_grab.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* ── 动态快捷键表 ─────────────────────────────────────── */
typedef struct {
    uint8_t  keycode;
    uint16_t mods;
    uint8_t  event_id;  /* 0 表示该槽位空闲 */
} LinuxShortcutEntry;

static LinuxShortcutEntry s_shortcuts[LINUX_GRAB_MAX_SHORTCUTS];
static int                s_shortcut_count = 0;
static pthread_mutex_t    s_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ── XCB 全局状态 ─────────────────────────────────────── */
static xcb_connection_t *s_conn      = NULL;
static xcb_window_t      s_root      = 0;
static pthread_t         s_thread;
static int               s_notify_fd = -1;
static volatile int      s_running   = 0;

/* NumLock / CapsLock 修饰键组合，每个快捷键注册 4 次 */
static const uint16_t kExtraMods[] = {
    0,
    XCB_MOD_MASK_2,                        /* NumLock  */
    XCB_MOD_MASK_LOCK,                     /* CapsLock */
    XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK
};
#define EXTRA_MODS_COUNT 4

/* ── 内部辅助函数 ─────────────────────────────────────── */

static void do_grab_key(uint8_t keycode, uint16_t mods)
{
    unsigned int i;
    for (i = 0; i < EXTRA_MODS_COUNT; i++) {
        xcb_grab_key(s_conn,
                     0,               /* owner_events = 0 */
                     s_root,
                     (uint16_t)(mods | kExtraMods[i]),
                     keycode,
                     XCB_GRAB_MODE_ASYNC,
                     XCB_GRAB_MODE_ASYNC);
    }
}

static void do_ungrab_key(uint8_t keycode, uint16_t mods)
{
    unsigned int i;
    for (i = 0; i < EXTRA_MODS_COUNT; i++) {
        xcb_ungrab_key(s_conn,
                       keycode,
                       s_root,
                       (uint16_t)(mods | kExtraMods[i]));
    }
}

/* ── 监听线程 ─────────────────────────────────────────── */

static void *event_thread(void *arg)
{
    (void)arg;
    while (s_running) {
        xcb_generic_event_t *ev = xcb_wait_for_event(s_conn);
        if (!ev) break; /* 连接断开，线程退出 */

        if ((ev->response_type & ~0x80) == XCB_KEY_PRESS) {
            xcb_key_press_event_t *kev = (xcb_key_press_event_t *)ev;
            uint8_t  kc   = kev->detail;
            /* 屏蔽 NumLock / CapsLock，避免影响匹配 */
            uint16_t mods = (uint16_t)(kev->state &
                            ~(uint16_t)(XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK));

            uint8_t evt = 0;
            int i;
            pthread_mutex_lock(&s_mutex);
            for (i = 0; i < s_shortcut_count; i++) {
                if (s_shortcuts[i].event_id != 0 &&
                    s_shortcuts[i].keycode  == kc &&
                    s_shortcuts[i].mods     == mods) {
                    evt = s_shortcuts[i].event_id;
                    break;
                }
            }
            pthread_mutex_unlock(&s_mutex);

            if (evt != 0) {
                write(s_notify_fd, &evt, 1);
            }
        }
        free(ev);
    }
    return NULL;
}

/* ── 公共接口实现 ─────────────────────────────────────── */

unsigned int linux_grab_keysym_to_keycode(unsigned long keysym)
{
    unsigned int kc = 0;
    Display *dpy = XOpenDisplay(NULL);
    if (dpy) {
        kc = (unsigned int)XKeysymToKeycode(dpy, (KeySym)keysym);
        XCloseDisplay(dpy);
    }
    return kc;
}

int linux_grab_init(int notify_fd)
{
    const xcb_setup_t     *setup;
    xcb_screen_iterator_t  iter;

    s_notify_fd = notify_fd;
    memset(s_shortcuts, 0, sizeof(s_shortcuts));
    s_shortcut_count = 0;

    s_conn = xcb_connect(NULL, NULL);
    if (!s_conn || xcb_connection_has_error(s_conn)) return 0;

    setup  = xcb_get_setup(s_conn);
    iter   = xcb_setup_roots_iterator(setup);
    s_root = iter.data->root;

    s_running = 1;
    pthread_create(&s_thread, NULL, event_thread, NULL);
    return 1;
}

void linux_grab_register(unsigned int keycode, uint16_t mods, uint8_t event_id)
{
    int i;
    if (!s_conn || keycode == 0 || event_id == 0) return;

    pthread_mutex_lock(&s_mutex);

    /* 如果该 event_id 已存在，先解除旧的注册 */
    for (i = 0; i < s_shortcut_count; i++) {
        if (s_shortcuts[i].event_id == event_id) {
            do_ungrab_key(s_shortcuts[i].keycode, s_shortcuts[i].mods);
            /* 从数组中移除该槽位 */
            s_shortcuts[i] = s_shortcuts[s_shortcut_count - 1];
            s_shortcut_count--;
            break;
        }
    }

    /* 注册新快捷键 */
    if (s_shortcut_count < LINUX_GRAB_MAX_SHORTCUTS) {
        s_shortcuts[s_shortcut_count].keycode  = (uint8_t)keycode;
        s_shortcuts[s_shortcut_count].mods     = mods;
        s_shortcuts[s_shortcut_count].event_id = event_id;
        s_shortcut_count++;
        do_grab_key((uint8_t)keycode, mods);
        xcb_flush(s_conn);
    }

    pthread_mutex_unlock(&s_mutex);
}

void linux_grab_ungrab_all(void)
{
    int i;
    if (!s_conn) return;

    pthread_mutex_lock(&s_mutex);
    for (i = 0; i < s_shortcut_count; i++) {
        if (s_shortcuts[i].event_id != 0) {
            do_ungrab_key(s_shortcuts[i].keycode, s_shortcuts[i].mods);
        }
    }
    memset(s_shortcuts, 0, sizeof(s_shortcuts));
    s_shortcut_count = 0;
    xcb_flush(s_conn);
    pthread_mutex_unlock(&s_mutex);
}

void linux_grab_cleanup(void)
{
    s_running = 0;
    if (s_conn) {
        xcb_disconnect(s_conn); /* 使 xcb_wait_for_event 返回 NULL，线程自然退出 */
        s_conn = NULL;
    }
    pthread_join(s_thread, NULL);
    pthread_mutex_destroy(&s_mutex);
}
