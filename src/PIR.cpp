#include "PIR.hpp"
#include <gpiod.h>
#include <chrono>
#include <iostream>

bool PIR::start(const std::string& chipName, int line, Callback onMotion)
{
    running.store(true);

    th = std::thread([=]() mutable {
        gpiod_chip* chip = gpiod_chip_open_by_name(chipName.c_str());
        if (!chip) { std::cerr << "gpiod: open chip failed\n"; return; }

        gpiod_line* ln = gpiod_chip_get_line(chip, line);
        if (!ln) { std::cerr << "gpiod: get line failed\n"; gpiod_chip_close(chip); return; }

        if (gpiod_line_request_rising_edge_events(ln, "pir") < 0) {
            std::cerr << "gpiod: request rising edge failed\n";
            gpiod_chip_close(chip);
            return;
        }

        while (running.load()) {
            timespec ts{1, 0}; // 1s timeout
            int ret = gpiod_line_event_wait(ln, &ts);
            if (ret < 0) continue;
            if (ret == 0) continue;

            gpiod_line_event ev{};
            if (gpiod_line_event_read(ln, &ev) == 0) {
                if (ev.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
                    if (onMotion) onMotion();
                }
            }
        }

        gpiod_line_release(ln);
        gpiod_chip_close(chip);
    });

    return true;
}

void PIR::stop()
{
    running.store(false);
    if (th.joinable()) th.join();
}
