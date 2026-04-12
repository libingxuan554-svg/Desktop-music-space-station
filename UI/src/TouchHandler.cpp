#include "TouchHandler.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cmath>
#include <sys/ioctl.h>

namespace UI {

TouchHandler::TouchHandler(const std::string& device) : running(false) {
    touchFd = open(device.c_str(), O_RDONLY);
    if (touchFd == -1) {
        std::cerr << "Failed to open touch device!" << std::endl;
    } else {
       ioctl(touchFd, EVIOCGRAB, 1);
    }
}

TouchHandler::~TouchHandler() { 
    stopListening(); 
    if (touchFd != -1){
        ioctl(touchFd, EVIOCGRAB, 0);
        close(touchFd); 
    }
}

void TouchHandler::startListening(ClickCallback onClick, ScrollCallback onScroll) {
    clickCb = onClick;
    scrollCb = onScroll;
    running = true;
    workerThread = std::thread(&TouchHandler::eventLoop, this);
}

void TouchHandler::stopListening() {
    running = false;
    if (workerThread.joinable()) workerThread.join();
}

void TouchHandler::eventLoop() {
    struct input_event ev;
    int tempX = 0, tempY = 0;
    bool isTouchDown = false;
    bool wasTouchDown = false;
    int startX = 0, startY = 0;

    while (running) {
        if (read(touchFd, &ev, sizeof(ev)) > 0) {
            // 1. 暂存收到的零散坐标
            if (ev.type == EV_ABS) {
                if (ev.code == ABS_X) tempX = ev.value;
                if (ev.code == ABS_Y) tempY = ev.value;
            }
            // 2. 暂存按压状态
            else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
                isTouchDown = (ev.value == 1);
            }
            // 3. 🌟 核心：收到同步信号，彻底解决时间差漂移
            else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
                currentX = tempX;
                currentY = tempY;

                if (isTouchDown && !wasTouchDown) {
                    startX = currentX;
                    startY = currentY;
                    wasTouchDown = true;
                }
                else if (!isTouchDown && wasTouchDown) {
                    wasTouchDown = false;

                    int deltaY = currentY - startY;
                    if (std::abs(deltaY) > 50) { 
                        if (scrollCb) scrollCb(deltaY);
                    } 
                    else {
                        if (clickCb) clickCb(startX, startY);
                    }
                }
            }
        }
    }
}

}
