#!/bin/bash

# 功能：递归查找并删除指定目录下所有的 .o 文件
# 作者：编程助手
# 版本：1.0

# ======================== 配置区 ========================
# 设置要扫描的根目录（默认为当前目录，你可以修改为绝对路径）
TARGET_DIR="."
# 是否开启交互模式（true=删除前确认，false=直接删除）
INTERACTIVE=true
# =======================================================

# 检查目标目录是否存在
if [ ! -d "$TARGET_DIR" ]; then
    echo "错误：目标目录 $TARGET_DIR 不存在！"
    exit 1
fi

# 切换到目标目录
cd "$TARGET_DIR" || exit 1

# 查找所有 .o 文件
echo "正在查找 $TARGET_DIR 目录下的所有 .o 文件..."
O_FILES=$(find . -type f -name "*.o" -print)

# 如果没有找到 .o 文件
if [ -z "$O_FILES" ]; then
    echo "未找到任何 .o 文件，无需删除。"
    exit 0
fi

# 显示找到的 .o 文件列表
echo -e "\n找到以下 .o 文件："
echo "$O_FILES"

# 交互模式确认
if [ "$INTERACTIVE" = true ]; then
    echo -e "\n是否确定删除以上所有 .o 文件？[y/N]"
    read -r CONFIRM
    if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
        echo "用户取消删除操作。"
        exit 0
    fi
fi

# 执行删除操作
echo -e "\n开始删除 .o 文件..."
find . -type f -name "*.o" -delete

# 验证删除结果
REMAINING_FILES=$(find . -type f -name "*.o" -print)
if [ -z "$REMAINING_FILES" ]; then
    echo "✅ 所有 .o 文件已成功删除！"
else
    echo "⚠️  部分 .o 文件删除失败，剩余文件："
    echo "$REMAINING_FILES"
    exit 1
fi

exit 0
