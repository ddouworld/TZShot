#ifndef AI_TYPE_H
#define AI_TYPE_H

// AI 模型类型枚举
// 值与 QML 设置页面 aiModel ListModel 的索引一一对应：
//   0 → 阿里云百炼（Qwen）
//   1 → 火山引擎（Seedream）
enum AIType
{
    QWENAI     = 0,
    SEEDREAMAI = 1
};

#endif // AI_TYPE_H
