// Copyright (C) 2026 DShare-HID contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "usb_frame_decoder.h"
#include <cstring>

namespace open_bridge {
void UsbFrameDecoder::discard(size_t count)
{
  used_ -= count;
  std::memmove(buffer_, buffer_ + count, used_);
}

void UsbFrameDecoder::feed(const uint8_t *data, size_t length)
{
  for (size_t i = 0; i < length; ++i) {
    buffer_[used_++] = data[i];
    parse();
  }
}

void UsbFrameDecoder::parse()
{
  while (used_ >= 2) {
    if (buffer_[0] != 0x5a || buffer_[1] != 0xc3) {
      discard(1);
      continue;
    }
    if (used_ < 8) return;
    const size_t length = buffer_[6] | (static_cast<size_t>(buffer_[7]) << 8);
    if (buffer_[2] != 1 || buffer_[5] != 0 || length > maxPayload) {
      discard(1);
      continue;
    }
    if (used_ < 8 + length) return;
    if (handler_) handler_(context_, buffer_[3], buffer_[4], buffer_ + 8, length);
    discard(8 + length);
  }
}
} // namespace open_bridge
