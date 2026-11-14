/*
 * SPDX-FileCopyrightText: 2026 DShare-HID Open Bridge contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "PointerHoming.h"

#include <algorithm>
#include <cstdlib>
#include <limits>

namespace deskflow::bridge::pointer {

std::vector<PointerStep> splitMove(int32_t dx, int32_t dy, int32_t maxStep)
{
  // a report carries int16 deltas, and a non-positive step would never finish
  maxStep = std::clamp<int32_t>(maxStep, 1, std::numeric_limits<int16_t>::max());

  std::vector<PointerStep> steps;
  int64_t remainingX = dx;
  int64_t remainingY = dy;
  while (remainingX != 0 || remainingY != 0) {
    const auto stepX = static_cast<int16_t>(std::clamp<int64_t>(remainingX, -maxStep, maxStep));
    const auto stepY = static_cast<int16_t>(std::clamp<int64_t>(remainingY, -maxStep, maxStep));
    steps.push_back({stepX, stepY});
    remainingX -= stepX;
    remainingY -= stepY;
  }
  return steps;
}

std::vector<PointerStep> homeAndMoveTo(int32_t x, int32_t y)
{
  std::vector<PointerStep> steps(kHomingSteps, PointerStep{kHomingDelta, kHomingDelta});
  const auto toTarget = splitMove((std::max)(x, 0), (std::max)(y, 0));
  steps.insert(steps.end(), toTarget.begin(), toTarget.end());
  return steps;
}

} // namespace deskflow::bridge::pointer
