#!/bin/bash
# ---------------------------------------------------------
# 🚀 Team 15: 空间站环境一键配置脚本 (2026-04-12 版)
# ---------------------------------------------------------

echo "🔧 步骤 1: 安装底层开发依赖..."
sudo apt-get update
sudo apt-get install -y libasound2-dev cmake g++ bluetooth bluez libfftw3-dev

echo "🔐 步骤 2: 配置硬件访问权限..."
# 允许用户直接读写显示缓冲区、触摸屏和 SPI 接口
sudo usermod -aG video $USER
sudo usermod -aG input $USER
sudo usermod -aG spi $USER

echo "♾️ 步骤 3: 开启用户会话持久化..."
# 关键：解决蓝牙音频在 SSH 退出后自动断开的 Bug
sudo loginctl enable-linger $USER

echo "📂 步骤 4: 创建构建目录..."
mkdir -p build

echo "✅ 配置完成！请【重新登录】树莓派或执行 'newgrp video'，然后即可开始编译。"
# 在 setup.sh 文件的末尾添加
echo "----------------------------------------------------"
echo "💡 提示：系统环境已就绪。"
echo "请确保你的 JBL 蓝牙音箱已开启配对模式，然后手动执行："
echo "bluetoothctl"
echo "在控制台中输入: pair [你的音箱MAC], trust [你的音箱MAC], connect [你的音箱MAC]"
echo "----------------------------------------------------"
