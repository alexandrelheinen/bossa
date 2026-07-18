/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/telemetry/ring_buffer.hpp"

#include <cstdint>
#include <stdexcept>
#include <syslog.h>

namespace bossa::telemetry {

namespace {

void remove_at_offset(std::vector<StoredSample> &slots, std::size_t capacity,
                      std::size_t head, std::size_t &size, std::size_t offset) {
    for (std::size_t i = offset; i + 1 < size; ++i) {
        slots[(head + i) % capacity] = slots[(head + i + 1) % capacity];
    }
    --size;
}

} // namespace

RingBuffer::RingBuffer(std::size_t capacity) : capacity_(capacity) {
    if (capacity_ == 0) {
        throw std::invalid_argument("RingBuffer capacity must be > 0");
    }
    slots_.resize(capacity_);
}

bool RingBuffer::enqueue(const StoredSample &sample) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (size_ < capacity_) {
        slots_[(head_ + size_) % capacity_] = sample;
        ++size_;
        return true;
    }

    // Full: find lowest-priority resident (oldest among ties).
    std::size_t drop_offset = 0;
    Priority drop_priority = slots_[head_].priority;
    for (std::size_t i = 1; i < size_; ++i) {
        const Priority priority = slots_[(head_ + i) % capacity_].priority;
        if (static_cast<std::uint8_t>(priority) <
            static_cast<std::uint8_t>(drop_priority)) {
            drop_offset = i;
            drop_priority = priority;
        }
    }

    if (static_cast<std::uint8_t>(sample.priority) <
        static_cast<std::uint8_t>(drop_priority)) {
        ++drop_count_;
        syslog(LOG_WARNING,
               "ring_buffer: dropped incoming low-priority sample for %s",
               sample.channel_id);
        return false;
    }

    if (drop_offset == 0) {
        head_ = (head_ + 1) % capacity_;
        --size_;
    } else {
        remove_at_offset(slots_, capacity_, head_, size_, drop_offset);
    }

    ++drop_count_;
    syslog(LOG_WARNING, "ring_buffer: dropped priority=%u sample to make room",
           static_cast<unsigned>(drop_priority));

    slots_[(head_ + size_) % capacity_] = sample;
    ++size_;
    return true;
}

std::optional<StoredSample> RingBuffer::dequeue() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (size_ == 0) {
        return std::nullopt;
    }
    StoredSample sample = slots_[head_];
    head_ = (head_ + 1) % capacity_;
    --size_;
    return sample;
}

std::size_t RingBuffer::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_;
}

std::size_t RingBuffer::capacity() const { return capacity_; }

bool RingBuffer::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_ == 0;
}

std::size_t RingBuffer::drop_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return drop_count_;
}

} // namespace bossa::telemetry
