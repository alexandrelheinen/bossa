/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <string_view>

namespace bossa::core {

/**
 * @brief Reserved edge node_id for automated / CI telemetry.
 *
 * Never assign this id to a production device. Phase 4 D1 cleanup and local
 * test harnesses delete all rows keyed by this value.
 *
 * Historical note: treated as the reserved "0x00" test identity in prose;
 * the wire format is a stable UTF-8 node_id string (not a binary byte).
 */
inline constexpr const char *kReservedTestNodeId = "bossa-test-00000000";

/** @brief True when @p node_id is the reserved CI / sim identity. */
inline bool is_reserved_test_node(std::string_view node_id) {
    return node_id == kReservedTestNodeId;
}

} // namespace bossa::core
