#pragma once
#include <vector>
#include <mutex>
#include <cstddef>

class RingBuffer {
public:
    explicit RingBuffer(size_t capacity = 0) {
        resize(capacity);
    }

    void resize(size_t capacity) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buffer.assign(capacity, 0.0f);
        m_capacity = capacity;
        m_head = 0;
        m_size = 0;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_head = 0;
        m_size = 0;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size;
    }

    size_t capacity() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_capacity;
    }

    void push(const float* data, size_t count) {
        if (data == nullptr || count == 0 || m_capacity == 0) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        // 如果一次写入的数据比容量还大，只保留最后 capacity 个样本
        if (count >= m_capacity) {
            data += (count - m_capacity);
            count = m_capacity;
            m_head = 0;
            m_size = 0;
        }

        for (size_t i = 0; i < count; ++i) {
            m_buffer[m_head] = data[i];
            m_head = (m_head + 1) % m_capacity;

            if (m_size < m_capacity) {
                ++m_size;
            }
        }
    }

    bool readLatest(float* out, size_t count) const {
        if (out == nullptr || count == 0) {
            return false;
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        if (count > m_size) {
            return false;  // 数据还不够
        }

        size_t start = (m_head + m_capacity - count) % m_capacity;

        for (size_t i = 0; i < count; ++i) {
            out[i] = m_buffer[(start + i) % m_capacity];
        }

        return true;
    }

    bool readLatest(std::vector<float>& out, size_t count) const {
        out.resize(count);
        return readLatest(out.data(), count);
    }

private:
    mutable std::mutex m_mutex;
    std::vector<float> m_buffer;
    size_t m_capacity = 0;
    size_t m_head = 0;   // 下一个写入位置
    size_t m_size = 0;   // 当前有效数据量
};
