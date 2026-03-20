#include "TouchHandler.hpp"
#include "InteractionManager.hpp" //  对接交互层
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cmath>

namespace UI {
    TouchHandler::TouchHandler(const std::string& device) : running(false) {
        touchFd = open(device.c_str(), O_RDONLY);
		if (touchFd == -1) {
        // 错误处理
        std::cerr << "Failed to open touch device!" << std::endl;
    }

    TouchHandler::~TouchHandler() { stopListening(); if (touchFd != -1) close(touchFd); }

    void TouchHandler::startListening(ClickCallback callback) {
        onClick = callback;
        running = true;
        workerThread = std::thread(&TouchHandler::eventLoop, this);
    }

    void TouchHandler::stopListening() {
        running = false;
        if (workerThread.joinable()) workerThread.join();
    }

    void TouchHandler::eventLoop() {
        struct input_event ev;
		// 树莓派 7 寸屏典型坐标范围校准 (0-4095 -> 800x480)
		const float scaleX = 800.0f / 4095.0f;
		const float scaleY = 480.0f / 4095.0f;
	
        while (running) {
            if (read(touchFd, &ev, sizeof(ev)) > 0) {
				// 1. 处理坐标更新 (EV_ABS)
                if (ev.type == EV_ABS) {
                    if (ev.code == ABS_X) currentX = static_cast<int>(ev.value * scaleX);
                if (ev.code == ABS_Y) currentY = static_cast<int>(ev.value * scaleY);
                } 
                
                // 2. 处理按键事件 (EV_KEY)
                else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
                    if (ev.value == 1) { 
                        // [新增逻辑] 手指按下：记录初始高度
                        startY = currentY; 
                    } 
                    else if (ev.value == 0) { 
                        // [新增逻辑] 手指抬起：计算垂直位移
                        int deltaY = currentY - startY;

                        if (std::abs(deltaY) > 40) { 
                            // 情况 A：位移较大，识别为滑动
                            // 调用 InteractionManager 的滑动接口，实现歌单滚动
                            InteractionManager::handleScroll(deltaY);
                        } 
                        else {
                            // 情况 B：位移很小，识别为点击
                            // 1. 触发你原始定义的 onClick 回调
                            if (onClick) onClick(currentX, currentY);
                            
                            // 2. [新增对接] 同时发送给交互管理器进行按钮判定
						    InteractionManager::handleTouch(currentX, currentY);
						}
					}
				}
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}