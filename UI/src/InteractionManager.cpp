#include "InteractionManager.hpp" // Include interaction manager header
#include <iostream>               // Include standard input/output stream
#include <algorithm>              // Include standard algorithm library
#include <chrono>    	          // Include high-resolution time library for system clock
#include <ctime>                  // Include C-style time conversion library

// Define UI namespace
namespace UI {

// // [REAL-TIME COMPLIANCE]:
// Static member initialization. Using std::atomic to guarantee lock-free data sharing across threads, completely avoiding mutex overhead.
std::atomic<UIPage> InteractionManager::currentPage{UIPage::STANDBY}; // Initialize current page to standby page
std::atomic<int> InteractionManager::scrollOffset{0};                 // Initialize scroll offset to 0
InteractionManager::CommandEmitter InteractionManager::commandEmitter = nullptr; // Initialize command emitter to null

System::PlaybackStatus InteractionManager::currentStatus;   // Global cache for music playback status
System::AudioVisualData InteractionManager::currentVisual;  // Global cache for spectrum visual data
System::EnvironmentStatus InteractionManager::currentEnv;   // Global cache for physical environment sensor data

static int volumeDisplayTimer = 0; // Countdown timer for volume display box
static float g_songScrollX = 0;    // Horizontal scroll offset for overly long song titles in player UI

static std::string pressedBtnId = ""; // Unique identifier of the currently pressed button
static int pressedBtnTimer = 0;       // Countdown timer for button pressed highlight state frames

/**
 * @brief Bind the low-level control command emission callback
 * @param[in] emitter  The callback function passed in
 */
void InteractionManager::setCommandEmitter(CommandEmitter emitter) {
    commandEmitter = emitter;		 // Assign the low-level communication interface to the internal pointer
}

/**
 * @brief  Update the state data of the music subsystem
 * @param[in] status  Playback status
 * @param[in] visual  Spectrum visual data
 */
void InteractionManager::updateSystemStatus(const System::PlaybackStatus& status, const System::AudioVisualData& visual) {
    if (currentStatus.songName != status.songName) { // Reset song title scrolling when a track change is detected
        g_songScrollX = 0; 							 //  Reset X-axis offset
    }
    currentStatus = status; 						 //  Overwrite the latest playback status
    currentVisual = visual; 						 //  Overwrite the latest spectrum data
}

/**
 * @brief  Update physical environment sensory data
 * @param[in] env  Environment sensor status
 */
void InteractionManager::updateEnvStatus(const System::EnvironmentStatus& env) {
    currentEnv = env; 			// Overwrite the latest physical data dictionary
}

/**
 * @brief  Dynamically build and retrieve the active button layout matrix for the current page
 * @return  Array of buttons containing all valid hot zones
 */
std::vector<UIButton> InteractionManager::getActiveLayout() {
    std::vector<UIButton> layout; 		// Create temporary button container
    UIPage page = currentPage.load();   // Lock-free reading of the current page

// ============== Touch layer layout ==============
// If in the music list page
if (page == UIPage::MUSIC_LIST) {

        // When backing off STANDBY from LIST, sent a standby instruction to the bottom layer
        layout.push_back({45, 25, 130, 50, "BACK", {0,0,0}, [](){ 
			//  Inject button to return to the main page
            if(commandEmitter) commandEmitter({System::CommandType::ENTER_STANDBY, 0, 0.0f}); 
			//  Trigger system-level standby sleep command
            currentPage.store(UIPage::STANDBY);
			//  Atomic switch of page state
        }});

        int offset = scrollOffset.load(); 					// Get current list scroll offset
        int songCount = currentStatus.playlist.size(); 		// Get total song count
        for(int i = 0; i < songCount; ++i) {				// Iterate to generate list buttons
            int btnY = 85 * i + offset + 120; 				// Calculate Y-axis physical coordinate based on index and offset
            if (btnY < -60 || btnY > 600) continue; 		// Viewport clipping: directly discard invisible elements to save massive overhead
            layout.push_back({60, btnY, 904, 60, currentStatus.playlist[i], {0, 0, 0}, [i](){ 		// Inject closure callback for song selection
                if(commandEmitter) commandEmitter({System::CommandType::SELECT_SONG, i, 0.0f});		// Dispatch song selection command to low-level
                currentPage.store(UIPage::PLAYER); 			// Automatically jump to player page
                g_songScrollX = 0;							// Initialize scroll axis
            }});
        }
    }
    else if (page == UIPage::PLAYER) {
	// If in the music player page
        layout.push_back({55, 45, 130, 50, "BACK", {0,0,0}, [](){ currentPage.store(UIPage::MUSIC_LIST); }});
		// Return to list button
        int btnW = 140, spacing = 28, startX = 106, yPos = 410; // Define bottom control panel parameter matrix
        layout.push_back({startX, yPos, btnW, 50, "VOL-", {0,0,0}, [](){ if(commandEmitter) commandEmitter({System::CommandType::VOLUME_DOWN}); volumeDisplayTimer = 45; }}); 	//  Volume down button
        layout.push_back({startX + (btnW + spacing), yPos, btnW, 50, "PREV", {0,0,0}, [](){ if(commandEmitter) commandEmitter({System::CommandType::PREV_TRACK}); }}); 			//  Previous track button
        layout.push_back({startX + (btnW + spacing)*2, yPos, btnW, 50, "PLAY", {0,0,0}, [](){ if(commandEmitter) commandEmitter({System::CommandType::PLAY_PAUSE}); }}); 		//  Play/Pause button
        layout.push_back({startX + (btnW + spacing)*3, yPos, btnW, 50, "NEXT", {0,0,0}, [](){ if(commandEmitter) commandEmitter({System::CommandType::NEXT_TRACK}); }});		//  Next track button
        layout.push_back({startX + (btnW + spacing)*4, yPos, btnW, 50, "VOL+", {0,0,0}, [](){ if(commandEmitter) commandEmitter({System::CommandType::VOLUME_UP}); volumeDisplayTimer = 45; }}); // Volume up button
    }
    return layout;  // Return computed hot zone matrix
}

/**
 * @brief  Execute the overall rendering pipeline for the current page
 * @param[in] Pointer to the Framebuffer driver instance
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * // This function runs in a dedicated UI thread, internally holding a mutex to protect double-buffer writes.
 * // All pixel rendering time complexities are constant, meeting the 33.3ms (30FPS) real-time budget requirement.
 */
void InteractionManager::renderCurrentPage(FramebufferUI* ui) {
    ui->lock(); // Lock VRAM mutex to prevent screen tearing

    if (pressedBtnTimer > 0) { 	// Check click highlight countdown
        pressedBtnTimer--; 		// Consume countdown frames
    } else {
        pressedBtnId = ""; 		// Countdown finished, clear button highlight ID
    }

    UIPage page = currentPage.load(); // Lock-free retrieval of current route page

	// ============== STANDBY rendering layer ==============
    if (page == UIPage::STANDBY) {
        ui->drawPreloadedBmp("STANDBY"); //Instantly copy pre-decompressed geek background image

        // 1. Get and render real system time
        auto now = std::chrono::system_clock::now(); //Get current high-resolution timestamp
        std::time_t now_c = std::chrono::system_clock::to_time_t(now); //Convert to C standard timestamp
        struct tm parts; //Define time structure
        localtime_r(&now_c, &parts); //POSIX thread-safe local time retrieval

        char timeStr[16]; //Character array for formatted time
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", parts.tm_hour, parts.tm_min, parts.tm_sec); //Format as HH:MM:SS
        char dateStr[32]; //Character array for formatted date
        strftime(dateStr, sizeof(dateStr), "%a, %b %d", &parts); //Format as weekday and date

        // Centered giant clock
        UIRenderer::drawText(ui, timeStr, 220, 60, {0, 255, 255}, 9); // Draw main time
        UIRenderer::drawText(ui, dateStr, 380, 180, {150, 220, 255}, 3); // Draw sub-date

        // 2. Interface for real environment data
        // Extract real-time hardware data from currentEnv
        int envHum = 65;
		float envTemp = currentEnv.temperature; 	// Inject real physical room temperature
        int weatherCode = currentEnv.weatherCode; 	// Inject real network weather code
        int cpuLoad = currentEnv.cpuLoadPercent; 	// Inject real CPU load percentage
        int memUsage = currentEnv.memUsagePercent; 	// Inject real memory usage percentage

        // Render weather text and real icons
	std::string weatherStr = "UNKNOWN"; 	// Default status is unknown
        switch(weatherCode) { 				// Convert enumeration code to English string
            case 0: weatherStr = "SUNNY"; break;
            case 1: weatherStr = "CLOUDY"; break;
            case 2: weatherStr = "RAINY"; break;
            case 3: weatherStr = "SNOWY"; break;
            case 4: weatherStr = "STORM"; break;
            case 5: weatherStr = "FOGGY"; break;
        }

        UIRenderer::drawText(ui, weatherStr, 85, 270, {0, 255, 255}, 4); //Draw weather English word
 
        char envStr[32]; //Environment info combination buffer
        snprintf(envStr, sizeof(envStr), "%.1fC:%d%%", envTemp, envHum); //Combine temperature and humidity
        UIRenderer::drawText(ui, envStr, 85, 340, {255, 255, 255}, 3); 	//Draw combined environment info
        UIRenderer::renderWeatherIcon(ui, 370, 300, weatherCode); 		//Generate pixel weather illustration based on code block

        // 3. Render system monitoring slots
        UIRenderer::drawText(ui, std::to_string(cpuLoad) + "%", 380, 445, {0, 255, 255}, 3); // Draw CPU value
        UIRenderer::drawText(ui, std::to_string(memUsage) + "%", 380, 500, {0, 255, 255}, 3); // Draw memory value
        UIRenderer::renderStandbyMemoryBar(ui, 180, 505, 190, 16, memUsage); // Map data to warning bar UI

        // 4. Drive the vitality of topology diagram: dynamic breathing light
        // Depends on low-level heartbeat calling renderCurrentPage periodically, otherwise phase will not increase
        static float breathPhase = 0.0f; // Static accumulator maintaining wave phase
        breathPhase += 0.15f; 			// Update phase with tiny steps to create breathing feel
        UIRenderer::renderTopologyBreathingLights(ui, 775, 370, breathPhase); 
		// Push phase and trigger network node lighting render
    }

    // ============== MUSIC_LIST Rendering Layer ==============
    else if (page == UIPage::MUSIC_LIST) {
        ui->drawPreloadedBmp("MUSIC_LIST"); //Instantly copy background

        UIRenderer::drawText(ui, "BACK", 65, 45, {255, 50, 50}, 4); //Draw back button characters

        int total = currentStatus.playlist.size(); //Get playlist library size
        int curIdx = 0; //Index value record of currently playing song
        for(size_t i = 0; i < currentStatus.playlist.size(); ++i) { //Iterate to find the filename currently playing
            if(currentStatus.playlist[i] == currentStatus.songName) { curIdx = i; break; } //If matched, break the loop
        }

	char pageBuf[16]; //Pagination system page cache
	snprintf(pageBuf, sizeof(pageBuf), "%02d/%02d", curIdx + 1, total); //Format as 01/15 style
	UIRenderer::drawText(ui, pageBuf, 860, 55, {180, 255, 255}, 3); //Draw total track info at top right

        int songCount = currentStatus.playlist.size(); //Cache list length for loop
        int offset = scrollOffset.load(); 			   //Lock-free reading of user finger swipe amount

        int clipTop = 110; // Define top Y boundary of list physical viewport
        int clipBot = 530; // Define bottom Y boundary of list physical viewport

         // Start main list rendering loop
		for(int i = 0; i < songCount; ++i) {
            int btnY = 85 * i + offset + 120; 
            if (btnY + 60 < clipTop || btnY > clipBot) continue; //If the whole item exceeds viewport, skip all calculations!

            bool isCur = (currentStatus.playlist[i] == currentStatus.songName); //Determine if this item is the selected music
            int btnX = 60, btnW = 904, btnH = 60; //Define physical geometric parameters for each list item

            UIRenderer::renderListButton(ui, btnX, btnY, btnW, btnH, isCur, clipTop, clipBot); //Call drawing of clipped cyber frame with Y-axis viewport protection
            Color txtCol = isCur ? Color{0, 0, 0} : Color{0, 255, 255}; //Selected state is black text, normal state is cyber cyan

	    char prefixBuf[16]; // Serial number cache
            snprintf(prefixBuf, sizeof(prefixBuf), "%02d", i + 1); // Format 01 02 serial number
            UIRenderer::drawListText(ui, prefixBuf, btnX + 30, btnY + 18, txtCol, 3, 0, 1024, clipTop, clipBot); 
			//Render left serial number within restricted frame

            int nameX = btnX + 110; // Calculate song title X-axis start point
            int clipNameRight = btnX + btnW - 280; // Strictly stipulate right cut-off boundary for song title, preventing overlap with tail text
            UIRenderer::drawListText(ui, currentStatus.playlist[i], nameX, btnY + 18, txtCol, 3, nameX, clipNameRight, clipTop, clipBot); 
			// Perform song title rendering with bi-directional X Y axis protection

            char tailBuf[32]; // Tail info cache
            snprintf(tailBuf, sizeof(tailBuf), "[ID:%02dx] >", i + 1); // Build cool characters like [ID:01x] >
            int tailX = btnX + btnW - 260; // Calculate tail right start point
            UIRenderer::drawListText(ui, tailBuf, tailX, btnY + 18, txtCol, 3, 0, 1024, clipTop, clipBot); // Render tail armor decorative text

        }
    }
    // ============== PLAYER Rendering Layer ==============
    else if (page == UIPage::PLAYER) {
        ui->drawPreloadedBmp("PLAYER"); // O(N) instant copy of low-level background template

        // 1. Dynamically get accurate ID number of current song
        int curIdx = 0; //Current index initialization
        for(size_t i = 0; i < currentStatus.playlist.size(); ++i) { //Iterate to search
            if(currentStatus.playlist[i] == currentStatus.songName) { curIdx = i; break; } //Stop immediately on hit
        }

	char idBuf[32]; // ID cache character array
    	snprintf(idBuf, sizeof(idBuf), "[ID:%02d]", curIdx + 1); // Format ID as two digits

        // 2. Draw ID right next to "NOW PLAYING" (Approx X=800, Y=40)
        UIRenderer::drawText(ui, idBuf, 800, 40, {0, 255, 255}, 3); // Call low-level character generator to print pure cyan dot matrix

        // 3. Move song title down by 15 pixels, removed prefix concatenation
        int areaX = 235, areaY = 90; 	// Define marquee area top-left start point
        int clipRight = 974; 			// Define absolute right boundary of marquee visible area
        int areaW = clipRight - areaX;  // Calculate physical width of scrolling mask
        std::string fullTitle = currentStatus.songName; // Pure song title
        int textPixelW = fullTitle.length() * 32; 		// (Scale=4 means 32px per char) Calculate total horizontal physical size of text
        Color cyberCyan = {0, 255, 255}; 				// Define default cyan

        if (textPixelW <= areaW) { 	//If song title is very short, no need to scroll
            UIRenderer::drawText(ui, fullTitle, areaX, areaY, cyberCyan, 4, areaX, clipRight); // Direct left-aligned static printing
        } else { 					//Otherwise song title is too long, start low-level marquee scroll calculation
            g_songScrollX += 2.0f; 	//Smoothly advance offset by 2 pixels per frame
            if (g_songScrollX > textPixelW + 50) g_songScrollX = -areaW; //Once tail scrolls off screen, reset coordinate for loop
            UIRenderer::drawText(ui, fullTitle, areaX - (int)g_songScrollX, areaY, cyberCyan, 4, areaX, clipRight);
			//Combine negative offset with low-level boundary clip function for silky mask
        }

        // 4.Dynamic music spectrum
        UIRenderer::renderMultiBarEQ(ui, currentVisual.overallIntensity, 512, 330, 115); 
		// Push overall intensity resolved by audio engine to bar analyzer

        float prog = (currentStatus.totalDuration > 0) ? (float)currentStatus.currentPosition / currentStatus.totalDuration : 0.0f; // Avoid division by zero when calculating playback percentage
        UIRenderer::renderProgressBar(ui, prog, 250, 525, 680, 16); 
		// Draw slender cyber timeline fill

        UIRenderer::drawText(ui, "VOL " + std::to_string(currentStatus.volume) + "%", 65, 525, {0, 255, 255}, 2); 
		// Print hardware real volume from audio controller at bottom left
        
		// 5.Button in player page
        for (const auto& btn : getActiveLayout()) { // Retrieve and iterate over all interactive button coordinate sets of this page
            if (btn.id == "BACK") { 				// Filter special logic for back button
                UIRenderer::drawText(ui, btn.id, 70, 55, {255, 50, 50}, 4); // Render with warning red
                continue; 							// Skip subsequent common drawing
            }

            bool isHighlighted = false; // Button highlight state flag
            if (btn.id == "PLAY") { // If it's the play button
                isHighlighted = currentStatus.isPlaying || (pressedBtnId == btn.id); 
				// Always light up if music is playing or just pressed
            } else {
                isHighlighted = (pressedBtnId == btn.id); 
				// Other regular function keys only light up for a few frames while pressed
            }
            UIRenderer::renderButton(ui, btn, isHighlighted); 
			// Call specialized button frame function with 4-corner highlighter

            int scale = (btn.id.length() <= 4) ? 3 : 2; // Smart typography: large font for few words, small for many
            int charW = (scale == 3) ? 24 : 16; 		//Font physical pixel calculation constant
            int textW = btn.id.length() * charW; 		//Calculate total physical length of this text

            Color txtCol = {0, 220, 255}; 				// Default control panel text color
            if (btn.id == "PLAY" || btn.id == "PAUSE") txtCol = {0, 255, 150}; // Core driver buttons use more vivid emerald green

            UIRenderer::drawText(ui, btn.id, btn.x + (btn.w - textW)/2, btn.y + 13, txtCol, scale); 
			//Calculate centered coordinate and perform dot matrix mapping
        }
    }

    ui->flush(); // [Core Syscall] Push the assembled double-buffer data to Linux physical VRAM at extreme speed, completing frame presentation!
    ui->unlock(); // Release frame lock, allowing other threads to intervene
}

// ============== handleTouch - Touch Interaction Logic ==============
/**
 * @brief  Parse and process absolute coordinate touch down events from hardware
 * @param[in] x  Touch point X coordinate
 * @param[in] y  Touch point Y coordinate
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * // Trigger process is completely non-blocking, finding and executing bound lambda closures within O(N), instantly yielding thread control.
 */
void InteractionManager::handleTouch(int x, int y) {
    UIPage page = currentPage.load(); // Lock-free reading of current state, determine event routing flow

    if (page == UIPage::STANDBY) { 	// If at standby barrier
        currentPage.store(UIPage::MUSIC_LIST); // Directly break barrier and enter list routing upon any touch signal
        return; 					//Exit immediately
    }

    if (page == UIPage::PLAYER) { 			// Exclusive fast forward/rewind special slide interception area for progress bar
        if (y >= 505 && y <= 555 && x >= 245 && x <= 935) { // Frame a huge fault-tolerant rectangle allowing rough finger taps
            if (commandEmitter) { 			// Ensure low-level connection is alive
                float newProgress = static_cast<float>(x - 245) / 690.0f; 				// Perform precise mathematical conversion from physical distance to song time percentage
                commandEmitter({System::CommandType::SEEK_FORWARD, 0, newProgress}); 	// Wrap as system-level command and push into communication mailbox
            }
            return; // Process complete, block subsequent logic
        }
    }

    for (const auto& btn : getActiveLayout()) { // General button collision detection algorithm
        if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) { // Check if point is inside AABB bounding box
            pressedBtnId = btn.id; // Hit! Record button name immediately to trigger visual feedback
            pressedBtnTimer = 6;   // Give this button a highlight lifespan of 6 frames
            if (btn.action) btn.action(); // Wake up and safely execute bound lock-free closure callback in current thread
            break; 				   // Only one area allowed per click, saving redundant computation
        }
    }
}

// ============== handleScroll - Scroll Interaction Logic ==============
/**
 * @brief Parse and process relative Y-axis slide events from hardware
 * @param[in] deltaY  Y-axis physical slide relative difference
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * // Boundary calculation based on pure atomic variables. Prevents out-of-bounds crashes when sliding generates huge displacement signals.
 */
void InteractionManager::handleScroll(int deltaY) {
    if (currentPage.load() != UIPage::MUSIC_LIST) return; // Sliding calculation only allowed on pages with long lists

    int newOffset = scrollOffset.load() + deltaY; 	//Read current baseline and superimpose latest difference
    if (newOffset > 0) newOffset = 0;				//Top rubber-band defense wall: absolutely no exceeding track #0!
    int maxScroll = -std::max(0, static_cast<int>(currentStatus.playlist.size()) * 85 - 410); 
	//Deduce the absolute bottom deadline based on real dynamic list length and physical spacing
    if (newOffset < maxScroll) newOffset = maxScroll; 	//Bottom rubber-band defense wall: intercept excessive downward swipes
    scrollOffset.store(newOffset); 						//Re-inject sanitized safe deviation value into atomic center for safe reading next frame
}

} // namespace UI
