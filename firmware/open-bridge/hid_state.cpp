// Copyright (C) 2026 DShare-HID contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "hid_state.h"
#include <cstring>
#include <limits>

namespace open_bridge {
void HidState::keyboard()
{
  uint8_t report[8] = {modifiers_, 0};
  std::memcpy(report + 2, keys_, sizeof(keys_));
  if (send_) send_(context_, Report::Keyboard, report, sizeof(report));
}
void HidState::mouse(int16_t x, int16_t y, int8_t wheel)
{
  const uint16_t ux = static_cast<uint16_t>(x), uy = static_cast<uint16_t>(y);
  const uint8_t report[] = {buttons_, static_cast<uint8_t>(ux), static_cast<uint8_t>(ux >> 8),
                            static_cast<uint8_t>(uy), static_cast<uint8_t>(uy >> 8),
                            static_cast<uint8_t>(wheel)};
  if (send_) send_(context_, Report::Mouse, report, sizeof(report));
}
int16_t HidState::takeMotion(int32_t &pending)
{
  constexpr int32_t limit = std::numeric_limits<int16_t>::max();
  const int32_t step = pending > limit ? limit : (pending < -limit ? -limit : pending);
  pending -= step;
  return static_cast<int16_t>(step);
}
bool HidState::flushMotion()
{
  if (!hasPendingMotion()) return false;
  const int16_t x = takeMotion(pendingX_);
  const int16_t y = takeMotion(pendingY_);
  mouse(x, y, 0);
  return true;
}
void HidState::consumer()
{
  const uint8_t report[] = {static_cast<uint8_t>(consumer_), static_cast<uint8_t>(consumer_ >> 8)};
  if (send_) send_(context_, Report::Consumer, report, sizeof(report));
}
bool HidState::handle(const uint8_t *p, size_t n)
{
  if (!p || n < 4 || p[0] != 0x55 || p[1] != 0xaa || n != size_t(p[3]) + 4) return false;
  const uint8_t *d = p + 4;
  switch (p[2]) {
  case 1: case 2: {
    if (p[3] != 2) return false;
    // The host supplies the current modifier bitmap, including on release.
    modifiers_ = d[0];
    const uint8_t key = d[1];
    if (key && p[2] == 1) {
      bool found = false;
      for (auto k : keys_) found |= k == key;
      if (!found) {
        bool inserted = false;
        for (auto &k : keys_) if (!k) { k = key; inserted = true; break; }
        if (!inserted) { releaseAll(); return false; }
      }
    } else if (key) {
      for (auto &k : keys_) if (k == key) k = 0;
    }
    keyboard();
    return true;
  }
  case 3: {
    if (p[3] != 4) return false;
    // Accumulate; flushMotion() sends it. Saturate rather than overflow if
    // nothing is flushed for a long time.
    constexpr int32_t cap = 1 << 24;
    const int32_t x = static_cast<int16_t>(uint16_t(d[0]) | (uint16_t(d[1]) << 8));
    const int32_t y = static_cast<int16_t>(uint16_t(d[2]) | (uint16_t(d[3]) << 8));
    pendingX_ = pendingX_ + x > cap ? cap : (pendingX_ + x < -cap ? -cap : pendingX_ + x);
    pendingY_ = pendingY_ + y > cap ? cap : (pendingY_ + y < -cap ? -cap : pendingY_ + y);
    return true;
  }
  case 4: case 5:
    // Desktop sends Deskflow button IDs: left=1, middle=2, right=3.
    // HID uses a bitmap with right before middle.
    if (p[3] != 1 || d[0] < 1 || d[0] > 5) return false;
    {
      static constexpr uint8_t masks[] = {0, 1, 4, 2, 8, 16};
      const uint8_t mask = masks[d[0]];
      if (p[2] == 4) buttons_ |= mask; else buttons_ &= static_cast<uint8_t>(~mask);
    }
    {
      // the press or release happens where the pointer has moved to so far
      const int16_t x = takeMotion(pendingX_), y = takeMotion(pendingY_);
      mouse(x, y, 0);
    }
    return true;
  case 6:
    if (p[3] != 1) return false;
    {
      const int16_t x = takeMotion(pendingX_), y = takeMotion(pendingY_);
      mouse(x, y, static_cast<int8_t>(d[0]));
    }
    return true;
  case 7: case 8:
    if (p[3] != 2) return false;
    if (p[2] == 7) consumer_ = uint16_t(d[0]) | (uint16_t(d[1]) << 8);
    else if (consumer_ == (uint16_t(d[0]) | (uint16_t(d[1]) << 8))) consumer_ = 0;
    consumer(); return true;
  default: return false;
  }
}
void HidState::releaseAll()
{
  modifiers_ = buttons_ = 0;
  consumer_ = 0;
  pendingX_ = pendingY_ = 0;
  std::memset(keys_, 0, sizeof(keys_));
  keyboard(); mouse(0, 0, 0); consumer();
}
} // namespace open_bridge
