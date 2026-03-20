#ifndef MEDIA_PROGRESS_MANAGER_HPP
#define MEDIA_PROGRESS_MANAGER_HPP

#include "WavDecoder.hpp"
#include "RingBuffer.hpp"
#include "../../SystemInterfaces.hpp"

class MediaProgressManager {
public:
    MediaProgressManager(WavDecoder* dec, RingBuffer* buf);
    ~MediaProgressManager() = default;

    // 1. 接收 UI 的指令 (替代原来的 handleUserSeek)
    bool processCommand(const System::ControlCommand& cmd);

    // 2. 将底层的进度数据注入到队友的 UI 状态包里
    void injectTimeData(System::PlaybackStatus& status);

private:
    WavDecoder* decoder;
    RingBuffer* ringBuffer;
};

#endif // MEDIA_PROGRESS_MANAGER_HPP
