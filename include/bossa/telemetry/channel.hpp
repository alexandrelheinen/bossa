/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>

namespace bossa::telemetry {

/** @brief Sync / ring-buffer drop priority (lower drops first on overflow). */
enum class Priority : std::uint8_t {
    kLow = 0,
    kNormal = 1,
    kCritical = 2,
};

/** @brief Declarative upload mode for a channel. */
enum class SyncMode : std::uint8_t {
    kBatch = 0,
    kRealtime = 1,
    kOnChange = 2,
};

} // namespace bossa::telemetry
