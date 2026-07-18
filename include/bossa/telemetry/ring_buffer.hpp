/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <vector>

#include "bossa/telemetry/stored_sample.hpp"

namespace bossa::telemetry {

/**
 * @brief Fixed-capacity sample buffer with priority-aware overflow.
 *
 * Capacity is fixed at construction. enqueue/dequeue do not allocate.
 * On overflow, the lowest-priority resident sample is dropped first.
 */
class RingBuffer {
  public:
    /**
     * @brief Construct a ring buffer.
     * @param capacity Maximum number of stored samples (must be > 0).
     */
    explicit RingBuffer(std::size_t capacity);

    /**
     * @brief Enqueue a sample; may drop a lower-priority resident on overflow.
     * @param sample Sample to store.
     * @return true if the new sample was retained.
     */
    bool enqueue(const StoredSample &sample);

    /**
     * @brief Remove and return the oldest sample.
     * @return Sample if non-empty; otherwise nullopt.
     */
    std::optional<StoredSample> dequeue();

    /** @brief Current number of resident samples. */
    std::size_t size() const;

    /** @brief Configured capacity. */
    std::size_t capacity() const;

    /** @brief True when no samples are stored. */
    bool empty() const;

    /** @brief Number of samples dropped due to overflow since construction. */
    std::size_t drop_count() const;

  private:
    mutable std::mutex mutex_;
    std::vector<StoredSample> slots_;
    std::size_t capacity_{0};
    std::size_t head_{0};
    std::size_t size_{0};
    std::size_t drop_count_{0};
};

} // namespace bossa::telemetry
