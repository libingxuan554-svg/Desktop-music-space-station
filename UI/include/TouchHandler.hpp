#ifndef TOUCH_HANDLER_HPP
#define TOUCH_HANDLER_HPP

#include <linux/input.h>
#include <string>
#include <atomic>
#include <thread>
#include <functional>

namespace UI {

class TouchHandler {
public:
    using ClickCallback = std::function<void(int x, int y)>;
    using ScrollCallback = std::function<void(int deltaY)>;

    TouchHandler(const std::string& device = "/dev/input/event0");
    ~TouchHandler();

    void startListening(ClickCallback onClick, ScrollCallback onScroll);
    void stopListening();

private:
    void eventLoop();

    int touchFd;
    std::atomic<bool> running;
    std::thread workerThread;
    
    ClickCallback clickCb;
    ScrollCallback scrollCb;
    
    int currentX = 0;
    int currentY = 0;
    int startY = 0;
};

}
#endif
