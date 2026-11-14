// Copyright (C) 2026 DShare-HID contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>

namespace open_bridge {
// Report callbacks receive transient buffers. Mouse reports are buttons,
// signed 16-bit X/Y (little endian), and signed 8-bit wheel.
//
// Mouse movement is accumulated rather than reported at once: the host can
// send movement far faster than Bluetooth delivers reports, and a report per
// movement backs up the link and then the USB input. The caller sends the
// accumulated movement with flushMotion() at the Bluetooth report rate. Button
// and wheel reports carry any pending movement, so events keep their order.
class HidState {
public:
  enum class Report : uint8_t { Keyboard, Mouse, Consumer };
  using Send = void (*)(void *, Report, const uint8_t *, size_t);
  HidState(Send send, void *context) : send_(send), context_(context) {}
  bool handle(const uint8_t *packet, size_t length);
  void releaseAll();
  // Sends one report with pending movement, up to the 16-bit limit per axis.
  // Returns false if there was nothing to send.
  bool flushMotion();
  bool hasPendingMotion() const { return pendingX_ != 0 || pendingY_ != 0; }
private:
  int16_t takeMotion(int32_t &pending);
  void keyboard();
  void mouse(int16_t x, int16_t y, int8_t wheel);
  void consumer();
  Send send_;
  void *context_;
  uint8_t modifiers_ = 0, keys_[6]{}, buttons_ = 0;
  uint16_t consumer_ = 0;
  int32_t pendingX_ = 0, pendingY_ = 0;
};
} // namespace open_bridge
