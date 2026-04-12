#ifndef MEDIA_PROGRESS_MANAGER_HPP
#define MEDIA_PROGRESS_MANAGER_HPP

#include "WavDecoder.hpp"
#include "../../SystemInterfaces.hpp" 

class MediaProgressManager {
public:
    MediaProgressManager(WavDecoder* dec);

    bool processCommand(const System::ControlCommand& cmd);
    void injectTimeData(System::PlaybackStatus& status);

private:
    WavDecoder* decoder;
};

#endif
