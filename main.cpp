#include "HardwareController.hpp"
#include <vector>
#include <unistd.h>

int main() {
    HardwareController hw;

    if (!hw.initialize()) return 1;
    if (!hw.initLedStrip()) return 1;

    std::vector<float> low = {1.0f, 0.8f, 0.4f, 0.2f, 0.1f, 0.05f, 0.02f, 0.01f};
    std::vector<float> mid = {0.1f, 0.2f, 0.5f, 1.0f, 0.9f, 0.4f, 0.1f, 0.05f};
    std::vector<float> high = {0.01f, 0.02f, 0.05f, 0.1f, 0.3f, 0.6f, 1.0f, 0.9f};

    while (true) {
        hw.updateLighting(low);
        sleep(1);

        hw.updateLighting(mid);
        sleep(1);

        hw.updateLighting(high);
        sleep(1);
    }

    hw.shutdown();
    return 0;
}
