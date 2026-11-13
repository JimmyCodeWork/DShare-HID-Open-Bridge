// Copyright (C) 2026 DShare-HID contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "../usb_frame_decoder.h"
#include <cassert>
#include <cstdint>
#include <vector>

struct Frame { uint8_t type, flags; std::vector<uint8_t> payload; };
static void receive(void *ctx, uint8_t type, uint8_t flags, const uint8_t *p, size_t n)
{
  static_cast<std::vector<Frame> *>(ctx)->push_back({type, flags, {p, p + n}});
}
int main()
{
  std::vector<Frame> frames;
  open_bridge::UsbFrameDecoder decoder(receive, &frames);
  const uint8_t stream[] = {0x00, 0x5a, 0x5a, 0xc3, 1, 0x80, 0, 0, 2, 0, 1, 2,
                            0x5a, 0xc3, 1, 1, 7, 0, 0, 0};
  for (auto byte : stream) decoder.feed(&byte, 1);
  assert(frames.size() == 2 && frames[0].type == 0x80);
  assert((frames[0].payload == std::vector<uint8_t>{1, 2}));
  assert(frames[1].flags == 7 && frames[1].payload.empty());
  const uint8_t malformed[] = {0x5a, 0xc3, 1, 1, 0, 0, 1, 1,
                               0x5a, 0xc3, 2, 1, 0, 0, 0, 0,
                               0x5a, 0xc3, 1, 1, 0, 1, 0, 0,
                               0x5a, 0xc3, 1, 1, 0, 0, 1, 0, 42};
  decoder.feed(malformed, sizeof(malformed));
  assert(frames.size() == 3 && frames.back().payload[0] == 42);
  decoder.feed(stream + 2, 5);
  decoder.reset();
  decoder.feed(stream + 2, 10);
  assert(frames.size() == 4);
}
