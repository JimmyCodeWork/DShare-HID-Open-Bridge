/*
 * SPDX-FileCopyrightText: 2026 DShare-HID Open Bridge contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "PointerHomingTests.h"

#include "platform/bridge/PointerHoming.h"

#include <cstdlib>
#include <limits>

using namespace deskflow::bridge;
using namespace deskflow::bridge::pointer;

namespace {

struct Total
{
  int64_t x = 0;
  int64_t y = 0;
};

Total sum(std::vector<PointerStep>::const_iterator begin, std::vector<PointerStep>::const_iterator end)
{
  Total total;
  for (auto it = begin; it != end; ++it) {
    total.x += it->dx;
    total.y += it->dy;
  }
  return total;
}

Total sum(const std::vector<PointerStep> &steps)
{
  return sum(steps.begin(), steps.end());
}

} // namespace

void PointerHomingTests::splitMoveKeepsTheTotal()
{
  const auto steps = splitMove(1300, -700, 512);
  const auto total = sum(steps);
  QCOMPARE(total.x, int64_t(1300));
  QCOMPARE(total.y, int64_t(-700));
  QCOMPARE(steps.size(), size_t(3));
}

void PointerHomingTests::splitMoveStaysWithinTheStep()
{
  for (const auto &step : splitMove(-5000, 2049, 256)) {
    QVERIFY(std::abs(step.dx) <= 256);
    QVERIFY(std::abs(step.dy) <= 256);
  }
}

void PointerHomingTests::splitMoveOfNothingIsEmpty()
{
  QVERIFY(splitMove(0, 0).empty());
}

void PointerHomingTests::splitMoveHandlesLargeAndInvalidInput()
{
  // beyond int16 in total, and a step that is out of range either way
  const auto large = splitMove(100000, -100000, 1000000);
  QCOMPARE(sum(large).x, int64_t(100000));
  QCOMPARE(sum(large).y, int64_t(-100000));
  for (const auto &step : large) {
    QVERIFY(step.dx <= std::numeric_limits<int16_t>::max());
    QVERIFY(step.dy >= -std::numeric_limits<int16_t>::max());
  }

  const auto tiny = splitMove(3, -2, 0);
  QCOMPARE(sum(tiny).x, int64_t(3));
  QCOMPARE(sum(tiny).y, int64_t(-2));
  QCOMPARE(tiny.size(), size_t(3));
}

void PointerHomingTests::homingStartsInTheTopLeftCorner()
{
  const auto steps = homeAndMoveTo(1376, 1032);
  QVERIFY(steps.size() > size_t(kHomingSteps));
  for (int i = 0; i < kHomingSteps; ++i) {
    QCOMPARE(steps[i], (PointerStep{kHomingDelta, kHomingDelta}));
  }
  // far more than any screen, in counts, so the pointer is pinned in the corner
  const auto homing = sum(steps.begin(), steps.begin() + kHomingSteps);
  QVERIFY(homing.x <= -30000);
  QVERIFY(homing.y <= -30000);
}

void PointerHomingTests::homingThenReachesTheTarget()
{
  const auto steps = homeAndMoveTo(2752, 1000);
  const auto toTarget = sum(steps.begin() + kHomingSteps, steps.end());
  QCOMPARE(toTarget.x, int64_t(2752));
  QCOMPARE(toTarget.y, int64_t(1000));
  for (auto it = steps.begin() + kHomingSteps; it != steps.end(); ++it) {
    QVERIFY(it->dx >= 0 && it->dx <= kMaxStepDelta);
    QVERIFY(it->dy >= 0 && it->dy <= kMaxStepDelta);
  }
}

void PointerHomingTests::homingToTheCornerOnlyHomes()
{
  QCOMPARE(homeAndMoveTo(0, 0).size(), size_t(kHomingSteps));
}

void PointerHomingTests::homingClampsNegativeTargets()
{
  const auto steps = homeAndMoveTo(-50, 300);
  const auto toTarget = sum(steps.begin() + kHomingSteps, steps.end());
  QCOMPARE(toTarget.x, int64_t(0));
  QCOMPARE(toTarget.y, int64_t(300));
}

QTEST_MAIN(PointerHomingTests)
