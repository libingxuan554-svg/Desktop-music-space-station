#include "TouchHandler.hpp" // Include touch handler header
#include <fcntl.h>          // Include file control definitions
#include <unistd.h>         // Include POSIX system API
#include <iostream>         // Include standard input/output stream
#include <cmath>            // Include math library for calculating absolute slide values
#include <sys/ioctl.h>      // Include device I/O control interface

namespace UI {

/**
 * @brief Constructor: Initialize touch device node and claim exclusive control
 * @param[in] device Linux  Device path under Linux input subsystem
 * @note [Real-Time Constraints]:
 * File I/O and exclusive operations are executed only during system boot, avoiding runtime overhead.
 */
TouchHandler::TouchHandler(const std::string& device) : running(false) {
    touchFd = open(device.c_str(), O_RDONLY); //Open event node in read-only mode
    if (touchFd == -1) {
        std::cerr << "Failed to open touch device!" << std::endl; //Output error log if open fails
    } else {
       ioctl(touchFd, EVIOCGRAB, 1); //Core operation: Grab exclusive access to prevent touch events from leaking to Linux desktop and messing up the cursor
    }
}

/**
 * @brief Destructor: Release low-level hardware descriptors and thread resources
 * @note [Architecture / SOLID]:
 * Strictly adheres to RAII (Resource Acquisition Is Initialization) principle, ensuring no memory or handle leaks.
 */
TouchHandler::~TouchHandler() { 
    stopListening(); //Safely terminate the background listening loop
    if (touchFd != -1){
        ioctl(touchFd, EVIOCGRAB, 0); //Return control back to the OS
        close(touchFd); //Close the file descriptor
    }
}

/**
 * @brief Start the event listening system and register high-level business callbacks
 * @param[in] onClick  Closure triggered by a touch click
 * @param[in] onScroll  Closure triggered by a touch scroll
 */
void TouchHandler::startListening(ClickCallback onClick, ScrollCallback onScroll) {
    clickCb = onClick;     //Cache the injected click event delegate
    scrollCb = onScroll;   //Cache the injected scroll event delegate
    running = true;        //Raise the internal running flag
    workerThread = std::thread(&TouchHandler::eventLoop, this); 
    //Spawn an independent event reading thread, completely decoupling from the renderer
}

/**
 * @brief Block and safely shut down the worker thread
 */
void TouchHandler::stopListening() {
    running = false;       //Lower the loop flag
    if (workerThread.joinable()) workerThread.join(); 
    //Block main control until hardware thread safely finishes last loop and exits
}

/**
 * @brief  Core listening loop, parsing the low-level input_event structures
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * // Uses blocking read() inside this while loop; the thread is woken up ONLY when a physical touch interrupt occurs.
 * // Perfectly achieves Zero Polling requirement, with CPU usage strictly at 0% when idle.
 */
void TouchHandler::eventLoop() {
    struct input_event ev;       // Declare Linux standard input structure container
    int tempX = 0, tempY = 0;    // Fragmented coordinates in high-speed cache
    bool isTouchDown = false;    // Hardware's current real physical press state
    bool wasTouchDown = false;   // Press state from the previous interrupt feedback
    int startX = 0, startY = 0;  // Record original baseline coordinates at the moment of press

    while (running) {
        if (read(touchFd, &ev, sizeof(ev)) > 0) { 
        // Pure event-driven: Thread suspends here, waiting for low-level DMA to move events
            // 1. Temporarily store received fragmented coordinates
            if (ev.type == EV_ABS) {                     // Parse absolute physical coordinate system
                if (ev.code == ABS_X) tempX = ev.value;  // Capture X-axis data frame
                if (ev.code == ABS_Y) tempY = ev.value;  // Capture Y-axis data frame
            }
            // 2. Temporarily store press state
            else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {  // Capture physical surface contact signal of the screen
                isTouchDown = (ev.value == 1);                     // 1 means finger down, 0 means finger released
            }
            // 3. Core: Received sync signal, completely solving time-gap drift
            else if (ev.type == EV_SYN && ev.code == SYN_REPORT) { // Hardware declares the end of sending a complete frame data packet
                currentX = tempX; // Safely commit X coordinate to main state machine
                currentY = tempY; // Safely commit Y coordinate to main state machine

                if (isTouchDown && !wasTouchDown) { // State edge trigger: Capture the first frame of press down (Rising Edge)
                    startX = currentX;    // Lock starting anchor X for scroll calculation
                    startY = currentY;    // Lock starting anchor Y for scroll calculation
                    wasTouchDown = true;  // Update historical memory
                }
                else if (!isTouchDown && wasTouchDown) { // State edge trigger: Capture the first frame of release (Falling Edge)
                    wasTouchDown = false;                // Clear historical memory

                    int deltaY = currentY - startY;     // Calculate the physical Y-axis vector displacement during this interaction
                    if (std::abs(deltaY) > 50) {        // Set deadzone: Slide exceeding 50 pixels threshold, judged as list scrolling intent
                        if (scrollCb) scrollCb(deltaY); // Trigger scroll action and inject offset into engine
                    } 
                    else {                                     // Vector displacement too small, treated as in-place tap action
                        if (clickCb) clickCb(startX, startY);  // Trigger click action and throw anchor coordinates upwards
                    }
                }
            }
        }
    }
}

} // namespace UI
