// Copyright (C) 2026 DShare-HID contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>

namespace open_bridge {
// Report callbacks receive transient buffers. Mouse reports are buttons,
// signed 16-bit X/Y (little endian), and signed 8-bit wheel.
class HidState {
public:
  enum class Report : uint8_t { Keyboard, Mouse, Consumer };
  using Send = void (*)(void *, Report, const uint8_t *, size_t);
  HidState(Send send, void *context) : send_(send), context_(context) {}
  bool handle(const uint8_t *packet, size_t length);
  void releaseAll();
private:
  void keyboard();
  void mouse(int16_t x, int16_t y, int8_t wheel);
  void consumer();
  Send send_;
  void *context_;
  uint8_t modifiers_ = 0, keys_[6]{}, buttons_ = 0;
  uint16_t consumer_ = 0;
};
} // namespace open_bridge
