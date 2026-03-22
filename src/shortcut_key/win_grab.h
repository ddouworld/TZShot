#ifndef WIN_GRAB_H
#define WIN_GRAB_H

/*
 * win_grab — Windows 全局快捷键抓取
 *
 * 使用 Windows RegisterHotKey / UnregisterHotKey API 注册快捷键，
 * 通过 Qt 的 QAbstractNativeEventFilter 在主线程消息循环中接收 WM_HOTKEY，
 * 触发回调通知上层（与 x11_grab 的 pipe 通知对称）。
 *
 * 事件 ID 与 x11_grab.h 保持一致：
 *   1 = 截图
 *   2 = 截图并保存
 *   3 = 贴图到桌面
 *   4 = 显示/隐藏窗口
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 最多支持注册的快捷键数量（与 x11_grab 保持一致） */
#define WIN_GRAB_MAX_SHORTCUTS  8

/* 事件 ID（与 x11_grab.h 中的宏值相同） */
#define WIN_GRAB_EVENT_SCREENSHOT       1
#define WIN_GRAB_EVENT_SCREENSHOT_SAVE  2
#define WIN_GRAB_EVENT_STICKY           3
#define WIN_GRAB_EVENT_TOGGLE           4

/*
 * 初始化（Windows 平台为空操作，只做状态清零，保持接口对称）
 * 返回 1 成功
 */
int win_grab_init(void);

/*
 * 注册快捷键
 * vk      : Windows 虚拟键码 (VK_*)
 * mods    : MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN 组合
 * event_id: 触发时的事件 ID（1~4）
 */
void win_grab_register(unsigned int vk, unsigned int mods, uint8_t event_id);

/*
 * 解除所有已注册快捷键
 */
void win_grab_ungrab_all(void);

/*
 * 释放所有资源（对称接口）
 */
void win_grab_cleanup(void);

/*
 * 查询某个事件 ID 对应的虚拟键码（0 表示未注册）
 */
unsigned int win_grab_get_vk(uint8_t event_id);

/*
 * 查询某个事件 ID 对应的修饰键掩码
 */
unsigned int win_grab_get_mods(uint8_t event_id);

#ifdef __cplusplus
}
#endif

#endif /* WIN_GRAB_H */
