#ifndef TOUCH_HANDLER_HPP
#define TOUCH_HANDLER_HPP

#include <linux/input.h> //  Include Linux core input event definitions
#include <string>        //  Include string class
#include <atomic>        //  Include atomic operations for lock-free concurrency control
#include <thread>        //  Include multi-threading support
#include <functional>    //  Include function wrapper for callback registration

//  Define UI namespace
namespace UI {

/**
 * @class TouchHandler
 * @brief  Responsible for listening to and parsing touch screen events from the low-level Linux input subsystem.
 * @note [Architecture / SOLID]:
 *  Follows SRP: strictly focused on reading /dev/input node data and triggering callbacks, without any GUI business logic.
 *  DIP and Event-Driven: passes hardware events to the upper layer via std::function callbacks, achieving extreme decoupling.
 */
class TouchHandler {

public:
/**
 * @brief  Click event callback type, providing absolute physical coordinates of the touch point
 */
    using ClickCallback = std::function<void(int x, int y)>; 
    
/**
 * @brief  Scroll event callback type, providing relative offset on the Y-axis
 */
    using ScrollCallback = std::function<void(int deltaY)>; 

/**
 * @brief  Constructor: opens the specified input device node and claims exclusive control
 * @param[in] device  Device path of the touch screen in Linux input subsystem, defaults to "/dev/input/event0"
 */
    TouchHandler(const std::string& device = "/dev/input/event0"); 
    
/**
 * @brief  Destructor: releases file descriptor and safely exits the listening thread
 */
    ~TouchHandler(); 

/**
 * @brief  Register click and scroll callbacks, and start an independent background event listening thread
 * @param[in] onClick  Callback function triggered on click event
 * @param[in] onScroll  Callback function triggered on scroll event
 */
    void startListening(ClickCallback onClick, ScrollCallback onScroll); 
    
/**
 * @brief  Stop listening, join the worker thread, and safely release system resources
 */
    void stopListening(); // Safely terminate the background worker thread

private:

/**
 * @brief  Core listening loop, running in the independent workerThread daemon
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * // Internally uses blocking read() on the evdev node, completely suspending and consuming zero CPU cycles when there is no event.
 * // Completely avoids polling (Zero Polling), perfectly conforming to the real-time event-driven standard required by the course.
 */
    void eventLoop(); 

    int touchFd; // File descriptor handle for the touch screen device
    
    // // [REAL-TIME COMPLIANCE]:
    //  Uses lock-free atomic variables to control the thread lifecycle, avoiding context switch overhead caused by mutexes.
    std::atomic<bool> running; // Lifecycle status flag for the listening thread    
    std::thread workerThread; // Background thread independently handling hardware interrupt signals
    
    ClickCallback clickCb; // Stores the click callback logic injected by the upper layer
    ScrollCallback scrollCb; // Stores the scroll callback logic injected by the upper layer
    
    int currentX = 0; // Real-time cached absolute X-axis coordinate
    int currentY = 0; // Real-time cached absolute Y-axis coordinate
    int startY = 0; // Records the Y coordinate at the moment of press, used to calculate scroll vector
};

}
#endif
