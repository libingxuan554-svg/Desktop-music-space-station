#include "InteractionManager.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>    // 确保包含时间库
#include <ctime>

namespace UI {

std::atomic<UIPage> InteractionManager::currentPage{UIPage::STANDBY};
std::atomic<int> InteractionManager::scrollOffset{0};
InteractionManager::CommandEmitter InteractionManager::commandEmitter = nullptr;

System::PlaybackStatus InteractionManager::currentStatus;
System::AudioVisualData InteractionManager::currentVisual;
System::EnvironmentStatus InteractionManager::currentEnv;

static int volumeDisplayTimer = 0;
static float g_songScrollX = 0;

static std::string pressedBtnId = "";
static int pressedBtnTimer = 0;

void InteractionManager::setCommandEmitter(CommandEmitter emitter) {
    commandEmitter = emitter;
}

void InteractionManager::updateSystemStatus(const System::PlaybackStatus& status, const System::AudioVisualData& visual) {
    if (currentStatus.songName != status.songName) {
        g_songScrollX = 0;
    }
    currentStatus = status;
    currentVisual = visual;
}

void InteractionManager::updateEnvStatus(const System::EnvironmentStatus& env) {
    currentEnv = env;
}

std::vector<UIButton> InteractionManager::getActiveLayout() {
    std::vector<UIButton> layout;
    UIPage page = currentPage.load();

    // ============== 触控层布局 (保持冻结安全) ==============
if (page == UIPage::MUSIC_LIST) {

        // 从 LIST 退回 STANDBY 时，向底层发送待机指令
        layout.push_back({45, 25, 130, 50, "BACK", {0,0,0}, [](){
            if(commandEmitter) commandEmitter({System::CommandType::ENTER_STANDBY, 0, 0.0f});
            currentPage.store(UIPage::STANDBY);
        }});

        int offset = scrollOffset.load();
        int songCount = currentStatus.playlist.size();
        for(int i = 0; i < songCount; ++i) {
            int btnY = 85 * i + offset + 120;
            if (btnY < -60 || btnY > 600) continue;
            layout.push_back({60, btnY, 904, 60, currentStatus.playlist[i], {0, 0, 0}, [i](){
                if(commandEmitter) commandEmitter({System::CommandType::SELECT_SONG, i, 0.0f});
                currentPage.store(UIPage::PLAYER);
                g_songScrollX = 0;
            }});
        }
    }
    else if (page == UIPage::PLAYER) {
        layout.push_back({55, 45, 130, 50, "BACK", {0,0,0}, [](){ currentPage.store(UIPage::MUSIC_LIST); }});

        int btnW = 140, spacing = 28, startX = 106, yPos = 410;
        layout.push_back({startX, yPos, btnW, 50, "VOL-", {0,0,0}, [](){ if(commandEmitter) commandEmitter({System::CommandType::VOLUME_DOWN}); volumeDisplayTimer = 45; }});
        layout.push_back({startX + (btnW + spacing), yPos, btnW, 50, "PREV", {0,0,0}, [](){ if(commandEmitter) commandEmitter({System::CommandType::PREV_TRACK}); }});
        layout.push_back({startX + (btnW + spacing)*2, yPos, btnW, 50, "PLAY", {0,0,0}, [](){ if(commandEmitter) commandEmitter({System::CommandType::PLAY_PAUSE}); }});
        layout.push_back({startX + (btnW + spacing)*3, yPos, btnW, 50, "NEXT", {0,0,0}, [](){ if(commandEmitter) commandEmitter({System::CommandType::NEXT_TRACK}); }});
        layout.push_back({startX + (btnW + spacing)*4, yPos, btnW, 50, "VOL+", {0,0,0}, [](){ if(commandEmitter) commandEmitter({System::CommandType::VOLUME_UP}); volumeDisplayTimer = 45; }});
    }
    return layout;
}

void InteractionManager::renderCurrentPage(FramebufferUI* ui) {
    ui->lock();

    if (pressedBtnTimer > 0) {
        pressedBtnTimer--;
    } else {
        pressedBtnId = "";
    }

    UIPage page = currentPage.load();

  // ============== 👇 STANDBY 渲染层 (编辑中) 👇 ==============
    if (page == UIPage::STANDBY) {
        ui->drawPreloadedBmp("STANDBY");

        // 🌟 1. 获取并渲染真实的系统时间 (零延迟、零假数据)
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        struct tm parts;
        localtime_r(&now_c, &parts); // POSIX 线程安全的本地时间获取

        char timeStr[16];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", parts.tm_hour, parts.tm_min, parts.tm_sec);
        char dateStr[32];
        strftime(dateStr, sizeof(dateStr), "%a, %b %d", &parts);

        // 居中巨型时钟 (Scale=9, 宽度约 576px, 起点 X=224)
        UIRenderer::drawText(ui, timeStr, 220, 60, {0, 255, 255}, 9);
        UIRenderer::drawText(ui, dateStr, 380, 180, {150, 220, 255}, 3);

        // 🌟 2. 预留给您的底层真实环境数据接口
//        float envTemp = 23.4f;      
        int envHum = 65;            
//        int weatherCode = 2;        // 0=晴天, 1=多云, 2=下雨
//        int cpuLoad = 32;           // CPU 负载
//        int memUsage = 100;          // 内存使用率

        // 🌟 真正对接到底层！从 currentEnv 提取实时硬件数据
        float envTemp = currentEnv.temperature;
//      int envHum = static_cast<int>(currentEnv.humidity);
        int weatherCode = currentEnv.weatherCode;
        int cpuLoad = currentEnv.cpuLoadPercent;
        int memUsage = currentEnv.memUsagePercent;

        // 渲染天气文本与真实图标
	std::string weatherStr = "UNKNOWN";
        switch(weatherCode) {
            case 0: weatherStr = "SUNNY"; break;
            case 1: weatherStr = "CLOUDY"; break;
            case 2: weatherStr = "RAINY"; break;
            case 3: weatherStr = "SNOWY"; break;
            case 4: weatherStr = "STORM"; break;
            case 5: weatherStr = "FOGGY"; break;
        }

        UIRenderer::drawText(ui, weatherStr, 85, 270, {0, 255, 255}, 4);
 
        char envStr[32];
        snprintf(envStr, sizeof(envStr), "%.1fC:%d%%", envTemp, envHum);
        UIRenderer::drawText(ui, envStr, 85, 340, {255, 255, 255}, 3);
        UIRenderer::renderWeatherIcon(ui, 370, 300, weatherCode);

        // 🌟 3. 渲染系统监控槽位
        UIRenderer::drawText(ui, std::to_string(cpuLoad) + "%", 380, 445, {0, 255, 255}, 3);

        UIRenderer::drawText(ui, std::to_string(memUsage) + "%", 380, 500, {0, 255, 255}, 3);
        
        // 调用专属的分段渐变内存条
        UIRenderer::renderStandbyMemoryBar(ui, 180, 505, 190, 16, memUsage);

        // 🌟 4. 驱动拓扑图的生命力：动态呼吸灯
        // 需依赖底层心跳定期调用 renderCurrentPage，否则 phase 不会增加
        static float breathPhase = 0.0f;
        breathPhase += 0.15f; 
        UIRenderer::renderTopologyBreathingLights(ui, 775, 370, breathPhase);

    }

    // ============== 👇 MUSIC_LIST 渲染层 (已冻结，绝不更改) 👇 ==============
    else if (page == UIPage::MUSIC_LIST) {
        ui->drawPreloadedBmp("MUSIC_LIST");

        UIRenderer::drawText(ui, "BACK", 65, 45, {255, 50, 50}, 4);

        int total = currentStatus.playlist.size();
        int curIdx = 0;
        for(size_t i = 0; i < currentStatus.playlist.size(); ++i) {
            if(currentStatus.playlist[i] == currentStatus.songName) { curIdx = i; break; }
        }

	char pageBuf[16];
	snprintf(pageBuf, sizeof(pageBuf), "%02d/%02d", curIdx + 1, total);
	UIRenderer::drawText(ui, pageBuf, 860, 55, {180, 255, 255}, 3);

        int songCount = currentStatus.playlist.size();
        int offset = scrollOffset.load();

        int clipTop = 110;
        int clipBot = 530;

        for(int i = 0; i < songCount; ++i) {
            int btnY = 85 * i + offset + 120;

            if (btnY + 60 < clipTop || btnY > clipBot) continue;

            bool isCur = (currentStatus.playlist[i] == currentStatus.songName);
            int btnX = 60, btnW = 904, btnH = 60;

            UIRenderer::renderListButton(ui, btnX, btnY, btnW, btnH, isCur, clipTop, clipBot);

            Color txtCol = isCur ? Color{0, 0, 0} : Color{0, 255, 255};

	    char prefixBuf[16];
            snprintf(prefixBuf, sizeof(prefixBuf), "%02d", i + 1);
            UIRenderer::drawListText(ui, prefixBuf, btnX + 30, btnY + 18, txtCol, 3, 0, 1024, clipTop, clipBot);

            int nameX = btnX + 110;
            int clipNameRight = btnX + btnW - 280;
            UIRenderer::drawListText(ui, currentStatus.playlist[i], nameX, btnY + 18, txtCol, 3, nameX, clipNameRight, clipTop, clipBot);

            char tailBuf[32];
            snprintf(tailBuf, sizeof(tailBuf), "[ID:%02dx] >", i + 1);
            int tailX = btnX + btnW - 260;
            UIRenderer::drawListText(ui, tailBuf, tailX, btnY + 18, txtCol, 3, 0, 1024, clipTop, clipBot);

        }
    }
    // ============== 👇 PLAYER 渲染层 (冻结隔离保护) 👇 ==============
    else if (page == UIPage::PLAYER) {
        ui->drawPreloadedBmp("PLAYER");

        // 🌟 1. 动态获取当前歌曲的准确 ID 编号
        int curIdx = 0;
        for(size_t i = 0; i < currentStatus.playlist.size(); ++i) {
            if(currentStatus.playlist[i] == currentStatus.songName) { curIdx = i; break; }
        }

	char idBuf[32];
    	snprintf(idBuf, sizeof(idBuf), "[ID:%02d]", curIdx + 1);

        // 🌟 2. 将 ID 绘制在背景图 "NOW PLAYING" 字符的正右边 (约 X=800, Y=40)
        UIRenderer::drawText(ui, idBuf, 800, 40, {0, 255, 255}, 3);

        // 🌟 3. 歌名向下移动 15 像素 (Y: 75 -> 90)，并且去除了前缀拼接
        int areaX = 235, areaY = 90;
        int clipRight = 974;
        int areaW = clipRight - areaX;
        std::string fullTitle = currentStatus.songName; // 纯净的歌名
        int textPixelW = fullTitle.length() * 32;
        Color cyberCyan = {0, 255, 255};

        if (textPixelW <= areaW) {
            UIRenderer::drawText(ui, fullTitle, areaX, areaY, cyberCyan, 4, areaX, clipRight);
        } else {
            g_songScrollX += 2.0f;
            if (g_songScrollX > textPixelW + 50) g_songScrollX = -areaW;
            UIRenderer::drawText(ui, fullTitle, areaX - (int)g_songScrollX, areaY, cyberCyan, 4, areaX, clipRight);
        }

        // ====== 以下渲染逻辑完全冻结 ======
        UIRenderer::renderMultiBarEQ(ui, currentVisual.overallIntensity, 512, 330, 115);

        float prog = (currentStatus.totalDuration > 0) ? (float)currentStatus.currentPosition / currentStatus.totalDuration : 0.0f;
        UIRenderer::renderProgressBar(ui, prog, 250, 525, 680, 16);

        UIRenderer::drawText(ui, "VOL " + std::to_string(currentStatus.volume) + "%", 65, 525, {0, 255, 255}, 2);

        for (const auto& btn : getActiveLayout()) {
            if (btn.id == "BACK") {
                UIRenderer::drawText(ui, btn.id, 70, 55, {255, 50, 50}, 4);
                continue;
            }

            bool isHighlighted = false;
            if (btn.id == "PLAY") {
                isHighlighted = currentStatus.isPlaying || (pressedBtnId == btn.id);
            } else {
                isHighlighted = (pressedBtnId == btn.id);
            }
            UIRenderer::renderButton(ui, btn, isHighlighted);

            int scale = (btn.id.length() <= 4) ? 3 : 2;
            int charW = (scale == 3) ? 24 : 16;
            int textW = btn.id.length() * charW;

            Color txtCol = {0, 220, 255};
            if (btn.id == "PLAY" || btn.id == "PAUSE") txtCol = {0, 255, 150};

            UIRenderer::drawText(ui, btn.id, btn.x + (btn.w - textW)/2, btn.y + 13, txtCol, scale);
        }
    }

    ui->flush();
    ui->unlock();
}

  // ============== handleTouch 触控交互逻辑 (冻结隔离保护) 👇 ==============
void InteractionManager::handleTouch(int x, int y) {
    UIPage page = currentPage.load();

    if (page == UIPage::STANDBY) {
        currentPage.store(UIPage::MUSIC_LIST);
        return;
    }

    if (page == UIPage::PLAYER) {
        if (y >= 505 && y <= 555 && x >= 245 && x <= 935) {
            if (commandEmitter) {
                float newProgress = static_cast<float>(x - 245) / 690.0f;
                commandEmitter({System::CommandType::SEEK_FORWARD, 0, newProgress});
            }
            return;
        }
    }

    for (const auto& btn : getActiveLayout()) {
        if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
            pressedBtnId = btn.id;
            pressedBtnTimer = 6;
            if (btn.action) btn.action();
            break;
        }
    }
}

  // ============== handleScroll h滑控交互逻辑 (冻结隔离保护) 👇 ==============
void InteractionManager::handleScroll(int deltaY) {
    if (currentPage.load() != UIPage::MUSIC_LIST) return;

    int newOffset = scrollOffset.load() + deltaY;
    if (newOffset > 0) newOffset = 0;
    int maxScroll = -std::max(0, static_cast<int>(currentStatus.playlist.size()) * 85 - 410);
    if (newOffset < maxScroll) newOffset = maxScroll;
    scrollOffset.store(newOffset);
}

} // namespace UI
