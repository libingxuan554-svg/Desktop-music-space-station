#ifndef INTERACTION_MANAGER_HPP
#define INTERACTION_MANAGER_HPP

#include <vector>
#include <atomic>
#include <functional>
#include "../../SystemInterfaces.hpp"
#include "FramebufferUI.hpp"
#include "UIRenderer.hpp"

namespace UI {

enum class UIPage { STANDBY, MUSIC_LIST, PLAYER };

class InteractionManager {
public:
    using CommandEmitter = std::function<void(const System::ControlCommand&)>;

    static void setCommandEmitter(CommandEmitter emitter);
    
    static void updateSystemStatus(const System::PlaybackStatus& status, const System::AudioVisualData& visual);
    static void updateEnvStatus(const System::EnvironmentStatus& env);
    
    static void renderCurrentPage(FramebufferUI* ui);

    static void handleTouch(int x, int y);
    static void handleScroll(int deltaY);

private:
    static std::vector<UIButton> getActiveLayout();

    static std::atomic<UIPage> currentPage;
    static std::atomic<int> scrollOffset;
    static CommandEmitter commandEmitter;

    static System::PlaybackStatus currentStatus;
    static System::AudioVisualData currentVisual;
    static System::EnvironmentStatus currentEnv;
};

}
#endif
