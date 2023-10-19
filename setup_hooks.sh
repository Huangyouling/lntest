#!/bin/sh

# 复制 post-checkout.sh 到 .git/hooks/ 并重命名为 post-checkout
cp post-checkout.sh .git/hooks/post-checkout

# 赋予 .git/hooks/ 目录下的所有文件执行权限
chmod +x .git/hooks/*

