#pragma once
#include <vector>
#include <atomic>
#include <span>
#include <algorithm>

namespace player {

template <typename T>
class AudioRingBuffer {
public:
    explicit AudioRingBuffer(size_t capacity) 
        : m_buffer(capacity), m_capacity(capacity) {}

    size_t write(std::span<const T> data) {
        size_t head = m_head.load(std::memory_order_relaxed);
        size_t tail = m_tail.load(std::memory_order_acquire);

        size_t available = m_capacity - (head - tail);
        size_t to_write = std::min(data.size(), available);

        for (size_t i = 0; i < to_write; ++i) {
            m_buffer[(head + i) % m_capacity] = data[i];
        }

        m_head.store(head + to_write, std::memory_order_release);
        return to_write;
    }

    size_t read(std::span<T> out_data) {
        size_t head = m_head.load(std::memory_order_acquire);
        size_t tail = m_tail.load(std::memory_order_relaxed);

        size_t available = head - tail;
        size_t to_read = std::min(out_data.size(), available);

        for (size_t i = 0; i < to_read; ++i) {
            out_data[i] = m_buffer[(tail + i) % m_capacity];
        }

        m_tail.store(tail + to_read, std::memory_order_release);
        return to_read;
    }

    size_t size() const {
        size_t head = m_head.load(std::memory_order_relaxed);
        size_t tail = m_tail.load(std::memory_order_relaxed);
        return head - tail;
    }

    void clear() {
        m_head.store(0, std::memory_order_relaxed);
        m_tail.store(0, std::memory_order_relaxed);
    }

private:
    std::vector<T> m_buffer;
    size_t m_capacity;
    alignas(64) std::atomic<size_t> m_head{0};
    alignas(64) std::atomic<size_t> m_tail{0};
};

}