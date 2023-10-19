#!/bin/sh

# 目标目录和链接名称
TARGET="../RTSP"
LINK_NAME="list_test/RTSP"

# 根据操作系统确定要执行的命令
if [ "$(uname)" == "Darwin" ] || [ "$(uname)" == "Linux" ]; then
    # macOS 或 Linux
    ln -sf $TARGET $LINK_NAME
elif [ "$(expr substr $(uname -s) 1 5)" == "MINGW" ]; then
    # Windows (Git Bash)
    # 注意: mklink 在 Git Bash 中可能需要管理员权限
    cmd //C mklink /D $(pwd -W)/$LINK_NAME $(pwd -W)/$TARGET
fi

