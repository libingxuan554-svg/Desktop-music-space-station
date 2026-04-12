/**
 * @file Flowing Lights.cpp
 * @brief Audio-reactive flowing lights visualiser for an SPI LED strip.
 *
 * This program reads 32-bit floating-point audio samples from stdin,
 * computes the peak amplitude over a configurable chunk, maps that peak
 * to a bar of active LEDs, and sends the resulting frame to an SPI LED strip.
 *
 * Design notes:
 * - SpiLedStrip is responsible only for SPI transport and LED frame encoding.
 * - FlowingLightsVisualizer is responsible only for audio chunk processing
 *   and colour mapping.
 * - The two classes communicate via a callback to keep responsibilities separated.
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <cerrno>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>

/**
 * @class SpiLedStrip
 * @brief Sends 24-bit GRB LED frames to an SPI LED strip device.
 *
 * The class opens and configures a Linux SPI device such as /dev/spidev0.0
 * and converts each 24-bit GRB LED value into the SPI bit pattern required
 * by the LED strip.
 */
class SpiLedStrip {
public:
    /**
     * @brief Construct an SPI LED strip transport.
     * @param devicePath Linux SPI device path.
     * @param spiSpeedHz SPI clock speed in Hz.
     */
    explicit SpiLedStrip(std::string devicePath = "/dev/spidev0.0",
                         uint32_t spiSpeedHz = 3200000)
        : devicePath_(std::move(devicePath)), spiSpeedHz_(spiSpeedHz) {}

    /**
     * @brief Close the SPI device if it is open.
     */
    ~SpiLedStrip() {
        closeDevice();
    }

    /**
     * @brief Open and configure the SPI device.
     * @return true if the SPI device was opened and configured successfully.
     */
    bool initialise() {
        closeDevice();

        fd_ = ::open(devicePath_.c_str(), O_RDWR);
        if (fd_ < 0) {
            std::cerr << "Failed to open SPI device " << devicePath_
                      << ": " << std::strerror(errno) << '\n';
            return false;
        }

        uint8_t mode = SPI_MODE_0;
        if (::ioctl(fd_, SPI_IOC_WR_MODE, &mode) < 0) {
            std::cerr << "Failed to set SPI mode: " << std::strerror(errno) << '\n';
            closeDevice();
            return false;
        }

        if (::ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &spiSpeedHz_) < 0) {
            std::cerr << "Failed to set SPI speed: " << std::strerror(errno) << '\n';
            closeDevice();
            return false;
        }

        return true;
    }

    /**
     * @brief Send one full LED frame to the strip.
     * @param colours 24-bit GRB values, one per LED.
     * @return true if the full frame was written successfully.
     *
     * Each input colour is encoded bit-by-bit into the LED strip timing format.
     * A reset tail is appended after the frame.
     */
    bool sendFrame(const std::vector<uint32_t>& colours) const {
        if (fd_ < 0) {
            std::cerr << "SPI device is not initialised.\n";
            return false;
        }

        std::vector<uint8_t> spiData;
        spiData.reserve(colours.size() * kBitsPerLed + kResetBytes);

        for (uint32_t grb : colours) {
            for (int bit = 23; bit >= 0; --bit) {
                spiData.push_back(((grb >> bit) & 1U) ? kSpiOne : kSpiZero);
            }
        }

        for (int i = 0; i < kResetBytes; ++i) {
            spiData.push_back(0x00);
        }

        const ssize_t written =
            ::write(fd_, spiData.data(), static_cast<size_t>(spiData.size()));

        if (written != static_cast<ssize_t>(spiData.size())) {
            std::cerr << "Incomplete SPI write.\n";
            return false;
        }

        return true;
    }

private:
    void closeDevice() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

private:
    static constexpr uint8_t kSpiOne = 0xFE;
    static constexpr uint8_t kSpiZero = 0x80;
    static constexpr int kBitsPerLed = 24;
    static constexpr int kResetBytes = 500;

    int fd_ = -1;
    std::string devicePath_;
    uint32_t spiSpeedHz_ = 3200000;
};

/**
 * @class FlowingLightsVisualizer
 * @brief Converts incoming audio samples into LED frames.
 *
 * Audio samples are collected into chunks. For each chunk, the maximum absolute
 * amplitude is computed and scaled into the number of LEDs to turn on.
 * The generated LED frame is published through a callback.
 */
class FlowingLightsVisualizer {
public:
    /**
     * @brief Callback type for delivering a rendered LED frame.
     */
    using FrameCallback = std::function<void(const std::vector<uint32_t>&)>;

    /**
     * @brief Construct the visualiser.
     * @param ledCount Number of LEDs in the strip.
     * @param chunkSize Number of audio samples processed per update.
     * @param sensitivity Scaling factor from peak amplitude to active LED count.
     */
    explicit FlowingLightsVisualizer(std::size_t ledCount = 60,
                                     std::size_t chunkSize = 1024,
                                     float sensitivity = 200.0f)
        : ledCount_(ledCount),
          chunkSize_(chunkSize),
          sensitivity_(sensitivity) {
        sampleBuffer_.reserve(chunkSize_);
    }

    /**
     * @brief Register a callback that receives each rendered LED frame.
     * @param callback Function called whenever a new frame is available.
     */
    void registerFrameCallback(FrameCallback callback) {
        frameCallback_ = std::move(callback);
    }

    /**
     * @brief Run the visualiser on a blocking stream of float audio samples.
     * @param input Input stream containing 32-bit float samples.
     *
     * The stream is read in blocking mode. Every time enough samples have been
     * collected for one chunk, a new LED frame is rendered and published.
     */
    void run(std::istream& input) {
        float sample = 0.0f;

        while (input.read(reinterpret_cast<char*>(&sample), sizeof(sample))) {
            sampleBuffer_.push_back(std::abs(sample));

            if (sampleBuffer_.size() >= chunkSize_) {
                processChunk();
            }
        }
    }

private:
    void processChunk() {
        if (sampleBuffer_.empty()) {
            return;
        }

        const float maxAmplitude =
            *std::max_element(sampleBuffer_.begin(), sampleBuffer_.end());

        sampleBuffer_.clear();

        int numOn = static_cast<int>(maxAmplitude * static_cast<float>(ledCount_) * sensitivity_);
        numOn = std::clamp(numOn, 0, static_cast<int>(ledCount_));

        std::vector<uint32_t> frame(ledCount_, kOff);

        for (int i = 0; i < numOn; ++i) {
            frame[static_cast<std::size_t>(i)] = colourForIndex(static_cast<std::size_t>(i));
        }

        if (frameCallback_) {
            frameCallback_(frame);
        }
    }

    uint32_t colourForIndex(std::size_t index) const {
        const std::size_t firstBandEnd = ledCount_ / 3;
        const std::size_t secondBandEnd = (ledCount_ * 2) / 3;

        if (index < firstBandEnd) {
            return kGreenGrb;
        }
        if (index < secondBandEnd) {
            return kYellowGrb;
        }
        return kRedGrb;
    }

private:
    static constexpr uint32_t kOff = 0x000000;
    static constexpr uint32_t kGreenGrb = 0x440000;
    static constexpr uint32_t kYellowGrb = 0x444400;
    static constexpr uint32_t kRedGrb = 0x004400;

    std::size_t ledCount_;
    std::size_t chunkSize_;
    float sensitivity_;
    std::vector<float> sampleBuffer_;
    FrameCallback frameCallback_;
};

int main() {
    SpiLedStrip ledStrip("/dev/spidev0.0", 3200000);
    if (!ledStrip.initialise()) {
        return 1;
    }

    FlowingLightsVisualizer visualizer(60, 1024, 200.0f);

    visualizer.registerFrameCallback(
        [&ledStrip](const std::vector<uint32_t>& frame) {
            if (!ledStrip.sendFrame(frame)) {
                std::cerr << "Failed to send LED frame.\n";
            }
        });

    std::cout << "Flowing lights visualiser started. Waiting for audio samples on stdin...\n";
    visualizer.run(std::cin);

    return 0;
}
