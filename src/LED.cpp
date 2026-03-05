#include "LED.hpp"
#include <ws2811.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>

static ws2811_t ledstring;

bool LED::start(int gpioPin, int ledCount)
{
    gpio = gpioPin;
    count = ledCount;

    std::memset(&ledstring, 0, sizeof(ledstring));
    ledstring.freq = WS2811_TARGET_FREQ;
    ledstring.dmanum = 10;

    ledstring.channel[0].gpionum = gpio;
    ledstring.channel[0].count = count;
    ledstring.channel[0].invert = 0;
    ledstring.channel[0].brightness = 255;
    ledstring.channel[0].strip_type = WS2811_STRIP_GRB;

    if (ws2811_init(&ledstring) != WS2811_SUCCESS) {
        std::cerr << "ws2811_init failed\n";
        return false;
    }

    running.store(true);
    th = std::thread([&](){
        float phase = 0.0f;
        while (running.load()) {
            float lv = level.load();
            // 呼吸 + 音量叠加
            phase += 0.08f;
            float breathe = 0.5f + 0.5f * std::sin(phase);
            float intensity = std::min(1.0f, 0.15f + 0.85f * (0.6f*breathe + 0.9f*lv));

            // 单色（白）示例：你可改成彩虹/频谱
            int val = (int)(255.0f * intensity);
            ws2811_led_t color = ((val & 0xFF) << 16) | ((val & 0xFF) << 8) | (val & 0xFF);

            for (int i = 0; i < count; ++i) {
                ledstring.channel[0].leds[i] = color;
            }

            ws2811_render(&ledstring);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }

        // 关灯
        for (int i = 0; i < count; ++i) ledstring.channel[0].leds[i] = 0;
        ws2811_render(&ledstring);

        ws2811_fini(&ledstring);
    });

    return true;
}

void LED::stop()
{
    running.store(false);
    if (th.joinable()) th.join();
}

void LED::setLevel(float v)
{
    if (v < 0) v = 0;
    if (v > 1) v = 1;
    level.store(v);
}
