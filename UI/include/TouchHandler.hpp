#ifndef TOUCH_HANDLER_HPP
#define TOUCH_HANDLER_HPP

#include <linux/input.h>
#include <string>
#include <atomic>
#include <thread>
#include <functional>

namespace UI {

/**
 * @class TouchHandler
 * @brief Handles touch screen input events using Linux Input Subsystem.
 * Optimized for low-latency event polling without blocking the main UI thread.
 */
class TouchHandler {
public:
    // 定义点击回调函数：当用户点击 (x, y) 时触发
    using ClickCallback = std::function<void(int, int)>;

    TouchHandler(const std::string& device = "/dev/input/event0");
    ~TouchHandler();

    void startListening(ClickCallback callback);
    void stopListening();

private:
    void eventLoop();

    int touchFd;
    std::atomic<bool> running;
    std::thread workerThread;
    ClickCallback onClick;
    
    // 存储当前触摸坐标
    int currentX = 0;
    int currentY = 0;
    int startY = 0;//滑动判定
};

}
#endif