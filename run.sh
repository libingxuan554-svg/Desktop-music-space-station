#!/bin/bash

# 1. 设置音频运行路径变量
export XDG_RUNTIME_DIR=/run/user/1000

# 2. 进入构建目录
cd build

# 3. 赋予执行权限并以管理员权限启动（带环境变量透传）
# PULSE_SERVER 确保 root 能找到用户的蓝牙音频驱动
sudo -E PULSE_SERVER=unix:/run/user/1000/pulse/native ./MusicStation
