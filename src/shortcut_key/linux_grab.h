#ifndef LINUX_GRAB_H
#define LINUX_GRAB_H

/*
 * linux_grab — Linux 全局快捷键抓取（XCB + 线程 + pipe）
 *
 * 建立独立 XCB 连接，在后台线程监听 XCB_KEY_PRESS 事件，
 * 匹配已注册的快捷键后通过 pipe 写入 1 字节事件 ID，
 * 主线程用 QSocketNotifier 读取并分发信号。
 *
 * 接口与 win_grab.h 对称，便于 globalshortcut.cpp 统一管理。
 *
 * 事件 ID：
 *   1 = 截图
 *   2 = 截图并保存
 *   3 = 贴图到桌面
 *   4 = 显示/隐藏窗口
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 最多支持注册的快捷键数量（与 win_grab 保持一致） */
#define LINUX_GRAB_MAX_SHORTCUTS  8

/* 事件 ID */
#define LINUX_GRAB_EVENT_SCREENSHOT       1
#define LINUX_GRAB_EVENT_SCREENSHOT_SAVE  2
#define LINUX_GRAB_EVENT_STICKY           3
#define LINUX_GRAB_EVENT_TOGGLE           4

/*
 * 用 Xlib 查询 keysym 对应的硬件 keycode，查完立即关闭 Display
 */
unsigned int linux_grab_keysym_to_keycode(unsigned long keysym);

/*
 * 初始化：建立独立 XCB 连接，启动监听线程
 * notify_fd : 可写端 pipe fd，有事件时写入 1 字节事件 ID
 * 返回 1 成功，0 失败
 */
int linux_grab_init(int notify_fd);

/*
 * 注册快捷键：keycode + mods 组合 → 触发 event_id
 * 必须在 linux_grab_init() 之后调用
 * 同一 event_id 重复注册会先解除旧的再注册新的
 */
void linux_grab_register(unsigned int keycode, uint16_t mods, uint8_t event_id);

/*
 * 解除所有已注册的快捷键（不停止监听线程）
 */
void linux_grab_ungrab_all(void);

/*
 * 停止监听线程并释放资源
 */
void linux_grab_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* LINUX_GRAB_H */
