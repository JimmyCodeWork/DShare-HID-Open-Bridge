/*
 * SPDX-FileCopyrightText: 2026 DShare-HID Open Bridge contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/KeyTypes.h"

#include <cstdint>
#include <vector>

namespace deskflow::bridge {

/**
 * @brief One relative mouse report: a movement of (dx, dy) counts
 */
struct PointerStep
{
  int16_t dx = 0;
  int16_t dy = 0;

  bool operator==(const PointerStep &other) const
  {
    return dx == other.dx && dy == other.dy;
  }
};

/**
 * @brief Re-synchronising a relative pointer with the client's cursor position
 *
 * A bridge sends relative mouse reports, and the device (for example iPadOS) applies its own
 * pointer speed and acceleration to them. The device's pointer therefore drifts away from the
 * cursor position the client tracks, and it can end up confined to part of the screen.
 *
 * When the cursor enters the device, the client first pushes the pointer far up and to the
 * left, so the device pins it in its top-left corner, which matches the client's (0, 0). It then
 * moves the pointer to the entry position. Errors no longer accumulate across entries.
 */
namespace pointer {

/// Reports that push the pointer into the top-left corner. Together they move far more than
/// any device screen, in counts, so the corner is reached despite pointer speed settings.
inline constexpr int kHomingSteps = 8;
inline constexpr int16_t kHomingDelta = -4096;

/// The largest movement per axis in one report when moving to the entry position. Smaller
/// reports are closer to ordinary mouse movement than one large jump.
inline constexpr int32_t kMaxStepDelta = 512;

/**
 * @brief The reports that move a pointer by (dx, dy), each at most @p maxStep counts per axis
 */
std::vector<PointerStep> splitMove(int32_t dx, int32_t dy, int32_t maxStep = kMaxStepDelta);

/**
 * @brief The reports that home the pointer in the top-left corner, then move it to (x, y)
 *
 * Negative coordinates are treated as 0.
 */
std::vector<PointerStep> homeAndMoveTo(int32_t x, int32_t y);

/// The largest coordinate an absolute pointer report carries (logical maximum in the firmware's
/// report descriptor). 0 is the left or top edge, kAbsoluteMax the right or bottom edge.
inline constexpr uint16_t kAbsoluteMax = 32767;

/**
 * @brief A cursor coordinate on a screen axis of @p size pixels, scaled to 0..kAbsoluteMax
 *
 * Coordinates outside the screen are clamped to its edges.
 */
uint16_t toAbsolute(int32_t position, int32_t size);

/**
 * @brief Whether a key press is the shortcut that re-synchronises the device's pointer
 *
 * Ctrl+Shift+Home, with no other modifier held (lock keys are ignored). The bridge handles it
 * itself and doesn't send it to the device.
 */
bool isRecalibrateShortcut(KeyID id, KeyModifierMask mask);

} // namespace pointer
} // namespace deskflow::bridge
