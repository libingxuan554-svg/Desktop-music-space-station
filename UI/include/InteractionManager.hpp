#ifndef INTERACTION_MANAGER_HPP
#define INTERACTION_MANAGER_HPP

#include <vector>     // Include dynamic array container
#include <atomic>     // Include atomic operations for lock-free concurrency control
#include <functional> // Include function objects and callback wrappers
#include "../../SystemInterfaces.hpp" // Include system-level core interface protocols
#include "FramebufferUI.hpp"          // Include low-level framebuffer driver engine
#include "UIRenderer.hpp"             // Include high-level graphics rendering components

// Define UI namespace
namespace UI { 

/**
 * @enum UIPage
 * @brief Defines the current UI page state of the system
 */
enum class UIPage { STANDBY, MUSIC_LIST, PLAYER }; // Standby page, music list, player interface

/**
 * @class InteractionManager
 * @brief Core logic class handling UI page transitions and user input distribution.
 * @note [Architecture / SOLID]:
 * Follows SRP: only handles UI state and command mapping, no low-level drawing.
 * Follows DIP: decoupled from business logic via CommandEmitter callback.
 */
class InteractionManager {

public:

/**
 * @brief Define the emitter callback type for control commands
 * // Function wrapper for receiving and sending system commands
 */
    using CommandEmitter = std::function<void(const System::ControlCommand&)>; 

/**
 * @brief Register the command emitter callback function
 * @param[in] emitter  The actual emitter implementation passed in
 * //Bind the communication channel of upper-level business
 */
    static void setCommandEmitter(CommandEmitter emitter); 
 
/**
 * @brief Receive and update playback status and visual spectrum data from audio engine
 * @param[in] status  Playback status structure
 * @param[in] visual  Audio-visual spectrum data structure
 */
    static void updateSystemStatus(const System::PlaybackStatus& status, const System::AudioVisualData& visual); 

/**
 * @brief  Receive and update low-level physical environment sensory data
 * @param[in] env  Environment status structure (temp, humidity, CPU, etc.)
 */
    static void updateEnvStatus(const System::EnvironmentStatus& env); 

/**
 * @brief  Render the current page content to the Framebuffer
 * @param[in] ui  Pointer to the low-level graphics driver FramebufferUI
 * @note [Real-Time Constraints]:
 * Determinism: calls pure memory drawing and preloaded pixel copying, avoiding disk I/O.
 */
    static void renderCurrentPage(FramebufferUI* ui); 

/**
 * @brief  Handle touch click events from hardware
 * @param[in] x  Physical X coordinate of the touch point
 * @param[in] y  Physical Y coordinate of the touch point
 */
    static void handleTouch(int x, int y);

/**
 * @brief  Handle scroll events from hardware
 * @param[in] deltaY  Relative Y-axis scroll offset
 */
    static void handleScroll(int deltaY); 

private:

/**
 * @brief  Get the layout boundaries of all interactive elements on the current page
 * @return  Array containing all UI button structures
 */
    static std::vector<UIButton> getActiveLayout(); 

    // // [REAL-TIME COMPLIANCE]:
    //  Use std::atomic for lock-free concurrency control, ensuring high-frequency UI threads do not block upon reading.
    //  Strictly complies with Zero Polling and rigorous real-time constraints.
    static std::atomic<UIPage> currentPage; //  Atomic type: records currently rendered system page
    static std::atomic<int> scrollOffset; //  Atomic type: records scroll offset of list pages
    
    static CommandEmitter commandEmitter; //  Stores callback function pointer for sending commands to main control bus

    static System::PlaybackStatus currentStatus; //  Caches latest received music playback status
    static System::AudioVisualData currentVisual; //  Caches latest received spectrum rhythm data
    static System::EnvironmentStatus currentEnv; //  Caches latest received physical environment sensory data
};

} // namespace UI
#endif
