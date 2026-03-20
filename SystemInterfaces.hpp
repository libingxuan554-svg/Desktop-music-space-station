#ifndef SYSTEM_INTERFACES_HPP
#define SYSTEM_INTERFACES_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace System {

    /**
     * @brief 1. 指令接口 (UI -> 音频/系统逻辑)
     * 用于发送控制命令。UI 模块是发布者 (Publisher)。
     */
    enum class CommandType {
        PLAY_PAUSE,
        NEXT_TRACK,
        PREV_TRACK,
        VOLUME_UP,
        VOLUME_DOWN,
        SELECT_SONG,   // 配合 intValue 使用 (歌曲索引)
        SEEK_FORWARD,  // 快进
        SEEK_BACKWARD, // 快退
        ENTER_STANDBY  // 进入待机模式
    };

    struct ControlCommand {
        CommandType type;
        int32_t intValue = 0;   // 辅助参数，如音量值或歌曲 ID
        float floatValue = 0.0f; 
    };

    /**
     * @brief 2. 播放状态接口 (音频 -> UI)
     * UI 模块通过订阅此数据来更新歌名、进度条等。
     */
    struct PlaybackStatus {
        std::string songName;
        std::string artist;
        int32_t currentPosition; // 当前播放秒数
        int32_t totalDuration;   // 总时长秒数
        bool isPlaying;
        int32_t volume;          // 当前音量 (0-100)
    };

    /**
     * @brief 3. 实时音频流接口 (音频 -> UI/灯带)
     * 用于频谱动画和灯带律动。高频更新。
     */
    struct AudioVisualData {
        float overallIntensity;       // 整体响度 (0.0-1.0)，对接你的 renderMirrorEqualizer
        std::vector<float> frequencies; // 频段数据（如16个频段），用于高级频谱图
    };

    /**
     * @brief 4. 环境数据接口 (传感器 -> UI/系统逻辑)
     * 用于待机页面的仪表盘显示。
     */
    struct EnvironmentStatus {
        float temperature;
        float humidity;
        float pressure;     // 补充：气压
        uint32_t lux;       // 补充：光照强度（可用于自动调节 UI 亮度）
        std::string timeStr; // 格式化时间
    };
}

#endif