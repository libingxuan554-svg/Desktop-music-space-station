#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <cstdint>
#include <cmath>

#define LED_COUNT 60

void send_leds(int fd, const std::vector<uint32_t>& colors) {
    std::vector<uint8_t> spi_data;
    for (uint32_t grb : colors) {
        for (int i = 23; i >= 0; i--) {
            spi_data.push_back(((grb >> i) & 1) ? 0xFE : 0x80);
        }
    }
    for(int i = 0; i < 500; i++) spi_data.push_back(0x00);
    write(fd, spi_data.data(), spi_data.size());
}

int main() {
    int fd = open("/dev/spidev0.0", O_RDWR);
    if (fd < 0) return 1;

    uint8_t mode = SPI_MODE_0;
    ioctl(fd, SPI_IOC_WR_MODE, &mode);
    uint32_t speed = 3200000;
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    std::cout << ">>> 可视化引擎启动：正在监听系统音频..." << std::endl;

    float sample;
    std::vector<float> buffer;
    const int chunk_size = 1024; // 每次处理一小块音频

    // 从标准输入(stdin)读取原始浮点音频数据
    while (std::cin.read(reinterpret_cast<char*>(&sample), sizeof(float))) {
        buffer.push_back(std::abs(sample));
        
        if (buffer.size() >= chunk_size) {
            float max_amp = 0;
            for (float f : buffer) if (f > max_amp) max_amp = f;
            buffer.clear();

            // 计算亮度：振幅映射到灯数
            // 200.0f 是灵敏度，根据需要调整
            int num_on = (int)(max_amp * LED_COUNT * 200.0f);
            if (num_on > LED_COUNT) num_on = LED_COUNT;

            std::vector<uint32_t> leds(LED_COUNT, 0);
            for (int i = 0; i < num_on; i++) {
                if (i < 20) leds[i] = 0x440000;      // 绿 (GRB)
                else if (i < 40) leds[i] = 0x444400; // 黄
                else leds[i] = 0x004400;             // 红
            }
            send_leds(fd, leds);
        }
    }
    
    close(fd);
    return 0;
}
