// Copyright (C) 2026 DShare-HID contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>

namespace open_bridge {

// Incremental decoder for the desktop's USB link. Storage is fixed so corrupt
// lengths cannot exhaust memory on the microcontroller.
class UsbFrameDecoder {
public:
  static constexpr size_t maxPayload = 256;
  using Handler = void (*)(void *context, uint8_t type, uint8_t flags,
                           const uint8_t *payload, size_t length);

  UsbFrameDecoder(Handler handler, void *context) : handler_(handler), context_(context) {}
  void feed(const uint8_t *data, size_t length);
  void reset() { used_ = 0; }

private:
  void parse();
  void discard(size_t count);
  uint8_t buffer_[8 + maxPayload]{};
  size_t used_ = 0;
  Handler handler_;
  void *context_;
};

} // namespace open_bridge
