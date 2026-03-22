#ifndef X11_GRAB_H
#define X11_GRAB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 事件 ID：通过 pipe 写给 Qt 主线程，对应 4 个功能动作 */
#define X11_GRAB_EVENT_SCREENSHOT       1   /* 截图           */
#define X11_GRAB_EVENT_SCREENSHOT_SAVE  2   /* 截图并保存     */
#define X11_GRAB_EVENT_STICKY           3   /* 贴图到桌面     */
#define X11_GRAB_EVENT_TOGGLE           4   /* 显示/隐藏窗口  */

/* 最多支持注册的快捷键数量 */
#define X11_GRAB_MAX_SHORTCUTS  8

/* 用 Xlib 查询 keysym 对应的硬件 keycode，查完立即关闭 Display */
unsigned int x11_grab_keysym_to_keycode(unsigned long keysym);

/* 初始化：建立独立 XCB 连接，启动监听线程
 * notify_fd : 可写端 pipe fd，有事件时写入 1 字节事件 ID
 * 返回 1 成功，0 失败 */
int x11_grab_init(int notify_fd);

/* 注册一个快捷键：keycode + mods 组合 → 触发 event_id
 * 必须在 x11_grab_init() 之后调用
 * 同一 event_id 重复注册会先解除旧的再注册新的 */
void x11_grab_register(unsigned int keycode, uint16_t mods, uint8_t event_id);

/* 解除所有已注册的快捷键（不停止监听线程） */
void x11_grab_ungrab_all(void);

/* 停止监听线程并释放资源 */
void x11_grab_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* X11_GRAB_H */
