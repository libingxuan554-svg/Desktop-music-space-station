#ifndef MUSIC_CONTROLLER_HPP
#define MUSIC_CONTROLLER_HPP

#include <vector>
#include <string>
#include <atomic>

// 引入根目录的全局接口
#include "../../SystemInterfaces.hpp" 

// 引入你自己的组件
#include "WavDecoder.hpp"
#include "RingBuffer.hpp"

// 引入队友的组件（因为已经搬到了 MPM/include 下，所以直接引用）
#include "AudioSource.hpp"

// 继承队友的 AudioSource 接口，成为引擎的供弹机
class MusicController : public AudioSource {
public:
    MusicController(WavDecoder* dec, RingBuffer* buf);
    ~MusicController() = default;

    // 1. 初始化并加载播放列表
    void setPlaylist(const std::vector<std::string>& list);
    
    // 2. 接收 UI 的所有指令（播放、暂停、切歌、音量）
    bool processCommand(const System::ControlCommand& cmd);

    // 3. 实现队友的虚函数：引擎每次缺数据都会疯狂调用这里！
    void onProcessAudio(std::vector<float>& buffer, uint32_t numFrames) override;

private:
    void loadCurrentTrack(); // 加载当前序号的歌曲

    WavDecoder* decoder;
    RingBuffer* ringBuffer;

    // 播放器状态
    std::vector<std::string> playlist;
    int currentTrackIndex = 0;
    
    std::atomic<bool> isPlaying{false}; // 播放/暂停标记
    std::atomic<float> volume{1.0f};    // 音量 (0.0 到 1.0)
};

#endif
